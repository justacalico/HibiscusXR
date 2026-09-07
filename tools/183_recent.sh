#!/system/bin/sh
# List tombstones newest-first by mtime with process + signal + top frames.
# (Filenames wrap, so mtime is the only reliable ordering.)
for f in $(ls -t /data/tombstones/tombstone_* 2>/dev/null | head -4); do
  echo "########## $f"
  grep -E "Timestamp|^pid:|^signal|Cause:" "$f"
  grep -E "#0[0-6] pc" "$f"
  echo
done
