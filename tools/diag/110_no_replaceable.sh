#!/system/bin/sh
# VRShell loads TWO copies of the Pico Unity SDK:
#   /system/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so   (app-private, 2051960)
#   /system/lib64/libPvr_UnitySDKExt11.so                    ("replaceable", 2047864)
# Both are mapped in the tombstone. That is Pico's design - the system copy is
# meant to override the app's - but if the pairing is wrong the JNI frame the
# generic trampoline builds could be garbage, which matches sp=0.
#
# Hide the replaceable one and see whether PicoVRInit gets further using only the
# app's own SDK. Reversible: it is renamed, not deleted.
exec 2>&1
case "$1" in
  hide)
    mount -o rw,remount /system
    for a in lib64 lib; do
      for v in Ext11 Ext10; do
        [ -f /system/$a/libPvr_UnitySDK$v.so ] && \
          mv /system/$a/libPvr_UnitySDK$v.so /system/$a/libPvr_UnitySDK$v.so.hidden && \
          echo "hid /system/$a/libPvr_UnitySDK$v.so"
      done
    done
    ;;
  restore)
    mount -o rw,remount /system
    for a in lib64 lib; do
      for v in Ext11 Ext10; do
        [ -f /system/$a/libPvr_UnitySDK$v.so.hidden ] && \
          mv /system/$a/libPvr_UnitySDK$v.so.hidden /system/$a/libPvr_UnitySDK$v.so && \
          echo "restored /system/$a/libPvr_UnitySDK$v.so"
      done
    done
    ;;
  *) echo "usage: $0 hide|restore"; exit 1 ;;
esac
sync
mount -o ro,remount /system
echo DONE
