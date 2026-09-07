// libgui/libui ABI shims for the 8.1 passthrough camera stack (libaircamera.so).
//
// airservice is what the see-through calibration app blocks on:
//     ServiceManager: Waiting for service 'airservice'
// It links libaircamera.so, which references five android:: symbols Android 10 no
// longer exports. Preloaded into airservice only, via its init entry.
//
//   BufferItemConsumer::setName  - moved up to ConsumerBase in Q. Single
//                                  inheritance with ConsumerBase as the primary
//                                  base, so `this` is directly compatible.
//   OutputConfiguration(gbp,int,int)
//                                - Q's ctor gained a physicalCameraId String16
//                                  and an isShared flag. Forward with defaults.
//   Fence::~Fence                - not exported at all in Q. Fence is a
//                                  LightRefBase (no vtable): atomic<int32> mCount
//                                  then unique_fd mFenceFd, so the fd sits at +4.
//                                  Close it so we do not leak descriptors.
//   getLockedImageInfo           - libgui internals, deleted in Q (CpuConsumer::
//   lockImageFromBuffer            lockBufferItem replaced them). Stubbed: the
//                                  goal is to let airservice start and publish its
//                                  binder service. If the camera frame path is
//                                  actually exercised these return failure, which
//                                  shows up as a clear error instead of a hang.
//
// Stubs are deliberate and logged, so a "why is passthrough black" question has an
// obvious answer in logcat rather than looking like a silent success.

#include <android/log.h>
#include <unistd.h>
#include <stdint.h>

#define TAG "shim_air"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

namespace android {
class String8;
class String16;
class BufferItem;
class CpuConsumer;
class IGraphicBufferProducer;
template <typename T> class sp;
}

extern "C" {

// ---- ConsumerBase::setName, the Q home of BufferItemConsumer::setName --------
void _ZN7android12ConsumerBase7setNameERKNS_7String8E(void *self, const void *name);

void _ZN7android18BufferItemConsumer7setNameERKNS_7String8E(void *self, const void *name)
{
    _ZN7android12ConsumerBase7setNameERKNS_7String8E(self, name);
}

// ---- OutputConfiguration -----------------------------------------------------
// 8.1: (sp<IGraphicBufferProducer>&, int rotation, int surfaceSetID)
// Q  : (sp<IGraphicBufferProducer>&, int rotation, const String16& physicalCameraId,
//       int surfaceSetID, bool isShared)
void _ZN7android8String16C1EPKc(void *self, const char *s);
void _ZN7android8String16D1Ev(void *self);
void _ZN7android8hardware7camera26params19OutputConfigurationC1ERNS_2spINS_22IGraphicBufferProducerEEEiRKNS_8String16Eib(
        void *self, void *gbp, int rotation, const void *physicalCameraId,
        int surfaceSetID, bool isShared);

void _ZN7android8hardware7camera26params19OutputConfigurationC1ERNS_2spINS_22IGraphicBufferProducerEEEii(
        void *self, void *gbp, int rotation, int surfaceSetID)
{
    // String16 is a single pointer; build an empty one for physicalCameraId
    void *empty[2] = { nullptr, nullptr };
    _ZN7android8String16C1EPKc(&empty, "");
    _ZN7android8hardware7camera26params19OutputConfigurationC1ERNS_2spINS_22IGraphicBufferProducerEEEiRKNS_8String16Eib(
            self, gbp, rotation, &empty, surfaceSetID, false);
    _ZN7android8String16D1Ev(&empty);
}

// ---- tinyxml2::XMLDocument -----------------------------------------------------
// 8.1: XMLDocument(bool processEntities)
// Q  : XMLDocument(bool processEntities, Whitespace whitespaceMode)
//
// Shipping 8.1's libtinyxml2.so alongside does NOT work: libvintf.so is a Q
// library in the same process and needs the two-argument ctor, so whichever copy
// wins, the other side fails to link. Keep Q's libtinyxml2 for everyone and
// provide the old entry point here, forwarding with PRESERVE_WHITESPACE (0),
// which is what the one-argument form defaulted to.
void _ZN8tinyxml211XMLDocumentC1EbNS_10WhitespaceE(void *self, bool processEntities, int whitespace);
void _ZN8tinyxml211XMLDocumentC2EbNS_10WhitespaceE(void *self, bool processEntities, int whitespace);

void _ZN8tinyxml211XMLDocumentC1Eb(void *self, bool processEntities)
{
    _ZN8tinyxml211XMLDocumentC1EbNS_10WhitespaceE(self, processEntities, 0);
}

void _ZN8tinyxml211XMLDocumentC2Eb(void *self, bool processEntities)
{
    _ZN8tinyxml211XMLDocumentC2EbNS_10WhitespaceE(self, processEntities, 0);
}

// ---- Fence::~Fence -----------------------------------------------------------
// LightRefBase<Fence> has no vtable: mCount (atomic<int32_t>) at +0, then
// unique_fd mFenceFd at +4. Closing it keeps descriptor use bounded; getting this
// wrong leaks rather than corrupts, which is the safer failure direction.
void _ZN7android5FenceD1Ev(void *self)
{
    if (!self) return;
    int32_t *fd = reinterpret_cast<int32_t *>(reinterpret_cast<char *>(self) + 4);
    if (*fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

// ---- CpuConsumer helpers deleted in Q ---------------------------------------
// Both returned status_t (0 = OK). Return an error so callers take their failure
// path rather than reading an uninitialised LockedBuffer.
static void warn_once(const char *what)
{
    static const char *last = nullptr;
    if (last == what) return;
    last = what;
    LOGW("%s is stubbed: Android 10 removed it from libgui. "
         "airservice can start, but the passthrough frame path will not deliver images.", what);
}

int32_t _ZN7android18getLockedImageInfoEPNS_11CpuConsumer12LockedBufferEiiPPhPjPiS6_(
        void *lockedBuffer, int a, int b, unsigned char **c, unsigned int *d, int *e, unsigned int *f)
{
    (void)lockedBuffer; (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    warn_once("getLockedImageInfo");
    return -38;   // -ENOSYS
}

int32_t _ZN7android19lockImageFromBufferEPNS_10BufferItemEjiPNS_11CpuConsumer12LockedBufferE(
        void *bufferItem, unsigned int usage, int fenceFd, void *lockedBuffer)
{
    (void)bufferItem; (void)usage; (void)fenceFd; (void)lockedBuffer;
    warn_once("lockImageFromBuffer");
    return -38;   // -ENOSYS
}

} // extern "C"
