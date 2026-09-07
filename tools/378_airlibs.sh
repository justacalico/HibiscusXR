#!/system/bin/sh
# airservice dlopens algorithm libraries by name. libSafetyAreaRecovery.so is
# missing here, which kills the see-through / boundary flow. Find it and every
# sibling algorithm library airservice can ask for.
echo "=== where does libSafetyAreaRecovery.so live? ==="
find /system /vendor -name "libSafetyAreaRecovery.so" 2>/dev/null

echo
echo "=== every algorithm library airservice names ==="
for b in /system/bin/airservice /vendor/bin/airservice; do
  [ -f "$b" ] || continue
  echo "  from $b:"
  strings -a "$b" 2>/dev/null | grep -E '^lib.*\.so$' | sort -u | sed 's/^/    /'
done

echo
echo "=== and from the air libraries themselves ==="
for d in /system/lib64/pvr_air /system/lib64 /vendor/lib64; do
  [ -d "$d" ] || continue
  for f in "$d"/libair*.so "$d"/libAIR*.so; do
    [ -f "$f" ] || continue
    strings -a "$f" 2>/dev/null | grep -E '^lib(Safety|Boundary|Area|Recovery)[A-Za-z]*\.so$'
  done
done | sort -u | sed 's/^/    /'

echo
echo "=== which of those are present here? ==="
for l in libSafetyAreaRecovery.so libBoundaryDetect.so libSafetyArea.so; do
  p=$(find /system /vendor -name "$l" 2>/dev/null | head -1)
  if [ -n "$p" ]; then echo "  PRESENT $l -> $p"; else echo "  MISSING $l"; fi
done

echo
echo "=== boundary data dir the algorithm wants ==="
ls -ld /data/misc/user/0/boundary 2>/dev/null || echo "  /data/misc/user/0/boundary ABSENT"
ls -l /data/misc/user/0/boundary/ 2>/dev/null
