#!/system/bin/sh
# The DSP filesystem side is identical to stock, so the difference is runtime.
# FastRPC can only get a remote handle if the CDSP subsystem has actually booted.
echo "=== subsystem states ==="
for d in /sys/class/subsys/subsys*; do
  [ -d "$d" ] || continue
  n=$(cat $d/name 2>/dev/null)
  s=$(cat $d/state 2>/dev/null)
  echo "  $n = $s"
done
echo
echo "=== dsp rpc daemons running? ==="
ps -A 2>/dev/null | grep -iE 'adsprpcd|cdsprpcd|sdsprpcd'
getprop | grep -iE 'init.svc.*rpcd'
echo
echo "=== fastrpc kernel messages ==="
dmesg 2>/dev/null | grep -iE 'adsprpc|cdsp|turing|fastrpc' | tail -20
echo
echo "=== pil / firmware loading ==="
dmesg 2>/dev/null | grep -iE 'pil|subsys.*boot|Booting|mdt' | tail -15
