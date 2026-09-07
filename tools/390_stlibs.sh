#!/system/bin/sh
# x28=0, fault addr 0x10, pc==lr on a binder thread: the same AArch64 x28 problem
# already patched in VRShell's libPvr_UnitySDK.so. The see-through app ships its
# own copy of the Pico SDK, so it needs the same treatment.
echo "=== see-through app native libs ==="
ls -l /system/priv-app/seethroughsetting/lib/arm64/ 2>/dev/null || \
  find /system/priv-app -ipath '*seethrough*' -name '*.so' 2>/dev/null | head -20

echo
echo "=== which SDK copies exist and are they the patched one? ==="
echo "  patched VRShell SDK = 120a6620340083fccde87c9591f62c63"
for f in $(find /system/priv-app -name 'libPvr_UnitySDK*.so' 2>/dev/null); do
  echo "  $(md5sum $f)"
done

echo
echo "=== which of them did the crashed process map? ==="
logcat -d | grep -iE 'open replaceable|libPvr_UnitySDKExt|VrApi.*version' | tail -10

echo
echo "=== does seethrough load Ext11? ==="
logcat -d | grep -iE 'Ext11|replaceable' | tail -6
