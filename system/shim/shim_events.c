// DisplayEventReceiver::getEvents translation shim.
//
// Resolving getBuiltInDisplay let libpvrmodule_platform.so load, and it then
// died with "stack corruption detected (-fstack-protector)" inside
// PVR::receiver(). That is not a linkage problem, it is a LAYOUT problem:
// DisplayEventReceiver::Event grew between Android 8.1 and Android 10.
//
//   8.1   Header{ uint32 type; uint32 id;        int64 timestamp; }  = 16
//         Event { Header; union{ VSync; Hotplug; } }                 = 24 bytes
//
//   10    Header{ uint32 type; uint64 displayId; int64 timestamp; }  = 24
//         Event { Header; union{ VSync; Hotplug; Config; } }         = 32 bytes
//
// Pico's receiver() declares a stack array sized with the 24-byte layout and
// asks for N events. Q's getEvents happily writes N * 32 bytes into it and walks
// off the end of the frame. The canary catches it, which is the only reason this
// showed up as a clean abort rather than silent corruption.
//
// So: read into a private Q-layout buffer, repack into the 8.1 layout the caller
// actually allocated, and return the same count. displayId is narrowed back to
// 32 bits, which is lossless here - this device has one internal display.

#include <dlfcn.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <android/log.h>

#define TAG "shim_pvr"

// __android_log_print with a null-format guard.
//
// pvrservice dies in PVR::serviceStatusCallback logging its "onClientStatusChange
// done" message. That call passes its format string in x28, and x28 does not
// survive the virtual dispatch into the notification client just above it, so
// bionic gets a null format and __vfprintf faults at 0x0. 8.1 let this slide;
// Q's printf does not.
//
// The message is pure diagnostics, so losing it costs nothing and keeps the whole
// VR service alive. This is a containment fix, not a repair: the underlying
// clobber still happens and anything else riding on that register is still at
// risk. Kept here because the shim is already preloaded into pvrservice.
int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    static int (*real_vprint)(int, const char *, const char *, va_list);
    static int (*real_write)(int, const char *, const char *);
    if (!real_vprint) {
        real_vprint = dlsym(RTLD_NEXT, "__android_log_vprint");
        real_write  = dlsym(RTLD_NEXT, "__android_log_write");
    }
    if (!real_vprint)
        return 0;
    if (!fmt) {
        // take the non-variadic path: with no va_list there is nothing to get
        // wrong, and the caller's (unknown) arguments are simply never read
        return real_write ? real_write(prio, tag ? tag : TAG,
                                       "<null format suppressed by shim_pvr>") : 0;
    }
    va_list ap;
    va_start(ap, fmt);
    int r = real_vprint(prio, tag ? tag : TAG, fmt, ap);
    va_end(ap);
    return r;
}

// Android 10 layout, what libgui actually writes
struct EventQ {
    uint32_t type;
    uint32_t _pad;
    uint64_t displayId;
    int64_t  timestamp;
    union { uint32_t count; uint8_t connected; int32_t configId; } u;
};

// Android 8.1 layout, what the caller allocated
struct EventO {
    uint32_t type;
    uint32_t id;
    int64_t  timestamp;
    union { uint32_t count; uint8_t connected; } u;
};

_Static_assert(sizeof(struct EventQ) == 32, "Q event layout drifted");
_Static_assert(sizeof(struct EventO) == 24, "O event layout drifted");

// ---------------------------------------------------------------------------
// SurfaceComposerClient::getDisplayInfo - same problem, different struct.
//
// android::DisplayInfo grew between 8.1 and 10: Q appended viewportW and
// viewportH at offsets 48 and 52, taking it from 48 to 56 bytes.
//
// PVR::Platform::getDisplayInfo() lays out its stack as
//     sp+0   DisplayInfo      (48 bytes, per 8.1)
//     sp+48  sp<IBinder>      display token
// so Q writing 56 bytes straight over the token. It then reloads the token,
// dereferences it for its vtable and dies:
//
//     fault addr 0x87000000f00
//                  0x870 = 2160 = viewportH
//                  0xf00 = 3840 = viewportW
//
// The bad pointer is literally the display resolution. Call through into a
// private oversized buffer and copy back only the 48 bytes the caller believes
// in.
#define DISPLAYINFO_O_SIZE 48

typedef int32_t (*getDisplayInfo_t)(const void *display, void *info);

int32_t _ZN7android21SurfaceComposerClient14getDisplayInfoERKNS_2spINS_7IBinderEEEPNS_11DisplayInfoE(
        const void *display, void *info) {
    static getDisplayInfo_t real = NULL;
    if (!real) {
        real = (getDisplayInfo_t)dlsym(RTLD_NEXT,
            "_ZN7android21SurfaceComposerClient14getDisplayInfoERKNS_2spINS_7IBinderEEEPNS_11DisplayInfoE");
        if (!real) {
            __android_log_print(ANDROID_LOG_ERROR, TAG,
                                "cannot resolve real getDisplayInfo: %s", dlerror());
            return -1;
        }
    }

    uint8_t tmp[256];
    memset(tmp, 0, sizeof(tmp));
    int32_t rc = real(display, tmp);
    memcpy(info, tmp, DISPLAYINFO_O_SIZE);
    return rc;
}

// Batch size. Pico asks for small counts; anything larger is chunked so we never
// need a variable-length array on the stack.
#define CHUNK 16

typedef ssize_t (*getEvents_t)(void* self, struct EventQ* events, size_t count);

ssize_t _ZN7android20DisplayEventReceiver9getEventsEPNS0_5EventEm(
        void* self, struct EventO* out, size_t count) {
    static getEvents_t real = NULL;
    if (!real) {
        real = (getEvents_t)dlsym(RTLD_NEXT,
                "_ZN7android20DisplayEventReceiver9getEventsEPNS0_5EventEm");
        if (!real) {
            __android_log_print(ANDROID_LOG_ERROR, TAG,
                                "cannot resolve real getEvents: %s", dlerror());
            return -1;
        }
    }

    if (count > CHUNK) count = CHUNK;   // clamp; caller loops for the rest

    struct EventQ tmp[CHUNK];
    ssize_t n = real(self, tmp, count);
    if (n <= 0) return n;
    if ((size_t)n > count) n = (ssize_t)count;   // defensive

    for (ssize_t i = 0; i < n; ++i) {
        out[i].type      = tmp[i].type;
        out[i].id        = (uint32_t)tmp[i].displayId;  // one internal display
        out[i].timestamp = tmp[i].timestamp;
        out[i].u.count   = tmp[i].u.count;              // union is 4 bytes both ways
    }
    return n;
}
