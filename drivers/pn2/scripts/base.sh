#!/usr/bin/env bash
# pn2 base payload - runs as drivers/pn2/driver.json's base step on the
# shared clean image. Everything Pico Neo 2 lands here, not in 143: init
# rc + daemons, keylayouts, VINTF manifest, CVService permissions and the
# device's build.prop identity.
set -u
R="${HIBISCUS_ROOT:?set HIBISCUS_ROOT to a build root - see system/dist/README.md}"
IMG="$R/out/system-base.img"
P="$R/drivers/pn2/image"
LOG="$R/notes/150_pn2_base.txt"
exec >"$LOG" 2>&1

fail=0
. "$R/tools/lib/img.sh"
[ -f "$IMG" ] || { echo "FAIL: $IMG missing - 143 never ran"; exit 1; }

echo "=== device init rc + daemons ==="
for f in "$P/etc/init"/*.rc; do put "$f" "/etc/init/$(basename "$f")" 644; done
for f in "$P/bin"/*; do put "$f" "/bin/$(basename "$f")" 755; done

echo
echo "=== keylayouts + VINTF + permissions ==="
mkd /usr/keylayout
for f in "$P/usr/keylayout"/*.kl; do put "$f" "/usr/keylayout/$(basename "$f")" 644; done
mkd /etc/pn2
put "$P/etc/pn2/vendor_manifest.xml" /etc/pn2/vendor_manifest.xml 644
mkd /etc/default-permissions
put "$P/etc/default-permissions/pn2-cvservice.xml" /etc/default-permissions/pn2-cvservice.xml 644

echo
echo "=== libinput keycode labels ==="
# the GSI's libinput does not know Pico's labels, so gpio-keys.kl would be
# discarded whole and Confirm would arrive as ENTER through Generic.kl.
TMP=$(mktemp -d)
for lib in lib64 lib; do
  debugfs -R "dump /$lib/libinput.so $TMP/libinput.so" "$IMG" 2>/dev/null
  if [ -s "$TMP/libinput.so" ] && python3 "$R/tools/patch/401_patch_libinput.py" \
      "$TMP/libinput.so" "$TMP/libinput.patched.so" >/dev/null 2>&1; then
    put "$TMP/libinput.patched.so" "/$lib/libinput.so" 644
  else
    echo "  SKIP    /$lib/libinput.so (patch failed or missing)"
    fail=$((fail+1))
  fi
  rm -f "$TMP/libinput.so" "$TMP/libinput.patched.so"
done

echo
echo "=== device props ==="
debugfs -R "dump /build.prop $TMP/build.prop" "$IMG" 2>/dev/null
if [ -s "$TMP/build.prop" ]; then
  if grep -q 'Pico Neo 2 (A7B10) port additions' "$TMP/build.prop"; then
    echo "  already patched ($(stat -c%s "$TMP/build.prop") bytes)"
  else
    # guarantee a trailing newline first or the append glues onto the last line
    [ -n "$(tail -c1 "$TMP/build.prop")" ] && echo "" >> "$TMP/build.prop"
    cat "$P/props.append" >> "$TMP/build.prop"
    # Keep this writer correct even if a tracked props.append file is missing
    # its final newline. The following identity properties must start on a
    # separate line or Android silently drops ro.product.device.
    [ -n "$(tail -c1 "$TMP/build.prop")" ] && echo "" >> "$TMP/build.prop"
    sed -i 's/^ro\.build\.product=.*/ro.build.product=PICOA7B10/' "$TMP/build.prop"
    grep -q '^ro.product.device=' "$TMP/build.prop" || echo 'ro.product.device=PICOA7B10' >> "$TMP/build.prop"
    grep -q '^ro.product.model='  "$TMP/build.prop" || echo 'ro.product.model=Pico Neo 2' >> "$TMP/build.prop"
    put "$TMP/build.prop" /build.prop 600
  fi
else
  echo "  ERROR: could not read build.prop"
  fail=$((fail+1))
fi
rm -rf "$TMP"

img_fsck || fail=$((fail+1))
[ "$fail" -eq 0 ] && echo "PN2 BASE OK" || echo "PN2 BASE HAD $fail FAILURES"
