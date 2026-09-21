# shim

[HibiscusXR/system/shim](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/shim)

Source for the ABI shim libraries - the bridge between Pico's Android 8.1
binaries and the Android 10 framework.

## `shim_pvr.S` -> `libshim_pvr.so`

Shim for `libpvrmodule_platform.so` - the one PVR library that imports symbols
Android 10 dropped (everything else in the stack imports nothing Q removed).
Written in assembly on purpose: `getBuiltInDisplay` returns `sp<IBinder>`,
which is non-trivially copyable and returned through the hidden `x8` pointer,
not `x0` - a C prototype would use the wrong convention and corrupt memory. A
tail branch preserves `x8`, `x0` and all argument registers exactly as the
caller set them.

Covers `getBuiltInDisplay` (-> `getInternalDisplayToken` for id 0, null
otherwise) and the `DisplayEventReceiver` ctor growth.

## `shim_air.cpp` -> `libshim_air.so`

libgui/libui ABI shims for the see-through camera stack (`libaircamera.so`,
which `airservice` blocks on). Forwards five android:: symbols Q no longer
exports - `BufferItemConsumer::setName`, the `OutputConfiguration` ctor that
gained a `physicalCameraId`, the unexported `Fence` destructor, and friends.
Preloaded into airservice only, via its init entry.

## `stub_skia.c` -> `libskia_stub.so`

Minimal skia stub for components that import symbols the GSI skia lacks.

Build via the `build*.sh` scripts in the repo (NDK/LLVM toolchain, see
`NDK_BIN` env var).
