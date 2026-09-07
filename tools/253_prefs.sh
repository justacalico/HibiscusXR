#!/system/bin/sh
# Where does the "setup already done" flag live? Provision crashes on our build so
# it never writes one; stock completed setup long ago and must have recorded it.
for P in com.picovr.provision com.pvr.launcher; do
  echo "######## $P"
  ls -l /data/data/$P/shared_prefs/ 2>/dev/null
  for f in /data/data/$P/shared_prefs/*.xml; do
    [ -f "$f" ] && echo "--- $f ---" && cat "$f"
  done 2>/dev/null
  echo
done
echo "=== any provision-ish files elsewhere ==="
ls -l /data/local/ 2>/dev/null | head
ls /sdcard/ 2>/dev/null | head -20
