#ifndef CTRL_STATE_H
#define CTRL_STATE_H

// Shared reader for the stock controller channel at /sdcard/CtrlShareMem.
// CVService mmaps that file and rewrites two 512-byte blocks per packet:
// left at 0, right at 512. All fields are big-endian (Java MappedByteBuffer
// order) and each block is bracketed by a flag byte: 1 while writing, 2 when
// done. The pn2 monado driver compiles this same file - one decoder, no
// duplicated layouts. Kept C99 for that reason; it must also compile as C++
// for the host tests.

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CTRL_LEFT 0
#define CTRL_RIGHT 1
#define CTRL_COUNT 2

#define CTRL_SHARE_PATH "/sdcard/CtrlShareMem"
#define CTRL_SHARE_SIZE 1024

// written while a block update is in flight / after it completes
#define CTRL_FLAG_WRITING 1
#define CTRL_FLAG_DONE 2

struct ctrl_pose {
    float x, y, z;
    float q0, qx, qy, qz;   // w first, matching the stock struct
    int32_t status;
    int64_t timestamp;
};

struct ctrl_keys {
    int32_t touch_x, touch_y;   // pad axes, ~0..255 centred near 128
    int32_t home, app;
    int32_t rocker;             // stick click
    int32_t trigger;
    int32_t battery;            // percent
    int32_t a, b;
    int32_t grip_l, grip_r;
};

struct ctrl_state {
    struct ctrl_pose fixed;     // arm-model position, stable when idle
    struct ctrl_pose fuse;      // live fused pose
    float head_y;
    struct ctrl_keys keys;
    int pose_ok;                // flag byte read DONE around the pose block
    int keys_ok;
};

// block flag/data offsets inside the shared file
#define CTRL_POSE_FLAG(which) ((which) * 512)
#define CTRL_POSE_DATA(which) ((which) * 512 + 4)
#define CTRL_KEY_FLAG(which)  ((which) * 512 + 100)
#define CTRL_KEY_DATA(which)  ((which) * 512 + 104)

// decode one controller's block out of a raw snapshot of the file
void ctrl_state_decode(const uint8_t buf[CTRL_SHARE_SIZE], int which,
                       struct ctrl_state *out);

// FNV-1a over the whole 512-byte block; a streaming controller changes this
// every frame, so a stopped hash is the disconnect signal
uint64_t ctrl_state_hash(const uint8_t buf[CTRL_SHARE_SIZE], int which);

// mmap'd reader; open() maps the file, snapshot() copies it out under the
// write flags. All no-gos return nonzero and leave the caller's buffer alone.
struct ctrl_share {
    int fd;
    volatile uint8_t *map;
    size_t size;
};

int ctrl_share_open(struct ctrl_share *s, const char *path);
void ctrl_share_close(struct ctrl_share *s);
int ctrl_share_snapshot(struct ctrl_share *s, uint8_t buf[CTRL_SHARE_SIZE]);

#ifdef __cplusplus
}
#endif

#endif
