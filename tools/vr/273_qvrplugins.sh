#!/system/bin/sh
# The QVR "plugins" are these driver stubs. Ours loads none of them, hence
# "Plugin not valid" and no tracking at all. Are they even on our filesystem?
LIBS="libqvr_cdsp_driver_stub.so libqvr_cam_cdsp_driver_stub.so libqvr_mapper_stub.so
libtobii_runtime.so libtobii_eyecore_stub.so libqti-perfd-client_system.so
libqvrservice.so libqvrcamera_client_system.so libui.so libbacktrace.so liblzma.so
android.hardware.configstore@1.0.so android.hardware.configstore-utils.so"
echo "=== presence (32-bit /system/lib is what qvrservice uses) ==="
for l in $LIBS; do
  a=""; b=""
  [ -f "/system/lib/$l" ]   && a="32"
  [ -f "/system/lib64/$l" ] && b="64"
  if [ -z "$a" ] && [ -z "$b" ]; then echo "  MISSING   $l"; else echo "  have $a$b   $l"; fi
done
echo
echo "=== also check vendor (untouched) ==="
for l in $LIBS; do
  [ -f "/vendor/lib/$l" ] && echo "  vendor32  $l"
done
echo
echo "=== qvrservice config that names plugins ==="
for f in /vendor/etc/qvr/* /system/etc/qvr/* /vendor/etc/qvrservice* ; do
  [ -f "$f" ] && echo "--- $f ---" && head -30 "$f"
done 2>/dev/null
