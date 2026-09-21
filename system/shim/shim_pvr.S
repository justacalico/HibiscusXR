// Shim for libpvrmodule_platform.so on Android 10.
//
// Pico's platform module is built against the Android 8.1 framework and imports
// two libgui symbols that Android 10 removed. Everything else in the PVR stack
// (libcompositor.pxr.so, libruntime.pxr.so, libpxr_6dof_optimization.so, ...)
// imports nothing Q dropped - this one library is the whole ABI problem.
//
// Written in assembly on purpose. Both symbols have C++ ABI details that a C
// prototype cannot express correctly on AArch64:
//   - getBuiltInDisplay returns sp<IBinder>, which is non-trivially copyable and
//     so is returned through the hidden pointer in x8, not x0. Declaring it in C
//     as returning a struct would use the wrong convention and corrupt memory.
//   - the DisplayEventReceiver ctor takes `this` in x0.
// A tail branch preserves x8, x0 and every argument register exactly as the
// caller set them, which is precisely what we want.

    .text

// android::SurfaceComposerClient::getBuiltInDisplay(int)
//   -> android::SurfaceComposerClient::getInternalDisplayToken()   for id == 0
//   -> null sp<IBinder>                                            otherwise
//
// Removed in Q. The 8.1 API took a display id: 0 is the internal panel, 1 is
// external/HDMI, and it returns null when that display is absent.
//
// The first version of this shim forwarded unconditionally and handed back the
// INTERNAL display token for every id. Anything probing for a second display got
// a valid token for a display that does not exist, and
// PVR::Platform::getDisplayInfo() then crashed on the bogus handle
// (SIGSEGV, fault addr 0x87000000f00). Honour the id.
//
// sp<IBinder> is returned through the hidden pointer in x8, and AArch64 requires
// x0 to hold that same address on return, hence the mov on the null path.
    .globl  _ZN7android21SurfaceComposerClient17getBuiltInDisplayEi
    .type   _ZN7android21SurfaceComposerClient17getBuiltInDisplayEi, %function
_ZN7android21SurfaceComposerClient17getBuiltInDisplayEi:
    cbnz    w0, .Lno_such_display
    b       _ZN7android21SurfaceComposerClient23getInternalDisplayTokenEv
.Lno_such_display:
    str     xzr, [x8]          // sp<IBinder>(nullptr)
    mov     x0, x8
    ret
    .size   _ZN7android21SurfaceComposerClient17getBuiltInDisplayEi, .-_ZN7android21SurfaceComposerClient17getBuiltInDisplayEi

// android::DisplayEventReceiver::DisplayEventReceiver(ISurfaceComposer::VsyncSource)
//   -> DisplayEventReceiver(ISurfaceComposer::VsyncSource,
//                           ISurfaceComposer::ConfigChanged)
//
// Q added a second parameter. eConfigChangedSuppress == 0 reproduces the old
// one-argument behaviour: do not deliver config-change events, which is what a
// caller written against 8.1 expects since it has no handler for them.
//   x0 = this, w1 = vsyncSource (both already in place), w2 = ConfigChanged
    .globl  _ZN7android20DisplayEventReceiverC1ENS_16ISurfaceComposer11VsyncSourceE
    .type   _ZN7android20DisplayEventReceiverC1ENS_16ISurfaceComposer11VsyncSourceE, %function
_ZN7android20DisplayEventReceiverC1ENS_16ISurfaceComposer11VsyncSourceE:
    mov     w2, #0
    b       _ZN7android20DisplayEventReceiverC1ENS_16ISurfaceComposer11VsyncSourceENS1_13ConfigChangedE
    .size   _ZN7android20DisplayEventReceiverC1ENS_16ISurfaceComposer11VsyncSourceE, .-_ZN7android20DisplayEventReceiverC1ENS_16ISurfaceComposer11VsyncSourceE

    .section .note.GNU-stack,"",%progbits
