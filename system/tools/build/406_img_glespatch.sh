#!/bin/bash
. "$(dirname "${BASH_SOURCE[0]}")/../lib/env.sh"
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
IMG=${HIBISCUS_ROOT}/out/system-pn2-full.img
LOG=${HIBISCUS_ROOT}/notes/406_glespatch.txt
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
exec >"$LOG" 2>&1

fail=0
. "$(dirname "${BASH_SOURCE[0]}")/../lib/img.sh"

echo "=== stock blob source ==="
SRC=""
for c in "$HIBISCUS_ROOT/linklibs/libGLESv2_adreno.so" \
         "$HIBISCUS_ROOT/notes/qlibs/libGLESv2_adreno.so"; do
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
if python3 "$HIBISCUS_ROOT/tools/patch/405_patch_gles_robust.py" \
     "$SRC" "$TMP/libGLESv2_adreno.so"; then
  ls -l "$TMP/libGLESv2_adreno.so"
else
  echo "PATCH FAILED"
  echo DONE
  exit 0
fi

echo
echo "=== inject into full image ==="
mkd /etc/pn2
# pn2-egl.rc (the bind mount over the vendor blob) ships with the rest of
# the device identity - the pn2 driver's base step installs it
put "$TMP/libGLESv2_adreno.so"              "/etc/pn2/libGLESv2_adreno.so" 644

echo
echo "=== repair + verify ==="
img_fsck || fail=$((fail+1))

echo
[ "$fail" -eq 0 ] && echo "GLES ROBUST PATCH OK" || echo "HAD $fail FAILURES"
echo DONE
