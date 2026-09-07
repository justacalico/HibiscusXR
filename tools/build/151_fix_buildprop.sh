#!/system/bin/sh
# Two things:
#
# 1. Repair a corruption I introduced. An earlier append to build.prop did not
#    end with a newline, so the next line got glued on:
#       ro.sf.hwrotation=90ro.product.device=PICOA7B10
#    which meant ro.product.device was never set at all.
#
# 2. Restore the forced landscape. Reverting it was a test of the theory that it
#    broke Pico's lens maths - it did not; with native portrait, pvrservice logs
#    byte-identical values to stock and VRShell still dies in the same place. So
#    landscape is safe and it is what makes the 2D UI readable in the headset.
exec 2>&1
mount -o rw,remount /system
cp -f /system/build.prop /system/build.prop.bak2

# drop every line we have ever written for these keys, commented or not
sed -i -E '/^#?(REVERTED )?ro\.surface_flinger\.primary_display_orientation/d' /system/build.prop
sed -i -E '/^#?(REVERTED )?ro\.sf\.hwrotation/d' /system/build.prop
sed -i -E '/^ro\.product\.device=/d' /system/build.prop
sed -i -E '/^ro\.product\.model=/d' /system/build.prop

# make sure the file ends with a newline before appending, or we glue lines again
tail -c1 /system/build.prop | od -An -c | grep -q '\\n' || echo "" >> /system/build.prop

cat >> /system/build.prop <<'EOF'
ro.surface_flinger.primary_display_orientation=ORIENTATION_90
ro.sf.hwrotation=90
ro.product.device=PICOA7B10
ro.product.model=Pico Neo 2
EOF

sync
mount -o ro,remount /system

echo "=== the lines now, one per line ==="
grep -nE 'primary_display_orientation|hwrotation|ro\.product\.(device|model)|ro\.build\.product' /system/build.prop
echo
echo "=== sanity: any line with two = signs (glued properties)? ==="
grep -nE '^[a-z].*=.*[a-z]+\.[a-z].*=' /system/build.prop || echo "  none - clean"
echo DONE
