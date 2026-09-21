#!/system/bin/sh
# Are we on the real Adreno driver, or has something fallen back to swiftshader?
echo "=== what SurfaceFlinger actually got ==="
dumpsys SurfaceFlinger 2>/dev/null | grep -iE 'GLES:|EGL |Vendor|Renderer|Version|DisplayId|activeConfig|composition engine' | head -20
echo
echo "=== egl props ==="
for p in ro.hardware.egl ro.hardware.vulkan ro.opengles.version debug.sf.hw \
         ro.sf.lcd_density persist.sys.sf.native_mode debug.renderengine.backend; do
  echo "$p = $(getprop $p)"
done
echo
echo "=== vendor GL drivers present ==="
ls -l /vendor/lib64/egl/ 2>&1
echo "--- vulkan ---"
ls -l /vendor/lib64/hw/vulkan.* 2>&1
echo
echo "=== is swiftshader loaded anywhere? (bad sign) ==="
ls /system/lib64/egl/ 2>&1
grep -l swiftshader /proc/*/maps 2>/dev/null | head -5
echo
echo "=== which GL libs has surfaceflinger actually mapped ==="
SF=$(pidof surfaceflinger)
grep -oE '/[^ ]*(adreno|swiftshader|libGLES|libEGL|vulkan)[^ ]*' /proc/$SF/maps 2>/dev/null | sort -u
echo
echo "=== composer / gralloc HALs ==="
lshal 2>/dev/null | grep -iE 'graphics.composer|graphics.mapper|graphics.allocator|renderscript'
echo
echo "=== kgsl (adreno kernel driver) ==="
ls -l /dev/kgsl-3d0 2>&1
cat /sys/class/kgsl/kgsl-3d0/gpu_model 2>&1
cat /sys/class/kgsl/kgsl-3d0/gpubusy 2>&1
cat /sys/class/kgsl/kgsl-3d0/gpuclk 2>&1
echo
echo "=== display config as the framework sees it ==="
dumpsys display 2>/dev/null | grep -iE 'DisplayDeviceInfo|mBaseDisplayInfo|real [0-9]|density' | head -12
wm size; wm density
echo DONE
