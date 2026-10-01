#include "xrtest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb_truetype.h"

// ---------- gles ----------

static const char *VS =
    "attribute vec3 aPos; attribute vec3 aCol; uniform mat4 uMvp; uniform vec3 uTint; varying vec3 vCol;"
    "void main(){ vCol=aCol*uTint; gl_Position=uMvp*vec4(aPos,1.0);}";
static const char *FS =
    "precision mediump float; varying vec3 vCol; void main(){ gl_FragColor=vec4(vCol,1.0);}";

static const char *TVS =
    "attribute vec3 aPos; attribute vec2 aUV; uniform mat4 uMvp; varying vec2 vUV;"
    "void main(){ vUV=aUV; gl_Position=uMvp*vec4(aPos,1.0);}";
static const char *TFS =
    "precision mediump float; varying vec2 vUV; uniform sampler2D uTex; uniform vec4 uCol;"
    "void main(){ float a=texture2D(uTex,vUV).a; if(a<0.02) discard; gl_FragColor=vec4(uCol.rgb,uCol.a*a);}";

static GLuint compile(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        LOGE("shader: %s", log);
    }
    return s;
}

static GLuint link_prog(const char *vs, const char *fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, compile(GL_VERTEX_SHADER, vs));
    glAttachShader(p, compile(GL_FRAGMENT_SHADER, fs));
    glLinkProgram(p);
    return p;
}

GLuint link_color_prog(void) { return link_prog(VS, FS); }

GLuint link_text_prog(void) { return link_prog(TVS, TFS); }

// ---------- font ----------

#define FONT_PX 34
#define ATLAS_W 512
#define ATLAS_H 512
#define FONT_FIRST 32
#define FONT_COUNT 96

static stbtt_bakedchar g_baked[FONT_COUNT];
GLuint g_font_tex = 0;
bool g_font_ok = false;

static const char *g_font_paths[] = {
    "/system/fonts/Roboto-Regular.ttf",
    "/system/fonts/DroidSans.ttf",
    "/system/fonts/NotoSansMono-Regular.ttf",
    "/data/local/tmp/xr/font.ttf",
    NULL};

void init_font(void) {
    FILE *f = NULL;
    for (int i = 0; g_font_paths[i]; i++) {
        f = fopen(g_font_paths[i], "rb");
        if (f) {
            LOGI("font: %s", g_font_paths[i]);
            break;
        }
    }
    if (!f) { LOGE("no usable ttf"); return; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *ttf = malloc(sz);
    if (fread(ttf, 1, sz, f) != (size_t)sz) { fclose(f); free(ttf); return; }
    fclose(f);

    unsigned char *bmp = calloc(ATLAS_W * ATLAS_H, 1);
    int res = stbtt_BakeFontBitmap(ttf, 0, FONT_PX, bmp, ATLAS_W, ATLAS_H,
                                   FONT_FIRST, FONT_COUNT, g_baked);
    free(ttf);
    if (res <= 0) { LOGE("bake failed %d", res); free(bmp); return; }

    glGenTextures(1, &g_font_tex);
    glBindTexture(GL_TEXTURE_2D, g_font_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, ATLAS_W, ATLAS_H, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, bmp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    free(bmp);
    g_font_ok = true;
}

// ---------- debug panel text ----------

float g_textv[MAX_TEXT_CHARS * 6 * 5];
int g_textn; // vertex count
static float g_penx, g_baseline;

void text_reset(void) { g_textn = 0; }

void text_str(float px, float py, const char *s) {
    if (!g_font_ok) return;
    g_penx = px;
    g_baseline = py + FONT_PX;
    for (; *s && g_textn < MAX_TEXT_CHARS * 6 - 6; s++) {
        int c = *s;
        if (c < FONT_FIRST || c >= FONT_FIRST + FONT_COUNT) c = '?';
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(g_baked, ATLAS_W, ATLAS_H, c - FONT_FIRST,
                           &g_penx, &g_baseline, &q, 1);
        float x0 = PANEL_X0 + q.x0 * kMPX, x1 = PANEL_X0 + q.x1 * kMPX;
        float y0 = PANEL_Y0 - q.y0 * kMPX, y1 = PANEL_Y0 - q.y1 * kMPX;
        float *v = g_textv + g_textn * 5;
        float quad[6][5] = {
            {x0, y0, PANEL_Z, q.s0, q.t0}, {x1, y0, PANEL_Z, q.s1, q.t0},
            {x0, y1, PANEL_Z, q.s0, q.t1},
            {x0, y1, PANEL_Z, q.s0, q.t1}, {x1, y0, PANEL_Z, q.s1, q.t0},
            {x1, y1, PANEL_Z, q.s1, q.t1}};
        memcpy(v, quad, sizeof(quad));
        g_textn += 6;
    }
}

// ---------- geometry ----------

GLuint vbo, ibo, cube_vbo, panel_vbo, line_vbo, text_vbo;
int g_lines, g_tris;

void build_scene(void) {
    static float v[4096 * 6];
    static uint16_t idx[8192];
    int nv = 0, ni = 0;
    // floor grid at y=-1.5, xz in [-10,10]
    for (int i = -10; i <= 10; i++) {
        float c = (i == 0) ? 0.9f : 0.3f;
        v[nv*6+0]=-10; v[nv*6+1]=-1.5f; v[nv*6+2]=i;  v[nv*6+3]=c; v[nv*6+4]=c; v[nv*6+5]=c; nv++;
        v[nv*6+0]=10;  v[nv*6+1]=-1.5f; v[nv*6+2]=i;  v[nv*6+3]=c; v[nv*6+4]=c; v[nv*6+5]=c; nv++;
        v[nv*6+0]=i;   v[nv*6+1]=-1.5f; v[nv*6+2]=-10; v[nv*6+3]=c; v[nv*6+4]=c; v[nv*6+5]=c; nv++;
        v[nv*6+0]=i;   v[nv*6+1]=-1.5f; v[nv*6+2]=10;  v[nv*6+3]=c; v[nv*6+4]=c; v[nv*6+5]=c; nv++;
    }
    g_lines = nv;
    for (int i = 0; i < g_lines; i++) idx[ni++] = i;

    // marker cubes: {x,y,z,colorIdx}
    const float cubes[][4] = {
        {0, 0, -3, 0}, {-3, 0, -1, 1}, {3, 0, -1, 2}, {0, 2, -6, 3}, {0, -1.4f, 2, 4}};
    const float cols[][3] = {
        {0.9f, 0.2f, 0.2f}, {0.2f, 0.9f, 0.2f}, {0.25f, 0.35f, 0.95f},
        {0.9f, 0.85f, 0.2f}, {0.7f, 0.25f, 0.8f}};
    static const uint16_t ci36[36] = {
        0,1,2, 2,1,3, 4,6,5, 5,6,7, 0,4,1, 1,4,5,
        2,3,6, 6,3,7, 0,2,4, 4,2,6, 1,5,3, 3,5,7};
    for (int c = 0; c < 5; c++) {
        int base = nv;
        float cx = cubes[c][0], cy = cubes[c][1], cz = cubes[c][2], s = 0.25f;
        int col = (int)cubes[c][3];
        for (int fi = 0; fi < 8; fi++) {
            v[nv*6+0]=cx+((fi&1)?s:-s); v[nv*6+1]=cy+((fi&2)?s:-s); v[nv*6+2]=cz+((fi&4)?s:-s);
            v[nv*6+3]=cols[col][0]; v[nv*6+4]=cols[col][1]; v[nv*6+5]=cols[col][2];
            nv++;
        }
        for (int k = 0; k < 36; k++) idx[ni++] = base + ci36[k];
    }
    g_tris = ni - g_lines;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, nv * 6 * sizeof(float), v, GL_STATIC_DRAW);
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(uint16_t), idx, GL_STATIC_DRAW);

    // unit cube, 36 non-indexed verts, pale faces; uTint colours each instance
    static float cu[36 * 6];
    static const uint8_t face_col[6][3] = {
        {230, 230, 235}, {190, 190, 200}, {215, 215, 225},
        {160, 160, 175}, {205, 205, 215}, {140, 140, 155}};
    // faces: +z -z +x -x +y -y as 2 tris each over a unit cube
    static const float fc[6][4][3] = {
        {{-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}},
        {{ 1,-1,-1}, {-1,-1,-1}, {-1, 1,-1}, { 1, 1,-1}},
        {{ 1,-1, 1}, { 1,-1,-1}, { 1, 1,-1}, { 1, 1, 1}},
        {{-1,-1,-1}, {-1,-1, 1}, {-1, 1, 1}, {-1, 1,-1}},
        {{-1, 1, 1}, { 1, 1, 1}, { 1, 1,-1}, {-1, 1,-1}},
        {{-1,-1,-1}, { 1,-1,-1}, { 1,-1, 1}, {-1,-1, 1}}};
    int cn = 0;
    for (int f = 0; f < 6; f++) {
        const uint8_t *cc = face_col[f];
        const int order[6] = {0, 1, 2, 0, 2, 3};
        for (int k = 0; k < 6; k++) {
            const float *p = fc[f][order[k]];
            cu[cn*6+0] = p[0]; cu[cn*6+1] = p[1]; cu[cn*6+2] = p[2];
            cu[cn*6+3] = cc[0] / 255.f; cu[cn*6+4] = cc[1] / 255.f; cu[cn*6+5] = cc[2] / 255.f;
            cn++;
        }
    }
    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cu), cu, GL_STATIC_DRAW);

    // panel background quad, opaque dark
    float bg[6 * 6];
    const float bc[3] = {0.03f, 0.04f, 0.07f};
    const float bz = PANEL_Z - 0.002f;
    const float bq[4][2] = {
        {PANEL_X0, PANEL_Y0 - PANEL_H}, {PANEL_X0 + PANEL_W, PANEL_Y0 - PANEL_H},
        {PANEL_X0, PANEL_Y0}, {PANEL_X0 + PANEL_W, PANEL_Y0}};
    const int bord[6] = {0, 1, 2, 2, 1, 3};
    for (int k = 0; k < 6; k++) {
        bg[k*6+0] = bq[bord[k]][0]; bg[k*6+1] = bq[bord[k]][1]; bg[k*6+2] = bz;
        bg[k*6+3] = bc[0]; bg[k*6+4] = bc[1]; bg[k*6+5] = bc[2];
    }
    glGenBuffers(1, &panel_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, panel_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bg), bg, GL_STATIC_DRAW);

    glGenBuffers(1, &line_vbo);
    glGenBuffers(1, &text_vbo);
}
