#!/system/bin/sh
# ALVR says it cannot find an EGL config, but our own NativeActivity demo picks
# one on this same system - so configs exist and the display works. That points
# at ALVR's config FILTER, not at EGL being broken. Find out what it is matching
# against.
echo "=== ALVR this run ==="
logcat -d 2>/dev/null | grep -iE 'ALVR NATIVE-RUST|P2ALVR' | grep -viE '^\s*[0-9]+:|Backtrace' | tail -20
echo
echo "=== what pixel format is the window actually? ==="
dumpsys SurfaceFlinger 2>/dev/null | grep -iE 'format|Display 0|activeConfig|orientation' | head -20
echo
echo "=== display layers and their formats ==="
dumpsys SurfaceFlinger 2>/dev/null | grep -iE '^\s*\+ (Buffer|Color)Layer' | head -10
echo
echo "=== our demo picked a config fine - proof EGL works ==="
logcat -d 2>/dev/null | grep -E 'pn2vr.*(GL_VENDOR|surface|eye targets)' | tail -4
echo
echo "=== rotation props in play ==="
for p in ro.surface_flinger.primary_display_orientation ro.sf.hwrotation \
         persist.sys.sf.native_mode ro.surface_flinger.max_frame_buffer_acquired_buffers; do
  echo "  $p = $(getprop $p)"
done
echo
echo "=== gralloc / mapper versions available ==="
lshal 2>/dev/null | grep -iE 'graphics.mapper|graphics.allocator'
echo DONE
