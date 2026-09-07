#!/bin/bash
# Get an ARM disassembler in WSL. Debian's Python is PEP668-managed, so use a venv
# rather than --break-system-packages.
set -u
LOG=/mnt/f/PN2Lineage/notes/156_getcap.txt
exec >"$LOG" 2>&1
VENV=$HOME/.pn2venv
if [ ! -x "$VENV/bin/python" ]; then
  python3 -m venv "$VENV" 2>&1 | tail -3
fi
"$VENV/bin/pip" install --quiet capstone 2>&1 | tail -5
"$VENV/bin/python" - <<'PY'
try:
    import capstone
    print("capstone OK", capstone.__version__)
except Exception as e:
    print("capstone MISSING:", e)
PY
echo DONE
