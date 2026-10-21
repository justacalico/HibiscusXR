# Shared debugfs helpers for the system image build scripts.
#
# Sourced by tools/build/*.sh. The caller sets IMG to the ext4 image path
# and fail=0 before first use - put/mkd/img_fsck increment fail on error.
#
# debugfs splits every request's arguments on whitespace and has no
# quoting, so a path containing a space would rm/write the wrong inode.
# Every helper here rejects whitespace-bearing paths instead.
#
# Verify every write by size - debugfs reports success even when it
# silently refuses to overwrite an existing file, which is why put()
# re-reads the listing.

img_no_ws() { # img_no_ws <path...>: rc 1 when any arg has whitespace
  local p
  for p in "$@"; do
    case "$p" in *[[:space:]]*) return 1 ;; esac
  done
}

put() {   # put <local> <img-path> <mode>
  local src="$1" dst="$2" mode="$3"
  local dir base want got
  dir=$(dirname "$dst"); base=$(basename "$dst")
  if ! img_no_ws "$src" "$dst"; then
    printf '  FAIL  %-52s whitespace in path\n' "$dst"
    fail=$((fail+1)); return
  fi
  [ -f "$src" ] || { printf '  MISSING SOURCE %s\n' "$src"; fail=$((fail+1)); return; }
  debugfs -w -R "rm $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "write $src $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst mode 0100$mode" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst gid 0" "$IMG" >/dev/null 2>&1
  want=$(stat -c%s "$src")
  # debugfs `ls -l` columns: inode mode (type) uid gid SIZE date time name,
  # so size is field 6
  got=$(debugfs -R "ls -l $dir" "$IMG" 2>/dev/null | awk -v b="$base" '$NF==b {print $6}' | head -1)
  if [ "$got" = "$want" ]; then
    printf '  OK    %-52s %12s\n' "$dst" "$got"
  else
    printf '  FAIL  %-52s wrote %s, image says "%s"\n' "$dst" "$want" "${got:-absent}"
    fail=$((fail+1))
  fi
}

mkd() {   # mkd <img-dir>
  if ! img_no_ws "$1"; then
    printf '  FAIL  %-52s whitespace in path\n' "$1"
    fail=$((fail+1)); return
  fi
  debugfs -w -R "mkdir $1" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $1 mode 040755" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $1 uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $1 gid 0" "$IMG" >/dev/null 2>&1
}

# Batch writes: append rm/write/sif commands for one file to IMG_CMDS,
# then run them all in a single debugfs -f session with img_run_cmds.
img_cmd_put() { # img_cmd_put <local> <img-path> [mode]
  local src="$1" dst="$2" mode="${3:-644}"
  if ! img_no_ws "$src" "$dst"; then
    printf '  FAIL  %-52s whitespace in path\n' "$dst" >&2
    return 1
  fi
  printf 'rm %s\nwrite %s %s\nsif %s mode 0100%s\nsif %s uid 0\nsif %s gid 0\n' \
      "$dst" "$src" "$dst" "$dst" "$mode" "$dst" "$dst" >> "$IMG_CMDS"
}

img_run_cmds() { # img_run_cmds: run $IMG_CMDS in one debugfs -f session
  debugfs -w -f "$IMG_CMDS" "$IMG" >/dev/null 2>&1
}

# debugfs exits 0 even when a path does not resolve - the only signal
# is the "File not found by ext2_lookup" text it prints.
img_exists() { # img_exists <img-path>
  ! debugfs -R "stat $1" "$IMG" 2>&1 | grep -q ext2_lookup
}

# debugfs `ls -l` fields: inode mode (links) uid gid size date time name.
img_ls() { # img_ls <img-dir> -> "mode name" per entry
  debugfs -R "ls -l $1" "$IMG" 2>/dev/null | \
    awk 'NF >= 8 && $2 ~ /^[0-9]+$/ {print $2, $NF}'
}

rmrf() { # rmrf <img-dir>: delete a directory tree inside the image
  local d="$1" mode name
  img_ls "$d" | while read -r mode name; do
    case "$name" in .|..) continue ;; esac
    case "$mode" in
      4*|04*) rmrf "$d/$name" ;;
      *)      debugfs -w -R "rm $d/$name" "$IMG" >/dev/null 2>&1 ;;
    esac
  done
  debugfs -w -R "rmdir $d" "$IMG" >/dev/null 2>&1
}

# debugfs write/rm leaves the accounting inconsistent - repair, then
# verify clean. rc 0 clean, 1 dirty.
img_fsck() {
  local out; out=$(mktemp)
  e2fsck -fy "$IMG" >"$out" 2>&1
  tail -4 "$out"
  if e2fsck -fn "$IMG" >"$out" 2>&1; then
    tail -2 "$out"; echo "  fsck CLEAN"
    rm -f "$out"; return 0
  fi
  tail -8 "$out"; echo "  fsck DIRTY"
  rm -f "$out"; return 1
}
