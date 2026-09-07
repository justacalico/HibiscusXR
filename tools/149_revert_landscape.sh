#!/system/bin/sh
# Undo the forced landscape and see whether VRShell survives.
#
# pvrservice on stock:  Override displayinfo: w=2160 h=3840   (native portrait)
# pvrservice on ours:   Override displayinfo: w=3840 h=2160   (swapped)
#
# The only difference is our ro.surface_flinger.primary_display_orientation=
# ORIENTATION_90, added to stop the 2D UI rendering across the seam between the
# lenses. Pico's stack derives lens and viewport geometry from the panel's native
# portrait dimensions, so handing it a rotated display makes UpdateLensInfo
# compute garbage - which is exactly where every Pico VR app dies.
#
# If this is right, the landscape fix and the VR stack are mutually exclusive as
# implemented, and the 2D seam has to be solved a different way (per-app
# orientation, or letting Pico's compositor own the display).
exec 2>&1
mount -o rw,remount /system
cp -f /system/build.prop /system/build.prop.landscape.bak
sed -i 's/^ro\.surface_flinger\.primary_display_orientation=/#REVERTED ro.surface_flinger.primary_display_orientation=/' /system/build.prop
sed -i 's/^ro\.sf\.hwrotation=/#REVERTED ro.sf.hwrotation=/' /system/build.prop
sync
mount -o ro,remount /system
echo "=== orientation lines now ==="
grep -nE 'primary_display_orientation|hwrotation' /system/build.prop
echo
echo "rebooting"
