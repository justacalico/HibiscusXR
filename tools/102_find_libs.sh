#!/bin/bash
# CVService wants libCVController.so, which is not the same file as the
# libCVControllerClient.pxr.so I already pulled. My extraction list was written
# from the directory listing rather than from what the apps actually load, so
# there may be more gaps. Find every non-AOSP-looking lib in the stock image so
# we can match against what the Pico apps ask for.
IMG=/mnt/f/PN2Lineage/images/ota_4.1.3/system.img
LOG=/mnt/f/PN2Lineage/notes/102_libs.txt
exec >"$LOG" 2>&1

echo "=== anything matching CVController ==="
for d in /lib64 /lib; do
  debugfs -R "ls -l $d" "$IMG" 2>/dev/null | grep -i 'cvcontroller'
done
echo

echo "=== all pico/pxr/pvr/qvr/vr libs in /lib64 ==="
debugfs -R "ls -l /lib64" "$IMG" 2>/dev/null \
  | grep -iE 'pvr|pxr|qvr|pico|cv|svr|vr[a-z]*\.so' | awk '{print $NF}' | sort
echo

echo "=== same for /lib ==="
debugfs -R "ls -l /lib" "$IMG" 2>/dev/null \
  | grep -iE 'pvr|pxr|qvr|pico|cv|svr|vr[a-z]*\.so' | awk '{print $NF}' | sort
echo

echo "=== what native libs do the installed apks reference? ==="
# JNI libs the apps System.loadLibrary() - grep the dex we extracted for names
for d in /mnt/f/PN2Lineage/pvr_dex/*/; do
  n=$(basename "$d")
  f=$(ls "$d"/*.dex 2>/dev/null | head -1)
  [ -f "$f" ] || continue
  libs=$(strings -a "$f" 2>/dev/null | grep -E '^(lib)?[A-Za-z0-9_]+$' \
         | grep -iE 'cvcontroller|pvr|pxr|qvr|svr|vrapi|tracking' | sort -u | tr '\n' ' ')
  [ -n "$libs" ] && printf '  %-22s %s\n' "$n" "$libs"
done
echo DONE
