#!/system/bin/sh
echo "=== qvrd service block ==="
awk '/^service qvrd/,/^$/' /vendor/etc/init/hw/init.target.rc
echo "=== does init.qcom.rc import init.target.rc? ==="
grep -n import /vendor/etc/init/hw/init.qcom.rc | head -20
echo "=== are ANY services from init.target.rc known to init? ==="
for s in $(awk '/^service /{print $2}' /vendor/etc/init/hw/init.target.rc); do
  v=$(getprop init.svc.$s)
  echo "  $s = ${v:-<unknown to init>}"
done
