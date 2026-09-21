#!/system/bin/sh
# ON STOCK. VRShell's PvrClient does a PackageManager lookup for
# "com.pvr.pvrservice" and we do not have that package at all - it was not in the
# 12 apps I extracted. Find its apk so we can bring it over.
echo "##### is it installed #####"
pm list packages 2>/dev/null | grep -iE 'pvrservice|pvr\.'
echo
echo "##### where does it live #####"
pm path com.pvr.pvrservice 2>&1
echo
echo "##### full pico package -> path map #####"
for p in $(pm list packages 2>/dev/null | sed 's/^package://' | grep -iE 'pvr|pico|psmart'); do
  echo "$p -> $(pm path $p 2>/dev/null | sed 's/^package://')"
done
echo DONE
