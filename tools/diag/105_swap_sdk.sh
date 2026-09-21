#!/system/bin/sh
# VRShell's PicovrSDK class does System.loadLibrary("Pvr_UnitySDK"), which
# resolves to libPvr_UnitySDK.so - but that file is byte-identical to
# libPvr_UnitySDKExt1.so and exports only 3 psmart JNI symbols, not
# isJavaService. Only Ext10 (57 symbols) and Ext11 (63) export it, so VRShell was
# built against one of those.
#
# Swap the base lib for the requested variant and see which one lets it start.
# Original kept as .orig so this is reversible.
VAR=${1:-Ext11}
exec 2>&1
mount -o rw,remount /system
for a in lib64 lib; do
  [ -f /system/$a/libPvr_UnitySDK.so.orig ] || \
    cp /system/$a/libPvr_UnitySDK.so /system/$a/libPvr_UnitySDK.so.orig
  if [ -f /system/$a/libPvr_UnitySDK$VAR.so ]; then
    cp -f /system/$a/libPvr_UnitySDK$VAR.so /system/$a/libPvr_UnitySDK.so
    chmod 644 /system/$a/libPvr_UnitySDK.so
    chown root:root /system/$a/libPvr_UnitySDK.so
    echo "$a: libPvr_UnitySDK.so <- $VAR ($(stat -c%s /system/$a/libPvr_UnitySDK.so) bytes)"
  else
    echo "$a: libPvr_UnitySDK$VAR.so not present"
  fi
done
sync
mount -o ro,remount /system
echo DONE
