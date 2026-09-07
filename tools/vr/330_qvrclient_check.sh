#!/system/bin/sh
# pvrservice calls QVRServiceClient_Create and is told "VR not supported", while
# the vendor test binary gets "supported" from the same service. pvrservice never
# maps libqvrservice_client.so, so its dlopen is probably failing.
echo "=== the client libs we have ==="
ls -l /system/lib64/libqvrservice_client.so /system/lib/libqvrservice_client.so 2>/dev/null
ls -l /vendor/lib64/libqvrservice_client.so /vendor/lib/libqvrservice_client.so 2>/dev/null
echo
echo "=== md5: is /system's copy the same as /vendor's? ==="
md5sum /system/lib64/libqvrservice_client.so /vendor/lib64/libqvrservice_client.so 2>/dev/null
echo
echo "=== which one does the vendor test binary use? ==="
T=$(pidof qvrservicetest64)
[ -n "$T" ] && grep -oE '/[^ ]*libqvr[^ ]*\.so' /proc/$T/maps 2>/dev/null | sort -u
echo
echo "=== is it whitelisted (matters if anything app-side loads it) ==="
grep -i qvrservice_client /system/etc/public.libraries.txt 2>/dev/null || echo "  not whitelisted"
echo
echo "=== try loading it the way pvrservice would ==="
# a 64-bit process in the system namespace
cat > /data/local/tmp/_ldtest.sh <<'EOF'
EOF
echo "  (checking deps instead)"
