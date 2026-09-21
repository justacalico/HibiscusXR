# Addendum: linker whitelist + 4 missing libraries

To be merged into `overlay/PROPRIETARY-PVR.md`. Same rule as the rest: every
proprietary Pico binary the port depends on is mapped here so the OS can ship
without it and the end user restores it from their own device.

## The whitelist (non-proprietary, ours to ship)

`/system/etc/public.libraries.txt`

The GSI ships only the AOSP list. Stock appends 22 Pico entries. On Android 8+
this file is what allows an app's linker namespace to `dlopen` a non-public
`/system` library; without it every Pico VR library load from an app returns
null, and Pico's code calls the result without checking. VRShell died calling
`pvrVirtualInputCreate` at `pc = 0x0` for exactly this reason.

This file is a plain text list, not a Pico binary, so we can generate it
ourselves. The 22 names to append are listed below.

## Libraries that were missing from the blob set

These four were never copied in. Extract from a stock Pico Neo 2 (PUI 4.1.3),
both ABIs, into the matching paths.

| Library | lib64 | lib | What it is |
|---|---:|---:|---|
| `libvirtualinputclient.so` | 40336 | 37512 | **Defines `pvrVirtualInputCreate`** (2 string hits vs `lib2dToVr.so`'s 1 reference). The 2D-app-in-VR virtual input bridge. Its absence is what made VRShell call a null pointer. |
| `libairclient.so` | 73232 | 62636 | Pico "air" / streaming client transport. |
| `libSafetyArea.so` | 5793816 | 3403728 | Guardian / play-area boundary rendering. |
| `libImageGrid.so` | 4554544 | 2681408 | Image grid compositing used by the shell. |

## The 22 whitelist entries

```
libpvrserviceclient.so
libvirtualinputclient.so
libpxrserviceclient.so
libairclient.so
libSafetyArea.so
libImageGrid.so
libPvr_UnitySDK.so
libPvr_UnitySDKExt1.so
libPvr_UnitySDKExt5.so
libPvr_UnitySDKExt8.so
libPvr_UnitySDKExt9.so
libPvr_UnitySDKExt10.so
libPvr_UnitySDKExt11.so
libPvr_UESDKExt2.so
libCVControllerClient.pxr.so
lib6DofReset.so
libpxrnotification.pxr.so
libconfigurationclient.pxr.so
libplugin.pxr.so
libloader.pxr.so
libruntime.pxr.so
libcompositor.pxr.so
```

Note `lib6DofReset.so` is whitelisted by stock but does not exist on either
device - stock logs `Open library<lib6DofReset.so> failed` too. Harmless; keep
the entry so the list matches stock exactly.
