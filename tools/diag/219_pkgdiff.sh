#!/system/bin/sh
# Everything Pico-ish that is installed, plus what is present on disk but not
# installed. Run on both units and diff to find the missing calibration app.
echo "=== installed packages (pico/pvr/picovr) ==="
pm list packages 2>/dev/null | sed 's/^package://' | grep -iE 'pico|pvr|psmart|pxr' | sort
echo
echo "=== apks present in /system/app and /system/priv-app ==="
ls /system/app /system/priv-app 2>/dev/null | sort
echo
echo "=== anything named like calibration / provision / sensor ==="
pm list packages 2>/dev/null | sed 's/^package://' | grep -iE 'calib|provis|sensor|6dof|track|setup|wizard' | sort
