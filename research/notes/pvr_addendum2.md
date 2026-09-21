
---

# Session 2 additions

Everything below is proprietary Pico content extracted from a stock Pico Neo 2
(PUI 4.1.3) unless marked otherwise. Same rule as the rest of this file: the OS
can ship without any of it, and an end user restores it from their own device.

## Files added to /system

| Path | Size | Origin | Why |
|---|---:|---|---|
| `/bin/airservice` | 44336 | stock `/system/bin` | Passthrough camera service, publishes binder `pvr.IAIRService`. The see-through app blocks forever on it. |
| `/bin/virtual_input` | 28288 | stock `/system/bin` | Daemon behind `libvirtualinputclient.so`; creates the `pvr-virtual-input-0..4` evdev nodes. |
| `/lib64/pvr_air/libairservice.so` | 172040 | stock `/system/lib64` | airservice implementation. |
| `/lib64/pvr_air/libaircamera.so` | 56704 | stock `/system/lib64` | Passthrough camera path. |
| `/lib64/libvirtualinput.so` (+32) | 31472 / 28808 | stock | virtual_input implementation. |
| `/lib64/libvirtualinputclient.so` (+32) | 40336 / 37512 | stock | **Defines `pvrVirtualInputCreate`**. Its absence made VRShell call a null pointer. |
| `/lib64/libairclient.so` (+32) | 73232 / 62636 | stock | airservice client. |
| `/lib64/libSafetyArea.so` (+32) | 5793816 / 3403728 | stock | Guardian / play-area boundary. |
| `/lib64/libImageGrid.so` (+32) | 4554544 / 2681408 | stock | Image grid compositing. |
| `/lib64/libdatabuffer.so` (+32) | 15064 / 20320 | stock | Dependency of libairclient/airservice. |
| `/priv-app/seethroughsetting/` | 203 MB + 7 libs | stock `/system/priv-app` | See-through (passthrough) calibration app, `com.pvr.seethrough.setting` 1.1.102. Re-signed with our platform key because it declares `sharedUser=android.uid.system`. Not odexed, so no deodex needed. |

## Files we ship ourselves (not proprietary)

| Path | What |
|---|---|
| `/etc/public.libraries.txt` | Merged list: the GSI's AOSP entries plus stock's 22 Pico entries. Plain text, generated, not a Pico binary. **Without it every Pico `dlopen` from an app returns null.** |
| `/lib64/pvr_air/libskia.so` | **Empty stub we compiled.** `libaircamera.so` lists libskia in DT_NEEDED but imports zero symbols from it. The real 8.1 libskia drags ICU 60, which conflicts irreconcilably with Q's ICU 63 in the same process. |
| `/lib64/pvr_air/libshim_air.so` | Our shim: forwards `BufferItemConsumer::setName` → `ConsumerBase::setName`, `OutputConfiguration(gbp,int,int)` → Q's longer ctor, `tinyxml2::XMLDocument(bool)` → `XMLDocument(bool,Whitespace)`, implements `Fence::~Fence`, and **stubs** `getLockedImageInfo` / `lockImageFromBuffer` (libgui internals deleted in Q). The two stubs log a warning; if the passthrough frame path is ever exercised it will fail visibly rather than silently. |
| `/lib64/libshim_pvr.so` | Existing shim, now also guarding `__android_log_print` against a null format (pvrservice passes its format in x28, which does not survive a virtual dispatch). |
| `/etc/init/pn2-airservice.rc` | Starts airservice + virtual_input. Isolates the 8.1 chain via `LD_LIBRARY_PATH=/system/lib64/pvr_air`. |
| `/etc/init/pn2-qvrd.rc` | Starts Qualcomm's `qvrservice` as **`pn2_qvrd`**. Vendor init defines `qvrd` without a `seclabel`, so init refuses it; a corrected block under the same name is discarded as a duplicate, hence the rename. Socket names are unchanged, which is what clients actually use. |
| `/etc/init/pn2-adbwifi.rc` | Optional wireless adb, off by default (`persist.pn2.adbwifi=1` to enable). |

## Binary patches (ours, applied to proprietary files)

| File | Patch | Why |
|---|---|---|
| `/apex/com.android.runtime.release/lib64/libart.so` | `mov sp, x28` → `mov sp, x29` at `art_quick_generic_jni_trampoline+172` | Android 10 parks the caller's sp in x28 across a JNI native call; x28 comes back 0 and sp becomes null. x29 holds the same frame base and survives. **Proven necessary**: on stock ART the original crash reproduces exactly. |
| `/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so` | `ldr x0,[x28,#8]` at `pvr_EnterVrMode+1336` redirected to a trampoline doing `sub x28, x27, #0x148` first | Same x28 loss. x27 was set to `struct + 0x148` from the same allocation and survives, so the base is recomputed from it. |

## Deliberately NOT included

- **`libPvr_UnitySDKExt11.so` x28 patch.** Same fix as VRShell's, and the see-through
  app needs it, but the code cave the generic patcher chose landed inside
  `PVR::BufferedFile::~BufferedFile` and hung VRShell (60 threads → 23). Needs a
  cave verified to be genuine dead space between functions, the way the VRShell
  one was.
- **CVService ABI correction.** Lives in `/data/system/packages.xml`, not `/system`.
  PackageManager cached `arm64-v8a` because it first scanned CVService before
  `lib/arm` was restored. On a fresh flash it scans with the libs present and
  derives `armeabi-v7a` itself.
- **8.1 `libtinyxml2.so` / `libicuuc.so` / `libicui18n.so`.** Shadowing these breaks
  Q components in the same process (`libvintf` needs the newer tinyxml2 ctor;
  `libandroidicu` needs ICU 63). Handled by forwarding/stubbing instead.
