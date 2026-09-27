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

// FNV-1a over the whole 512-byte block; a moved hash means a write landed
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

// Per-controller liveness.
//
// CVService's fused-pose thread rewrites both blocks about every 4 ms
// while its controller link runs and brackets each section write with the
// flag bytes, so the wire keeps moving even when the decoded state sits
// identical - a parked controller's pose and keys just don't change.
// Content change alone therefore can't be the link heartbeat: "the
// writer touched it" is. The flag flaps are observed by spin-sampling
// the mapped bytes for one write period; a slot that still shows
// all-zero data was reset or never had a controller on it.
struct ctrl_live {
    uint64_t hash;        // last block content hash
    uint64_t change_ns;   // CLOCK_MONOTONIC stamp of the last write seen
    uint64_t probe_ns;    // next allowed flag-edge probe (rate limit)
    int seen;             // a baseline hash exists
};

// a block counts live while a write was observed inside this window
#define CTRL_LIVE_NS 800000000ull

// quiet-content probes: cadence and spin budget. The write period is ~4
// ms, so a spin longer than that always crosses a full write when the
// service is streaming
#define CTRL_PROBE_GAP_NS 250000000ull
#define CTRL_PROBE_SPIN_NS 8000000ull

// fold one frame's observation into the tracker: a moved hash or a caught
// flag edge both mean a write landed. Returns nonzero while live.
int ctrl_live_feed(struct ctrl_live *l, uint64_t hash, int wire_edge,
                   uint64_t now_ns);

// nonzero when the flag bytes are worth probing this frame - only while
// the content is quiet, and rate-limited. Calling it claims the slot, so
// run the probe right after
int ctrl_live_probe_due(struct ctrl_live *l, uint64_t hash, uint64_t now_ns);

// spin-samples a block's flag bytes for a value change (a write crossing
// the probe window). Returns nonzero on the first edge, 0 when the budget
// ran out. Reads the live mapping, not a snapshot copy.
int ctrl_share_write_edge(const struct ctrl_share *s, int which,
                          uint64_t budget_ns);

// nonzero while the block carries controller data; the service publishes
// an all-zero frame for a reset or never-linked slot while the flags
// keep flapping
int ctrl_block_has_data(const uint8_t buf[CTRL_SHARE_SIZE], int which);

#ifdef __cplusplus
}
#endif

#endif
