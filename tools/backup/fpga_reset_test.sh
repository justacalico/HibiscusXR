#!/system/bin/sh
# Test whether holding the FPGA in reset frees the SPI bus for flash programming.
echo "=== gpiochip bases ==="
for c in /sys/class/gpio/gpiochip*; do
  echo "$c base=$(cat $c/base 2>/dev/null) ngpio=$(cat $c/ngpio 2>/dev/null) label=$(cat $c/label 2>/dev/null)"
done
echo

echo "=== current gpio84 (debugfs view) ==="
cat /sys/kernel/debug/gpio 2>/dev/null | grep -E 'gpio84|gpio-84'
echo

echo "=== attempt export of gpio 84 ==="
echo 84 > /sys/class/gpio/export 2>&1
if [ -d /sys/class/gpio/gpio84 ]; then
  echo "exported OK"
  echo "  direction: $(cat /sys/class/gpio/gpio84/direction 2>&1)"
  echo "  value    : $(cat /sys/class/gpio/gpio84/value 2>&1)"
else
  echo "export failed (likely claimed by the w25q driver)"
fi
echo

echo "=== BASELINE: read 256 bytes, note the pattern ==="
/data/local/tmp/w25qdump 256 /data/local/tmp/base.bin 2>&1 | grep -E 'VERDICT|buffer'
echo

for V in 0 1; do
  echo "############ gpio84 = $V ############"
  if [ -d /sys/class/gpio/gpio84 ]; then
    echo out > /sys/class/gpio/gpio84/direction 2>&1
    echo $V  > /sys/class/gpio/gpio84/value 2>&1
    echo "  set value -> $(cat /sys/class/gpio/gpio84/value 2>&1)"
    sleep 1
    echo "  --- read test ---"
    /data/local/tmp/w25qdump 256 /data/local/tmp/g$V.bin 2>&1 | grep -E 'VERDICT|buffer'
    echo "  --- write attempt ---"
    /system/bin/w25q_write_bin 2>&1 | tail -6
    echo "  exit=$?"
    echo "  --- dmesg ---"
    dmesg | grep -i -E 'w25q_wait_null|enter w25q|Check result' | tail -4
  else
    echo "  (gpio not exportable, skipping)"
  fi
  echo
done

echo "=== restore: leave gpio as found (0) ==="
if [ -d /sys/class/gpio/gpio84 ]; then
  echo 0 > /sys/class/gpio/gpio84/value 2>&1
  echo 84 > /sys/class/gpio/unexport 2>&1
  echo "unexported"
fi
echo TEST DONE
