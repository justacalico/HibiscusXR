#!/system/bin/sh
echo "=========== QVR client C API (right path this time) ==========="
for f in /system/lib64/libqvrservice_client.so /system/lib64/libqvrcamera_client.so; do
  echo "--- $f ---"
  strings -a "$f" | grep -E "^QVR[A-Za-z]*_[A-Za-z]" | sort -u | head -60
done

echo
echo "=========== is qvrservice reachable standalone? ==========="
echo "--- qvrservice sockets / props ---"
ls -l /dev/socket/ 2>/dev/null | grep -i qvr
getprop | grep -iE "qvr" | head

echo
echo "=========== pxr_config.txt (plaintext?) ==========="
head -c 1200 /system/etc/pvr/pxr_config.txt 2>/dev/null
echo
echo "=========== res.json head ==========="
head -c 900 /system/etc/pvr/res.json 2>/dev/null
echo
echo "=========== slam + boundary dirs ==========="
ls -l /system/etc/pvr/slam /system/etc/pvr/boundary 2>/dev/null

echo
echo "=========== any lens/distortion mesh anywhere ==========="
ls -l /persist/calibration /persist/calibrationfiles /persist/picoconfig 2>/dev/null
echo "--- grep the config blobs for distortion keywords ---"
strings -a /system/etc/pvr/psmvrapi_config1.txt 2>/dev/null | head -5
strings -a /system/lib64/libsvrapi.so 2>/dev/null | grep -iE "distortion|polynomial|lens|fov|ipd|k1|mesh" | sort -u | head -30
