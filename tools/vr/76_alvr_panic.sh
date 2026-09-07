#!/system/bin/sh
# ALVR aborts in alvr_initialize_opengl. The Rust panic message is printed just
# before the backtrace; grab the surrounding context plus anything EGL/GL
# related, since that is what the panic is about.
echo "=== panic message + preceding context ==="
logcat -d 2>/dev/null | grep -B 12 'NATIVE-RUST.*Backtrace' | tail -25
echo
echo "=== everything ALVR logged this run ==="
logcat -d 2>/dev/null | grep -E 'P2ALVR|ALVR' | grep -vE 'Backtrace|^\s+[0-9]+: ' | tail -30
echo
echo "=== EGL / GL / gralloc complaints ==="
logcat -d 2>/dev/null | grep -iE 'eglCreate|EGL_|Gralloc|mapper@|GLES|libEGL|eglChoose|eglMakeCurrent|surface' | tail -25
echo DONE
