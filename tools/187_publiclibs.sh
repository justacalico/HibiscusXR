#!/system/bin/sh
# Do we actually HAVE every library stock whitelists, and is libvirtualinputclient
# (the one that should own pvrVirtualInput*) present at all?
LIBS="libpvrserviceclient.so libvirtualinputclient.so libpxrserviceclient.so
libairclient.so libSafetyArea.so libImageGrid.so libPvr_UnitySDK.so
libPvr_UnitySDKExt1.so libPvr_UnitySDKExt5.so libPvr_UnitySDKExt8.so
libPvr_UnitySDKExt9.so libPvr_UnitySDKExt10.so libPvr_UnitySDKExt11.so
libPvr_UESDKExt2.so libCVControllerClient.pxr.so lib6DofReset.so
libpxrnotification.pxr.so libconfigurationclient.pxr.so libplugin.pxr.so
libloader.pxr.so libruntime.pxr.so libcompositor.pxr.so lib2dToVr.so"

echo "=== presence check (64 / 32) ==="
for l in $LIBS; do
  a=""; b=""
  [ -f "/system/lib64/$l" ] && a="64"
  [ -f "/system/lib/$l" ]   && b="32"
  if [ -z "$a" ] && [ -z "$b" ]; then echo "  MISSING      $l"; else echo "  present $a$b  $l"; fi
done
echo
echo "=== who exports pvrVirtualInputCreate ==="
for f in /system/lib64/lib2dToVr.so /system/lib64/libvirtualinputclient.so; do
  [ -f "$f" ] && echo "  $f: $(strings $f 2>/dev/null | grep -c pvrVirtualInputCreate) hits"
done
echo DONE
