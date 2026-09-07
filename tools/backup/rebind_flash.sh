#!/system/bin/sh
# Re-run the w25q probe (which re-asserts the FPGA reset pin) then immediately write.
D=/sys/bus/spi/drivers/spi-w25q

echo "=== driver binding before ==="
ls -l $D 2>&1 | grep -E 'spi[0-9]'
echo "gpio84: $(cat /sys/kernel/debug/gpio 2>/dev/null | grep -E 'gpio84')"
echo

echo "=== unbind spi3.0 ==="
echo spi3.0 > $D/unbind 2>&1
sleep 1
echo "gpio84 after unbind: $(cat /sys/kernel/debug/gpio 2>/dev/null | grep -E 'gpio84')"
echo

echo "=== rebind spi3.0 ==="
echo spi3.0 > $D/bind 2>&1
sleep 2
ls -l $D 2>&1 | grep -E 'spi[0-9]'
echo "gpio84 after rebind: $(cat /sys/kernel/debug/gpio 2>/dev/null | grep -E 'gpio84')"
echo "--- probe dmesg ---"
dmesg | grep -i -E 'w25q spi flash probe|fpga reset pin' | tail -6
echo

echo "=== immediate write attempt ==="
/system/bin/w25q_write_bin 2>&1
echo "exit=$?"
echo
echo "--- dmesg ---"
dmesg | grep -i -E 'w25q_wait_null|enter w25q|w25q_write_image' | tail -8
echo

echo "=== read back 256 ==="
/data/local/tmp/w25qdump 256 /data/local/tmp/after.bin 2>&1 | grep -E 'VERDICT|buffer|0xFF'
echo DONE
