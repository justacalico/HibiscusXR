#!/system/bin/sh
# Make today's two fixes survive a reboot, both on the SYSTEM side so stock
# /vendor stays untouched (the manifest edit still needs replacing later).
#
# 1. Audio modules. The stock vendor loads them at boot with
#      /vendor/bin/modprobe -a -d /vendor/lib/modules pinctrl-wcd wcd-dsp-glink \
#         snd-soc-wcd-spi snd-soc-sdm845
#    toybox modprobe hard-fails (rc=1) when /etc/modprobe.conf and /etc/modprobe.d
#    are absent. /etc is a symlink to /system/etc; the GSI ships neither, so the
#    exec died and no sound card ever registered. Empty ones are enough.
#
# 2. audioserver SIGSEGVs in getHwOffloadEncodingFormatsSupportedForA2DP() because
#    vendor advertises persist.vendor.bt.a2dp_offload_cap=sbc-aac but the primary
#    module exposes no offload formats, so the lookup derefs null. Turn the
#    framework's offload path off.

set -e
mount -o rw,remount /system

# --- 1. modprobe config ---------------------------------------------------
mkdir -p /system/etc/modprobe.d
[ -f /system/etc/modprobe.conf ] || : > /system/etc/modprobe.conf
chmod 755 /system/etc/modprobe.d
chmod 644 /system/etc/modprobe.conf
echo "created: $(ls -ld /system/etc/modprobe.d) / $(ls -l /system/etc/modprobe.conf)"

# prove the stock command now succeeds (modules already in, so it is a no-op)
/vendor/bin/modprobe -a -d /vendor/lib/modules pinctrl-wcd wcd-dsp-glink snd-soc-wcd-spi snd-soc-sdm845
echo "stock modprobe command rc=$?"

# --- 2. kill the A2DP offload path ---------------------------------------
for f in /system/build.prop /system/etc/prop.default; do
  [ -f "$f" ] || continue
  grep -q '^persist.bluetooth.a2dp_offload.disabled' "$f" && continue
  {
    echo ""
    echo "# a2dp offload disabled: vendor advertises offload_cap but exposes no"
    echo "# offload formats, so AudioPolicyManager null-derefs on the query."
    echo "persist.bluetooth.a2dp_offload.disabled=true"
    echo "ro.bluetooth.a2dp_offload.supported=false"
  } >> "$f"
  echo "patched $f"
done

sync
mount -o ro,remount /system
echo "--- verify ---"
ls -ld /system/etc/modprobe.d
tail -4 /system/build.prop
echo DONE
