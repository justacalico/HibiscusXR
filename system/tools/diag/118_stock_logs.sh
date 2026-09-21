#!/system/bin/sh
# Stock has persist.pvr.isLog2SDCard=1, so PUI may keep its own logs on disk.
# A VRShell startup log from a WORKING system would show whether
# "Open library<lib6DofReset.so> failed" is normal there too.
echo "##### /sdcard top level #####"
ls -lt /sdcard/ 2>/dev/null | head -15
echo
echo "##### anything log-ish #####"
find /sdcard -maxdepth 3 -iname '*log*' 2>/dev/null | head -20
echo
echo "##### psmart / pico dirs #####"
find /sdcard -maxdepth 2 -iname '*psmart*' -o -maxdepth 2 -iname '*pico*' -o -maxdepth 2 -iname '*pvr*' 2>/dev/null | head -20
echo
echo "##### does the psmvrapi config the app looks for exist here #####"
ls -l /sdcard/psmart/phoenix/psmvrapi_config.txt 2>&1
echo DONE
