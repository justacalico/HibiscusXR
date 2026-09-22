#include "ctrl_state.h"

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static int32_t be_i32(const uint8_t *p) {
    return (int32_t)((uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
                     (uint32_t)p[2] << 8 | (uint32_t)p[3]);
}

static int64_t be_i64(const uint8_t *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v = v << 8 | p[i];
    return (int64_t)v;
}

static float be_f32(const uint8_t *p) {
    uint32_t u = (uint32_t)be_i32(p);
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static void pose_at(const uint8_t *p, struct ctrl_pose *out) {
    out->x = be_f32(p + 0);
    out->y = be_f32(p + 4);
    out->z = be_f32(p + 8);
    out->q0 = be_f32(p + 12);
    out->qx = be_f32(p + 16);
    out->qy = be_f32(p + 20);
    out->qz = be_f32(p + 24);
    out->status = be_i32(p + 28);
    out->timestamp = be_i64(p + 32);
}

void ctrl_state_decode(const uint8_t buf[CTRL_SHARE_SIZE], int which,
                       struct ctrl_state *out) {
    memset(out, 0, sizeof(*out));
    const int pose_flag = CTRL_POSE_FLAG(which);
    const int key_flag = CTRL_KEY_FLAG(which);
    out->pose_ok = buf[pose_flag] == CTRL_FLAG_DONE;
    out->keys_ok = buf[key_flag] == CTRL_FLAG_DONE;
    if (out->pose_ok) {
        const uint8_t *p = buf + pose_flag + 4;
        pose_at(p, &out->fixed);
        pose_at(p + 40, &out->fuse);
        out->head_y = be_f32(p + 80);
    }
    if (out->keys_ok) {
        const uint8_t *k = buf + key_flag + 4;
        out->keys.touch_x = be_i32(k + 0);
        out->keys.touch_y = be_i32(k + 4);
        out->keys.home = be_i32(k + 8);
        out->keys.app = be_i32(k + 12);
        out->keys.rocker = be_i32(k + 16);
        out->keys.trigger = be_i32(k + 20);
        out->keys.battery = be_i32(k + 24);
        out->keys.a = be_i32(k + 28);
        out->keys.b = be_i32(k + 32);
        out->keys.grip_l = be_i32(k + 36);
        out->keys.grip_r = be_i32(k + 40);
    }
}

uint64_t ctrl_state_hash(const uint8_t buf[CTRL_SHARE_SIZE], int which) {
    const uint8_t *p = buf + which * 512;
    uint64_t h = 1469598103934665603ull;
    for (int i = 0; i < 512; i++) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

int ctrl_share_open(struct ctrl_share *s, const char *path) {
    memset(s, 0, sizeof(*s));
    s->fd = -1;
    if (!path) path = CTRL_SHARE_PATH;
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return -1;
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size < (off_t)CTRL_SHARE_SIZE) {
        close(fd);
        return -2;
    }
    void *m = mmap(NULL, CTRL_SHARE_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) {
        close(fd);
        return -3;
    }
    s->fd = fd;
    s->map = (volatile uint8_t *)m;
    s->size = CTRL_SHARE_SIZE;
    return 0;
}

void ctrl_share_close(struct ctrl_share *s) {
    if (s->map) munmap((void *)s->map, s->size);
    if (s->fd >= 0) close(s->fd);
    s->map = NULL;
    s->fd = -1;
}

int ctrl_share_snapshot(struct ctrl_share *s, uint8_t buf[CTRL_SHARE_SIZE]) {
    if (!s->map) return -1;
    // a block reads as mid-write when its flag stays WRITING; re-copy a few
    // times and take whatever settled
    for (int tries = 0; tries < 8; tries++) {
        memcpy(buf, (const uint8_t *)s->map, CTRL_SHARE_SIZE);
        if (buf[CTRL_POSE_FLAG(CTRL_LEFT)] != CTRL_FLAG_WRITING &&
            buf[CTRL_KEY_FLAG(CTRL_LEFT)] != CTRL_FLAG_WRITING &&
            buf[CTRL_POSE_FLAG(CTRL_RIGHT)] != CTRL_FLAG_WRITING &&
            buf[CTRL_KEY_FLAG(CTRL_RIGHT)] != CTRL_FLAG_WRITING)
            return 0;
    }
    memcpy(buf, (const uint8_t *)s->map, CTRL_SHARE_SIZE);
    return 0;
}
