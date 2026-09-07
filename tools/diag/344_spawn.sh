#!/system/bin/sh
P=$(pidof pvrservice)
echo "=== children of pvrservice ==="
ps -A -o PID,PPID,NAME 2>/dev/null | awk -v p="$P" '$2==p'
echo
echo "=== zombie / spawned leftovers ==="
ps -A 2>/dev/null | grep -E " Z |defunct" | head
echo
echo "=== command-ish strings in libpvrservice.so ==="
strings -a /system/lib64/libpvrservice.so 2>/dev/null | grep -E "^(/system/bin|/vendor/bin|sh -c|setprop|getprop|svc |logcat|echo |cat /|chmod|am |pm )" | head -20
echo
echo "=== does it link popen/system/posix_spawn ==="
strings -a /system/lib64/libpvrservice.so 2>/dev/null | grep -xE "popen|system|posix_spawn|fork|execl|execvp"
echo
echo "=== same for the platform module ==="
strings -a /system/lib64/libpvrmodule_platform.so 2>/dev/null | grep -xE "popen|system|posix_spawn"
strings -a /system/lib64/libpvrmodule_platform.so 2>/dev/null | grep -E "^(/system/bin|/vendor/bin|sh -c)" | head -10
echo
echo "=== and the tracker module ==="
for f in /system/lib64/libpvrmodule_*.so; do
  if strings -a "$f" 2>/dev/null | grep -qxE "popen|system|posix_spawn"; then
    echo "  SPAWNER: $f"
    strings -a "$f" 2>/dev/null | grep -E "^(/system/bin|/vendor/bin|sh -c|.*\.sh$)" | head -6
  fi
done
