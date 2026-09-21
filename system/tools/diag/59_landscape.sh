#!/system/bin/sh
# Force the whole OS to landscape.
#
# The panel reports itself as 2160x3840 portrait, but physically it is mounted
# with the long axis horizontal, spanning both eyes. Leaving Android in portrait
# is why the 2D UI renders across the seam between the lenses. Telling
# SurfaceFlinger the primary display is rotated makes landscape the NATURAL
# orientation, so boot animation, system UI and every app inherit it - not just
# apps that request landscape.
#
# ro.surface_flinger.primary_display_orientation is the Android 10 sysprop;
# ro.sf.hwrotation is the older name some paths still read. Set both.
# Also pin auto-rotate off so the accelerometer can never flip it back.

set -e
mount -o rw,remount /system

for f in /system/build.prop /system/etc/prop.default; do
  [ -f "$f" ] || continue
  if grep -q '^ro.surface_flinger.primary_display_orientation' "$f"; then
    echo "already set in $f"; continue
  fi
  {
    echo ""
    echo "# Panel is physically landscape across both eyes but reports 2160x3840"
    echo "# portrait. Make landscape the natural orientation so nothing renders"
    echo "# across the seam between the lenses."
    echo "ro.surface_flinger.primary_display_orientation=ORIENTATION_90"
    echo "ro.sf.hwrotation=90"
  } >> "$f"
  echo "patched $f"
done

sync
mount -o ro,remount /system

# auto-rotate off, fixed rotation. These live in /data, so also worth baking into
# the image later via a default settings overlay.
settings put system accelerometer_rotation 0
settings put system user_rotation 0

echo "--- before reboot ---"
wm size
wm density
echo "accelerometer_rotation = $(settings get system accelerometer_rotation)"
echo "user_rotation          = $(settings get system user_rotation)"
echo DONE
