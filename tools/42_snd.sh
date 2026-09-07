#!/system/bin/sh
echo "=== every sound/asoc/codec line in dmesg ==="
dmesg 2>/dev/null | grep -iE 'asoc|snd_|sound|wcd|tavil|wsa|codec|q6afe|q6asm|q6adm|msm-dai|deferred|EPROBE' | head -80
echo
echo "=== asoc sysfs ==="
ls /sys/kernel/debug/asoc/ 2>&1
ls -d /sys/devices/platform/soc/*sound* /sys/devices/platform/sound* 2>&1
echo
echo "=== driver bind state for the machine driver ==="
for d in /sys/bus/platform/drivers/*; do
  case "$d" in *sdm845*|*asoc*|*snd*|*sound*) echo "== $d"; ls "$d" ;; esac
done
echo
echo "=== devices that failed to probe ==="
cat /sys/kernel/debug/devices_deferred 2>&1 | head -30
echo
echo "=== is the machine driver a module? ==="
lsmod 2>/dev/null | head -20
ls /vendor/lib/modules/ 2>&1 | head -20
echo
echo "=== what does the DT say the sound node is ==="
ls -d /proc/device-tree/soc/sound* /proc/device-tree/sound* 2>&1
for n in /proc/device-tree/soc/sound*; do
  echo "-- $n"; cat "$n/compatible" 2>/dev/null | tr '\0' '\n'; cat "$n/status" 2>/dev/null | tr '\0' '\n'
done
echo
echo "=== vendor audio init services ==="
getprop | grep -iE 'init\.svc.*(audio|adsp|sensor)'
echo DONE
