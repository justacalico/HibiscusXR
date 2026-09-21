#!/system/bin/sh
# Qualcomm's own QVR client, from /vendor/bin. If it gets a client handle the
# service is healthy and Pico's client path is at fault; if it fails identically,
# qvrservice itself is the problem.
for T in /vendor/bin/qvrservicetest64 /vendor/bin/qvrservicetest; do
  [ -x "$T" ] || continue
  echo "######## $T"
  timeout 15 "$T" 2>&1 | head -25
  echo "--- done ---"
  echo
done
echo "=== what qvrservice logged during the test ==="
logcat -d -t 120 2>/dev/null | grep -iE 'QVRService|qvrservicetest|ConnectionMgr' | tail -15
