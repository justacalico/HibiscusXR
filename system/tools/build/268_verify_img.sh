#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Independent verification: extract key files back out of the image and hash them
# against the sources. The write log said OK, but debugfs has silently refused
# overwrites before, so confirm content rather than trusting the report.
set -u
IMG=${PN2_ROOT}/out/system-pn2-full.img
T=$(mktemp -d)
LOG=${PN2_ROOT}/notes/268_verify.txt
exec >"$LOG" 2>&1

check() {  # check <img-path> <local>
  local d="$1" s="$2"
  debugfs -R "dump $d $T/x" "$IMG" >/dev/null 2>&1
  local a b
  a=$(md5sum "$T/x" 2>/dev/null | cut -d' ' -f1)
  b=$(md5sum "$s"   2>/dev/null | cut -d' ' -f1)
  if [ -n "$a" ] && [ "$a" = "$b" ]; then printf '  MATCH  %s\n' "$d"
  else printf '  DIFFER %s  (img=%s src=%s)\n' "$d" "${a:-none}" "${b:-none}"; fi
}

echo "=== content verification ==="
check /etc/public.libraries.txt              ${PN2_ROOT}/overlay_pvr/public.libraries.txt
check /lib64/libvirtualinputclient.so        ${PN2_ROOT}/overlay_pvr/lib64/libvirtualinputclient.so
check /lib64/libSafetyArea.so                ${PN2_ROOT}/overlay_pvr/lib64/libSafetyArea.so
check /bin/airservice                        ${PN2_ROOT}/airsvc/bin/airservice
check /bin/virtual_input                     ${PN2_ROOT}/airsvc/bin/virtual_input
check /lib64/pvr_air/libshim_air.so          ${PN2_ROOT}/shim/libshim_air.so
check /lib64/pvr_air/libskia.so              ${PN2_ROOT}/shim/libskia_stub.so
check /lib64/libshim_pvr.so                  ${PN2_ROOT}/shim/libshim_pvr.so
check /apex/com.android.runtime.release/lib64/libart.so ${PN2_ROOT}/notes/libart-patched.so
check /priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so   ${PN2_ROOT}/notes/vrshell_lib/libPvr_UnitySDK.patched2.so
check /etc/init/pn2-qvrd.rc                  ${PN2_ROOT}/overlay/etc/init/pn2-qvrd.rc
# apks get re-signed with the platform key in 267, so the image copy never
# md5-matches the staged source - verify the signature instead
debugfs -R "dump /priv-app/seethroughsetting/seethroughsetting.apk $T/st.apk" "$IMG" >/dev/null 2>&1
BT=$(ls -d "${ANDROID_SDK_ROOT:-/opt/android-sdk}"/build-tools/* | sort -V | tail -1)
"$BT/apksigner" verify --print-certs "$T/st.apk" 2>/dev/null | grep -q "CN=PN2" \
  && printf '  MATCH  %s\n' "/priv-app/seethroughsetting/seethroughsetting.apk (CN=PN2)" \
  || printf '  DIFFER %s\n' "/priv-app/seethroughsetting/seethroughsetting.apk (not PN2-signed)"

echo
echo "=== whitelist really has the 22 Pico entries? ==="
debugfs -R "dump /etc/public.libraries.txt $T/pl" "$IMG" >/dev/null 2>&1
echo "  total .so entries: $(grep -c '\.so$' "$T/pl")"
grep -c -E 'libPvr_UnitySDK|pxr\.so|libvirtualinputclient' "$T/pl" | sed 's/^/  pico entries: /'

echo
echo "=== the ART patch is actually the patched byte? ==="
debugfs -R "dump /apex/com.android.runtime.release/lib64/libart.so $T/art" "$IMG" >/dev/null 2>&1
# mov sp,x29 = bf 03 00 91 at file offset 0x13f36c
od -An -tx1 -j $((0x13f36c)) -N4 "$T/art" | sed 's/^/  bytes at 0x13f36c: /'
echo "  (expect bf 03 00 91 = mov sp, x29; the original was 9f 03 00 91)"

echo
echo "=== OpenXR chain ==="
# the whole path wivrn/hello_xr takes: manifest -> staged runtime -> turnip.
# presence-only checks for the apk (re-signed); content checks for the rest
XR=${PN2_ROOT}/pn2xr
check /etc/openxr/1/active_runtime.json            "$XR/android/active_runtime.json"
check /product/etc/openxr/1/active_runtime.json    "$XR/android/active_runtime.json"
check /etc/init/pn2-openxr.rc                      ${PN2_ROOT}/overlay/etc/init/pn2-openxr.rc
check /etc/init/pn2-vulkan.rc                      ${PN2_ROOT}/overlay/etc/init/pn2-vulkan.rc
check /etc/permissions/pn2-xr-features.xml         ${PN2_ROOT}/overlay/etc/permissions/pn2-xr-features.xml
check /lib64/hw/vulkan.sdm845.so                   "$XR/turnip/out/libvulkan_freedreno.so"
check /lib64/libc++_shared.so                      "$XR/turnip/out/libc++_shared.so"
# turnip loads inside the sphal namespace (anything under /vendor/lib64);
# without libc++_shared in its default link the driver dies at dlopen
debugfs -R "dump /etc/ld.config.27.txt $T/ldcfg" "$IMG" >/dev/null 2>&1
grep -q 'link.default.shared_libs.*libc++_shared' "$T/ldcfg" \
  && printf '  MATCH  %s\n' "/etc/ld.config.27.txt (libc++_shared in sphal link)" \
  || printf '  DIFFER %s\n' "/etc/ld.config.27.txt (libc++_shared missing from sphal link)"
check /app/MonadoOpenXR/lib/arm64/libopenxr_monado.so \
    "$XR/monado/build-android/src/xrt/targets/openxr/libopenxr_monado.so"
debugfs -R "dump /app/MonadoOpenXR/MonadoOpenXR.apk $T/mx.apk" "$IMG" >/dev/null 2>&1
"$BT/apksigner" verify --print-certs "$T/mx.apk" 2>/dev/null | grep -q "CN=PN2" \
  && printf '  MATCH  %s\n' "/app/MonadoOpenXR/MonadoOpenXR.apk (CN=PN2)" \
  || printf '  DIFFER %s\n' "/app/MonadoOpenXR/MonadoOpenXR.apk (missing or not PN2-signed)"
# pvrservice is the black-display compositor - it must stay out of the image
if debugfs -R "stat /etc/init/pvrservice.rc" "$IMG" 2>&1 | grep -q ext2_lookup; then
  printf '  MATCH  %s\n' "/etc/init/pvrservice.rc absent"
else
  printf '  DIFFER %s\n' "/etc/init/pvrservice.rc still present"
fi

echo
echo "=== init scripts present ==="
debugfs -R "ls -l /etc/init" "$IMG" 2>/dev/null | grep -E 'pn2-' | awk '{printf "  %-28s %s bytes\n", $NF, $6}'

echo
echo "=== pvr_air directory ==="
debugfs -R "ls -l /lib64/pvr_air" "$IMG" 2>/dev/null | awk 'NF>5 {printf "  %-28s %s\n", $NF, $6}'
echo DONE
