// Home-environment state machine for the env process. Everything but the
// file read and the VBO upload lives in the pure modules (envmap.cpp
// parses, zipfile.cpp extracts); this file is the thin per-frame glue:
// poll the selection prop, load on change, free on switch-away.
//
// A custom pick that fails to load leaves envVerts at 0, which drawScene
// reads as "fall back to the built-in sky and grid" - a bad zip can never
// leave the user standing in a black void.

#include "homeenv.h"

#include "envmap.h"
#include "zipfile.h"
#include "../engine.h"
#include "../common/config.h"
#include "../common/log.h"
#include "../common/props.h"

#include <GLES2/gl2.h>
#include <sys/system_properties.h>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <vector>

namespace {

long long nowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void clearMesh(Engine* e) {
    if (e->envVbo) glDeleteBuffers(1, &e->envVbo);
    e->envVbo = 0;
    e->envVerts = 0;
}

// zip -> map.obj -> baked mesh -> VBO. Any miss along the chain leaves the
// mesh empty; the caller reports the scene fallback.
void load(Engine* e, const char* id) {
    clearMesh(e);
    char path[PROP_VALUE_MAX + 32];
    if (!envZipPath(id, path, sizeof(path))) {
        LOGE("env: bad id '%s'", id);
        return;
    }
    FILE* f = fopen(path, "rb");
    if (!f) {
        LOGE("env: cannot open %s", path);
        return;
    }
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || size > 64L * 1024 * 1024) {
        fclose(f);
        LOGE("env: %s size %ld out of range", path, size);
        return;
    }
    std::vector<uint8_t> zip((size_t)size);
    const size_t got = fread(zip.data(), 1, zip.size(), f);
    fclose(f);
    if (got != zip.size()) {
        LOGE("env: short read on %s", path);
        return;
    }
    std::vector<uint8_t> obj;
    if (!zipRead(zip.data(), zip.size(), "map.obj", &obj)) {
        LOGE("env: %s has no readable map.obj", path);
        return;
    }
    EnvMap map;
    if (!envFromObj((const char*)obj.data(), obj.size(), &map)) {
        LOGE("env: %s map.obj has no geometry", path);
        return;
    }
    if (!e->envVbo) glGenBuffers(1, &e->envVbo);
    glBindBuffer(GL_ARRAY_BUFFER, e->envVbo);
    glBufferData(GL_ARRAY_BUFFER, map.v.size() * sizeof(float),
                 map.v.data(), GL_STATIC_DRAW);
    e->envVerts = (int)(map.v.size() / 6);
    LOGI("env: %s loaded, %d verts", path, e->envVerts);
}

} // namespace

void envTick(Engine* e) {
    const long long now = nowMs();
    if (now < e->envPollAt) return;
    e->envPollAt = now + kEnvPollMs;

    char sel[PROP_VALUE_MAX];
    if (!propS(kEnvDebugProp, sel, sizeof(sel)))
        propS(kEnvProp, sel, sizeof(sel));

    const EnvMode mode = envModeOf(sel);
    e->envMode = mode;
    if (mode != kEnvCustom) {
        if (e->envVbo) clearMesh(e);
        e->envSel[0] = 0;
        return;
    }
    if (strcmp(sel, e->envSel) == 0) {
        if (e->envVerts > 0) return;         // loaded, leave it alone
        if (now < e->envRetryAt) return;     // a miss backs off, then retries
    }
    strncpy(e->envSel, sel, sizeof(e->envSel) - 1);
    e->envSel[sizeof(e->envSel) - 1] = 0;
    load(e, sel);
    // a zip that failed gets another shot every few seconds: pushing a
    // fixed file under the same name has to heal without a reboot
    e->envRetryAt = e->envVerts > 0 ? 0 : now + 3000;
}

void envDraw(Engine* e, const Mat4& viewProj) {
    glUseProgram(e->sceneProg);
    glUniformMatrix4fv(glGetUniformLocation(e->sceneProg, "uMVP"),
                       1, GL_FALSE, viewProj.m);
    glBindBuffer(GL_ARRAY_BUFFER, e->envVbo);
    const GLint aPos = glGetAttribLocation(e->sceneProg, "aPos");
    const GLint aCol = glGetAttribLocation(e->sceneProg, "aCol");
    glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)0);
    glVertexAttribPointer(aCol, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(aPos);
    glEnableVertexAttribArray(aCol);
    glDrawArrays(GL_TRIANGLES, 0, e->envVerts);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aCol);
}

void envRelease(Engine* e) {
    clearMesh(e);
    e->envSel[0] = 0;
}
