#!/system/bin/sh
# Is a universal fix possible? Under AAPCS x19-x28 are callee-saved, so ART must
# restore x28 across the JNI boundary. If it does not, one libart patch fixes
# every Pico app instead of patching each app's own SDK copy.
echo "=== which libart is live, and is it the patched one? ==="
ls -l /apex/com.android.runtime/lib64/libart.so
md5sum /apex/com.android.runtime/lib64/libart.so
echo "  (compare against notes/libart-patched.so)"

echo
echo "=== is our ART patch actually applied? ==="
echo "  patch site 0x13f36c: 0x910003BF = mov sp,x29 (patched) / 0x9100039F = mov sp,x28 (stock)"

echo
echo "=== every app that ships its own Pico SDK ==="
for f in $(find /system/priv-app /system/app -name 'libPvr_UnitySDK*.so' 2>/dev/null); do
  echo "  $(md5sum $f)"
done

echo
echo "=== and their SDK versions, from whatever has run ==="
logcat -d 2>/dev/null | grep -E 'VrApi.*(APP|lib) version' | sort -u | tail -8
