#!/system/bin/sh
# READ-ONLY probe of the FPGA SPI NOR. Nothing here writes to flash.
O=/sdcard/w25q_probe

echo "=== kernel messages ==="
dmesg | grep -i w25q
dmesg | grep -i winbond
dmesg | grep -i 'spi-nor'
dmesg | grep -i spinor
dmesg | grep -i fpga
dmesg | grep -i ndi
echo "(end dmesg)"
echo

echo "=== device nodes ==="
ls -l /dev/w25q /dev/spidev1.0
echo

echo "=== char driver 235 ==="
grep 235 /proc/devices
echo

echo "=== TEST READ: 4096 bytes ==="
dd if=/dev/w25q of=$O.4k bs=4096 count=1 2>&1
ls -l $O.4k 2>/dev/null
echo "--- first 256 bytes ---"
od -A x -t x1z $O.4k 2>/dev/null | head -16
echo

echo "=== TEST READ: 1 MiB ==="
dd if=/dev/w25q of=$O.1m bs=65536 count=16 2>&1
ls -l $O.1m 2>/dev/null
echo

echo "=== FULL READ (no count, read to EOF) ==="
dd if=/dev/w25q of=$O.full bs=65536 2>&1
ls -l $O.full 2>/dev/null
echo

echo "=== entropy sanity: how much is non-zero? ==="
if [ -f $O.full ]; then
  SZ=$(stat -c%s $O.full 2>/dev/null)
  echo "size: $SZ"
  echo "--- head ---"
  od -A x -t x1z $O.full 2>/dev/null | head -8
  echo "--- tail ---"
  od -A x -t x1z $O.full 2>/dev/null | tail -8
  echo "--- count of all-zero 16-byte lines (od squeezes repeats with *) ---"
  od -A x -t x1 $O.full 2>/dev/null | grep -c '\*'
fi
echo
echo "PROBE DONE"
