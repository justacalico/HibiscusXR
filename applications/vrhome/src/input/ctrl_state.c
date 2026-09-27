#include "ctrl_state.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
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

static uint64_t mono_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
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
    strncpy(s->path, path, sizeof(s->path) - 1);
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

int ctrl_live_feed(struct ctrl_live *l, uint64_t hash, int wire_edge,
                   uint64_t now_ns) {
    if (hash != l->hash) {
        l->hash = hash;
        // first read is just a baseline: the file can sit stale for
        // hours, so a moved hash only counts once the tracker has one
        if (l->seen) l->change_ns = now_ns;
        l->seen = 1;
    }
    if (wire_edge) {
        l->seen = 1;
        l->change_ns = now_ns;
    }
    return l->change_ns != 0 && now_ns - l->change_ns < CTRL_LIVE_NS;
}

struct probe_task {
    struct ctrl_probe *p;
    int which;           // CTRL_LEFT/CTRL_RIGHT, -1 = all four flags
    uint64_t budget_ns;
    char path[128];
};

// spin-samples the flag bytes for one write window: the flag sits at DONE
// between writes and only lifts to WRITING for a few microseconds, so the
// loop has to sample continuously. A flag frozen at WRITING (writer died
// mid-put) never edges and just burns the budget - the dead-writer answer
// either way. Maps its own view so the caller can close the file freely.
static void *probe_main(void *arg) {
    struct probe_task *t = (struct probe_task *)arg;
    int edge = 0;
    int fd = open(t->path[0] ? t->path : CTRL_SHARE_PATH, O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        void *m = mmap(NULL, CTRL_SHARE_SIZE, PROT_READ, MAP_SHARED, fd, 0);
        if (m != MAP_FAILED) {
            volatile const uint8_t *v = (volatile const uint8_t *)m;
            int offs[4];
            int n;
            if (t->which < 0) {
                offs[0] = CTRL_POSE_FLAG(CTRL_LEFT);
                offs[1] = CTRL_KEY_FLAG(CTRL_LEFT);
                offs[2] = CTRL_POSE_FLAG(CTRL_RIGHT);
                offs[3] = CTRL_KEY_FLAG(CTRL_RIGHT);
                n = 4;
            } else {
                offs[0] = CTRL_POSE_FLAG(t->which);
                offs[1] = CTRL_KEY_FLAG(t->which);
                n = 2;
            }
            uint8_t prev[4];
            for (int i = 0; i < n; i++) prev[i] = v[offs[i]];
            const uint64_t end = mono_ns() + t->budget_ns;
            for (uint32_t i = 0;; i++) {
                for (int f = 0; f < n; f++) {
                    const uint8_t cur = v[offs[f]];
                    if (cur != prev[f]) { edge = 1; break; }
                    prev[f] = cur;
                }
                if (edge || ((i & 0x3ff) == 0x3ff && mono_ns() >= end)) break;
            }
            munmap(m, CTRL_SHARE_SIZE);
        }
        close(fd);
    }
    __atomic_store_n(&t->p->state, edge ? 2 : 3, __ATOMIC_RELEASE);
    free(t);
    return NULL;
}

int ctrl_probe_start(struct ctrl_probe *p, const char *path, int which,
                     uint64_t budget_ns) {
    if (__atomic_load_n(&p->state, __ATOMIC_ACQUIRE) != 0) return 0;
    struct probe_task *t = (struct probe_task *)malloc(sizeof(*t));
    if (!t) return 0;
    memset(t, 0, sizeof(*t));
    t->p = p;
    t->which = which;
    t->budget_ns = budget_ns;
    if (path) strncpy(t->path, path, sizeof(t->path) - 1);
    __atomic_store_n(&p->state, 1, __ATOMIC_RELEASE);
    pthread_t th;
    if (pthread_create(&th, NULL, probe_main, t) != 0) {
        __atomic_store_n(&p->state, 0, __ATOMIC_RELEASE);
        free(t);
        return 0;
    }
    pthread_detach(th);
    return 1;
}

int ctrl_probe_poll(struct ctrl_probe *p) {
    const int s = __atomic_load_n(&p->state, __ATOMIC_ACQUIRE);
    if (s == 2) { __atomic_store_n(&p->state, 0, __ATOMIC_RELEASE); return 1; }
    if (s == 3) { __atomic_store_n(&p->state, 0, __ATOMIC_RELEASE); return -1; }
    return 0;
}

int ctrl_block_has_data(const uint8_t buf[CTRL_SHARE_SIZE], int which) {
    const uint8_t *p = buf + which * 512;
    for (int i = 1; i < 512; i++) {
        if (i != 100 && p[i] != 0) return 1;
    }
    return 0;
}
