#!/system/bin/sh
# READ-ONLY. Find which read recipe the w25q driver actually honours.
# Safe on the erased unit: reads only, and the flash is already blank.
O=/data/local/tmp/rt
rm -f $O.* 2>/dev/null

try() {   # label  dd-args...
  L="$1"; shift
  rm -f $O.$L
  dd if=/dev/w25q of=$O.$L "$@" 2>/dev/null
  SZ=$(stat -c%s $O.$L 2>/dev/null)
  [ -z "$SZ" ] && SZ=err
  echo "  $L  ->  $SZ bytes   (dd $*)"
}

echo "=== block-size matrix ==="
try bs1_8192      bs=1 count=8192
try bs256_1024    bs=256 count=1024
try bs512_512     bs=512 count=512
try bs1024_256    bs=1024 count=256
try bs2048_128    bs=2048 count=128
try bs4096_64     bs=4096 count=64
try bs8192_1      bs=8192 count=1
try bs8192_32     bs=8192 count=32
try bs16384_16    bs=16384 count=16
try bs65536_4     bs=65536 count=4
try bs262144_1    bs=262144 count=1
try noargs        bs=8192

echo
echo "=== cat ==="
rm -f $O.cat
cat /dev/w25q > $O.cat 2>/dev/null
echo "  cat -> $(stat -c%s $O.cat 2>/dev/null) bytes"

echo
echo "=== toybox/busybox variants ==="
rm -f $O.tb
toybox dd if=/dev/w25q of=$O.tb bs=8192 count=32 2>/dev/null
echo "  toybox dd bs=8192 count=32 -> $(stat -c%s $O.tb 2>/dev/null) bytes"

echo
echo "=== best result content check ==="
BEST=$(ls -S $O.* 2>/dev/null | head -1)
if [ -n "$BEST" ]; then
  echo "largest: $BEST ($(stat -c%s $BEST) bytes)"
  echo "--- head ---"
  od -A x -t x1 $BEST 2>/dev/null | head -4
  echo "--- distinct bytes ---"
  od -An -v -t x1 $BEST 2>/dev/null | tr ' ' '\n' | grep -v '^$' | sort -u | tr '\n' ' '
  echo
fi

echo
echo "=== dmesg tail after reads ==="
dmesg | grep -i w25q | tail -12
echo READTEST DONE
