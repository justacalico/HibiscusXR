#!/system/bin/sh
# Which library exports pvrVirtualInputCreate?
#
# VRShell dies calling a null function pointer, and the registers at the fault
# spell "pvrVirtu" / "alInputC" / "utCreate" -> pvrVirtualInputCreate. That is a
# dlsym returning NULL and the result being called unchecked, i.e. a missing
# export rather than a logic error. Find who provides it (if anyone).
echo "=== libs whose contents mention pvrVirtualInput ==="
grep -l pvrVirtualInput /system/lib64/*.so 2>/dev/null
grep -l pvrVirtualInput /system/lib/*.so 2>/dev/null
echo
echo "=== app-bundled libs ==="
grep -l pvrVirtualInput /system/priv-app/VRShell2/lib/arm64/*.so 2>/dev/null
echo
echo "=== every pvrVirtualInput* string found, with its file ==="
for f in /system/lib64/*.so /system/priv-app/VRShell2/lib/arm64/*.so; do
  s=$(strings "$f" 2>/dev/null | grep -o 'pvrVirtualInput[A-Za-z_]*' | sort -u | tr '\n' ' ')
  [ -n "$s" ] && echo "  $f : $s"
done
echo
echo "=== is there any Ext lib we are missing vs what apps ask for? ==="
ls /system/lib64/libPvr_UnitySDKExt*.so 2>/dev/null
echo "--- names apps reference ---"
for f in /system/priv-app/VRShell2/lib/arm64/*.so /system/lib64/libPvr_UnitySDK.so; do
  strings "$f" 2>/dev/null | grep -oE 'libPvr_UnitySDKExt[0-9]+\.so' | sort -u | sed "s|^|  $(basename $f): |"
done
echo DONE
