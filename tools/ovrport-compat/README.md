# ovrport-compat

Runtime shim for OVRPort-patched Oculus/Unity apps running on hsvr/Monado.

OVRPort's libOVRPlugin has two gaps for pre-2019 Unity titles:

1. `ovrp_BeginFrame` only submits a frame after an internal wait ran, but
   Unity <2019 never calls `ovrp_WaitToBeginFrame`, so every frame silently
   no-ops (black screen). The shim chains the wait (vtable slot 44) inside
   the begin slot (45).
2. `ovrp_SetupEyeTexture2` returns success for `handle=0`. The legacy
   contract requires it to fail so Unity retries with its own GL texture
   name, which the plugin then gives storage.
3. `EndEye2` is a stub, and OVRPort never creates a projection swapchain
   for this path. Each `xrEndFrame` only carries two quad layers backed
   by a 1700x512 chain (the splash panel), so eye content never reaches
   the display. The shim creates its own 1728x1536x2 swapchain via the
   runtime's real `xrCreateSwapchain`, acquires/waits/blits/releases it
   inside `EndEye2`, then rewrites `xrEndFrame` to prepend an
   `XrCompositionLayerProjection` whose view poses/FOVs come from
   `xrLocateViews` on the quad layer's space.

The shim is a `DT_NEEDED` dependency injected into `libOVRPlugin.so` inside
the target apk, so no smali changes are needed. It only activates on the
libOVRPlugin build it recognises (vtable signature check); other builds are
left untouched.

## Build

    ./build.sh            # needs an Android NDK, set ANDROID_NDK_HOME
    pip install lief      # for inject_apk.py

## Inject into an apk

    ./inject_apk.py game-ovrport.apk out.apk --zipalign /path/to/zipalign
    apksigner sign --ks your.keystore out.apk

or pass signing straight through:

    ./inject_apk.py in.apk out.apk --ks ks.jks --ks-pass pass:...

Logs appear under the `hsvrovrp` logcat tag.
