#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# THE DECISIVE TEST.
# Find Pico's VR runtime binaries in /system and determine what they link against.
# If they only need vendor + NDK/VNDK-stable libs -> portable.
# If they pull libgui/libui/libsurfaceflinger/private libbinder -> welded to this exact system image.
set -u
B=${PN2_ROOT}
SYS=$B/images/snapshot_lun0/system.bin
EX=$B/extracted
LOG=$B/notes/05_vrhunt.log
exec >"$LOG" 2>&1

mkdir -p "$EX/sysdirs" "$EX/vrbins"

echo "############ 1. directory inventory ############"
for d in /bin /xbin /lib64 /lib /etc /etc/init /etc/permissions /priv-app /app /framework; do
  echo "===== $d ====="
  debugfs -R "ls -l $d" "$SYS" 2>/dev/null
  echo
done

echo
echo "############ 2. Pico / VR named entries ############"
for d in /bin /xbin /lib64 /lib /etc /etc/init /priv-app /app /framework; do
  debugfs -R "ls -l $d" "$SYS" 2>/dev/null | awk -v D="$d" '{ $1="";$2="";$3="";$4="";$5="";$6="";$7="";$8="";
    name=$0; gsub(/^ +/,"",name); if (name!="." && name!="..") print D"/"name }'
done > /tmp/sysfiles.txt
sort -u /tmp/sysfiles.txt > "$EX/system_toplevel_files.txt"
wc -l < "$EX/system_toplevel_files.txt"
echo "--- matches ---"
grep -iE 'pvr|pico|vr|xr|tobii|eye|track|imu|sixdof|6dof|slam|holo|hmd' "$EX/system_toplevel_files.txt"

echo
echo "############ 3. extracting candidate binaries ############"
while read -r f; do
  base=$(basename "$f")
  case "$base" in
    *.so|*) ;;
  esac
  out="$EX/vrbins/$base"
  if debugfs -R "dump -p $f $out" "$SYS" >/dev/null 2>&1 && [ -s "$out" ]; then
    echo "pulled  $f  ($(stat -c%s "$out") bytes)"
  else
    rm -f "$out"
  fi
done < <(grep -iE '/(lib64|lib|bin|xbin)/.*(pvr|pico|vr|xr|tobii|eye|track|sixdof|6dof|slam|hmd)' "$EX/system_toplevel_files.txt")

echo
echo "############ 4. DECISIVE: dynamic linkage ############"
for f in "$EX/vrbins"/*; do
  [ -f "$f" ] || continue
  ft=$(file -b "$f" 2>/dev/null | head -1)
  case "$ft" in
    *ELF*) ;;
    *) echo "--- $(basename "$f") : not ELF ($ft)"; continue ;;
  esac
  echo "================================================================"
  echo "FILE: $(basename "$f")"
  echo "TYPE: $ft"
  echo "--- NEEDED ---"
  readelf -d "$f" 2>/dev/null | grep -E 'NEEDED|SONAME' | sed 's/^ *//'
  echo "--- risky imported symbols (system-private surfaces) ---"
  readelf --dyn-syms -W "$f" 2>/dev/null | grep -E 'UND' \
    | grep -iE 'SurfaceFlinger|android4view|BufferQueue|IGraphicBuffer|Surface|GraphicBuffer|IBinder|hwbinder|ISurfaceComposer|DisplayInfo|IDisplayEventConnection' \
    | awk '{print $8}' | sort -u | head -40
  echo
done

echo
echo "############ 5. init rc referencing VR services ############"
debugfs -R "ls -l /etc/init" "$SYS" 2>/dev/null | tail -n +3 | awk '{print $NF}' | while read -r rc; do
  case "$rc" in .|..) continue;; esac
  debugfs -R "dump -p /etc/init/$rc" "$SYS" 2>/dev/null > /tmp/rc.txt
  if grep -qiE 'pvr|pico|vr|tobii|track' /tmp/rc.txt 2>/dev/null; then
    echo "===== /etc/init/$rc ====="
    cat /tmp/rc.txt
    echo
  fi
done

echo
echo "############ 6. system manifest.xml (HAL inventory) ############"
debugfs -R "dump -p /manifest.xml $EX/props/system-manifest.xml" "$SYS" 2>/dev/null
cat "$EX/props/system-manifest.xml" 2>/dev/null

echo
echo "VRHUNT DONE"
