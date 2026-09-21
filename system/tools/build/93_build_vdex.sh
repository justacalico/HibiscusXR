#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Build vdexExtractor.
#
# Every Pico vdex is quickened (quickening_info_size != 0), so the embedded dex
# cannot simply be lifted out: the bytecode uses *-quick opcodes that encode
# vtable and field OFFSETS resolved against Pico's 8.1 boot image. They have to
# be reverted to normal opcodes with real indices, which is what this tool does
# with --unquicken (or --deps for the dependency info).
#
# Reimplementing that ourselves means walking every code item in every dex and
# rewriting opcodes against the quickening stream - a few hundred lines of
# fiddly, easy-to-get-subtly-wrong work, for something that already exists.
set -e
WORK=/home/justin/vdextools
LOG=${PN2_ROOT}/notes/93_vdex_build.txt
exec >"$LOG" 2>&1

mkdir -p "$WORK"
cd "$WORK"

if [ ! -d vdexExtractor ]; then
  echo "=== cloning ==="
  git clone --depth 1 https://github.com/anestisb/vdexExtractor.git
fi
cd vdexExtractor
echo "commit: $(git rev-parse --short HEAD)"
echo

echo "=== building ==="
if [ -x ./make.sh ]; then
  ./make.sh
else
  make
fi
echo

echo "=== result ==="
ls -l bin/ 2>/dev/null || ls -l ./vdexExtractor 2>/dev/null
find . -maxdepth 2 -type f -perm -u+x -name 'vdexExtractor*' -exec ls -l {} \;
echo DONE
