# ovrport-compat

Runtime shim for OVRPort-patched Oculus/Unity apps running on hsvr/Monado.

OVRPort's libOVRPlugin has two gaps for pre-2019 Unity titles:

1. The game never drives the XR frame loop on this path: vtable slot 44
   (the internal wait/begin) is never called by Unity <2019, so
   `xrWaitFrame`/`xrBeginFrame`/`xrEndFrame` never run and the compositor
   gets zero layers (black screen). The shim invokes slot 44 once from the
   begin slot to capture the `XrSession`, then drives a full
   wait/begin/locate/submit/end cycle itself inside `EndEye2`.
2. `ovrp_SetupEyeTexture2` returns success for `handle=0`. The legacy
   contract requires it to fail so Unity retries with its own GL texture
   name, which the plugin then gives storage.
3. `EndEye2` is a stub, and OVRPort never creates a projection swapchain
   for this path. The game's own `xrEndFrame` only carries two quad layers
   backed by a 1700x512 chain (the splash panel), so eye content never
   reaches the display. The shim creates two 1728x1536 eye swapchains via
   the runtime's real `xrCreateSwapchain`, acquires/waits/blits/releases
   them inside `EndEye2` (layer 0/1 of the stereo texture array), forces
   alpha to 1, then submits an `XrCompositionLayerProjection` with poses
   and FOVs from `xrLocateViews` on a LOCAL reference space it creates.

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
