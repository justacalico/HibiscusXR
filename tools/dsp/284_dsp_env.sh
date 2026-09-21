#!/system/bin/sh
# FastRPC cannot get a remote handle: the DSP-side skel library is not being
# found. Those are located via ADSP_LIBRARY_PATH and live under lib/rfsa.
P=$(pidof qvrservice)
echo "=== qvrservice environment (ADSP_LIBRARY_PATH?) ==="
tr '\0' '\n' < /proc/$P/environ 2>/dev/null | grep -iE 'ADSP|DSP|LD_' || echo "  (none set)"
echo
echo "=== fastrpc device nodes ==="
ls -lZ /dev/*rpc* 2>/dev/null
echo
echo "=== skel libraries on the filesystem ==="
for d in /vendor/lib/rfsa/adsp /vendor/lib/rfsa/dsp /vendor/dsp /dsp /vendor/lib/rfsa/cdsp; do
  [ -d "$d" ] && echo "--- $d ---" && ls "$d" 2>/dev/null | grep -iE 'qvr|skel' | head -10
done
echo
echo "=== anything named qvr on the dsp side ==="
find /vendor -name '*qvr*skel*' 2>/dev/null | head
find /vendor -name '*_skel.so' 2>/dev/null | head -10
echo
echo "=== is the cdsp subsystem up? ==="
cat /sys/kernel/debug/rpmsg/*/name 2>/dev/null | head
ls /sys/class/subsys* 2>/dev/null | head
for s in /sys/devices/platform/soc/*turing*/subsys*/state /sys/devices/platform/soc/*cdsp*/state; do
  [ -e "$s" ] && echo "  $s = $(cat $s 2>/dev/null)"
done
