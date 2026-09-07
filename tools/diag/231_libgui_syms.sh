#!/system/bin/sh
# Does Q's libgui still provide a setName we can forward to?
# BufferItemConsumer derives from ConsumerBase via single inheritance, so if
# ConsumerBase::setName exists the `this` pointer is directly compatible and a
# tail-branch shim is enough.
echo "=== setName symbols in our libgui ==="
grep -c . /dev/null 2>/dev/null
for f in /system/lib64/libgui.so; do
  echo "--- $f ---"
  strings -a "$f" 2>/dev/null | grep -E '_ZN7android(12ConsumerBase|18BufferItemConsumer)7setName' | sort -u
done
echo
echo "=== what libaircamera actually needs from libgui ==="
strings -a /system/lib64/pvr_air/libaircamera.so 2>/dev/null | grep -E '^_ZN7android' | grep -iE 'consumer|surface|buffer' | sort -u | head -30
