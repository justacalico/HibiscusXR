#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Patch the Adreno context-attrib parser and shadow the blob into
# system-pn2-full.img.
#
# The V@378 driver rejects EGL_CONTEXT_OPENGL_ROBUST_ACCESS (0x31B2, the EGL
# 1.5 core token) in eglCreateContext with EGL_BAD_ATTRIBUTE while happily
# accepting the EXT variant (0x30BF). The driver reports EGL 1.5, so spec-
# conforming callers send the core token - wgpu's GL backend does, its
# eglCreateContext fails, the GL backend never registers, and
# enumerate_adapters() comes back empty. ALVR then panics in
# alvr_initialize_opengl on adapters.remove(0) - the "removal index (is 0)
# should be < len (is 0)" abort seen on-device.
#
# 405_patch_gles_robust.py makes the unknown-key path accept 0x31B2 and route
# it to the driver's own 0x30BF handler. The blob lives in
# /system/etc/pn2/ and pn2-egl.rc bind-mounts it over the vendor file at
# early-init, so the vendor partition stays byte-identical.
#
# Stock blob source: linklibs/ or notes/qlibs/ (blobs package) first, adb
# pull as fallback. Full image only - the clean image ships no proprietary
# content. 64-bit only: every wgpu client seen so far is arm64; the 32-bit
# vendor blob keeps the same bug for now.
set -u
IMG=${PN2_ROOT}/out/system-pn2-full.img
LOG=${PN2_ROOT}/notes/406_glespatch.txt
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
exec >"$LOG" 2>&1

fail=0
put() {
  local src="$1" dst="$2" mode="$3" dir base want got
  dir=$(dirname "$dst"); base=$(basename "$dst")
  [ -f "$src" ] || { printf '  MISSING SRC %s\n' "$src"; fail=$((fail+1)); return; }
  debugfs -w -R "rm $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "write $src $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst mode 0100$mode" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst gid 0" "$IMG" >/dev/null 2>&1
  want=$(stat -c%s "$src")
  got=$(debugfs -R "ls -l $dir" "$IMG" 2>/dev/null | awk -v b="$base" '$NF==b {print $6}' | head -1)
  if [ "$got" = "$want" ]; then printf '  OK    %-44s %10s\n' "$dst" "$got"
  else printf '  FAIL  %-44s want %s got "%s"\n' "$dst" "$want" "${got:-absent}"; fail=$((fail+1)); fi
}

echo "=== stock blob source ==="
SRC=""
for c in "$PN2_ROOT/linklibs/libGLESv2_adreno.so" \
         "$PN2_ROOT/notes/qlibs/libGLESv2_adreno.so"; do
  [ -s "$c" ] && SRC="$c" && break
done
if [ -z "$SRC" ]; then
  adb pull /vendor/lib64/egl/libGLESv2_adreno.so "$TMP/stock.so" >/dev/null 2>&1 \
    && SRC="$TMP/stock.so" || true
fi
[ -n "$SRC" ] || { echo "no stock libGLESv2_adreno.so (linklibs, notes/qlibs, adb all empty)"; echo DONE; exit 0; }
ls -l "$SRC"

echo
echo "=== patch ==="
if python3 "$PN2_ROOT/tools/patch/405_patch_gles_robust.py" \
     "$SRC" "$TMP/libGLESv2_adreno.so"; then
  ls -l "$TMP/libGLESv2_adreno.so"
else
  echo "PATCH FAILED"
  echo DONE
  exit 0
fi

echo
echo "=== inject into full image ==="
debugfs -w -R "mkdir /etc/pn2" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /etc/pn2 mode 040755" "$IMG" >/dev/null 2>&1
put "$TMP/libGLESv2_adreno.so"              "/etc/pn2/libGLESv2_adreno.so" 644
put "$PN2_ROOT/overlay/etc/init/pn2-egl.rc" "/etc/init/pn2-egl.rc"          644

echo
echo "=== repair + verify ==="
e2fsck -fy "$IMG" 2>&1 | tail -4
if e2fsck -fn "$IMG" >/tmp/f4.txt 2>&1; then tail -2 /tmp/f4.txt; echo "  CLEAN"; else tail -6 /tmp/f4.txt; echo "  DIRTY"; fail=$((fail+1)); fi

echo
[ "$fail" -eq 0 ] && echo "GLES ROBUST PATCH OK" || echo "HAD $fail FAILURES"
echo DONE
