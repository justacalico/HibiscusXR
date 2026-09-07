#!/system/bin/sh
# Does stock's airservice also log QVRServiceClient_Create 0x0, or does it get a
# real client? That decides whether the NULL is the actual fault or a red herring.
echo "=== airservice + QVR lines on this device ==="
logcat -d 2>/dev/null | grep -iE 'AIRService|QVRServiceClient|QVRService:' | tail -30
echo
echo "=== is qvrservice actually serving anyone? ==="
ls -l /dev/socket/ 2>/dev/null | grep -i qvr
echo "--- who has the sockets open ---"
for p in $(pidof airservice) $(pidof qvrservice); do
  echo "pid $p:"
  ls -l /proc/$p/fd 2>/dev/null | grep -i socket | head -5
done
