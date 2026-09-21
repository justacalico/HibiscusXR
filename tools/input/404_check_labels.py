#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""dc_detect.kl now resolves, so the patched libinput is live. gpio-keys.kl still
falls back, so one of ITS labels is unknown. Check each one, NUL-delimited so
substrings like CAMERA_FOCUS do not give a false positive."""
import re

LIB = PN2_ROOT + "/notes/ourinput64.patched.so"
KL = PN2_ROOT + "/keylayout/gpio-keys.kl"

data = open(LIB, "rb").read()

labels = []
for line in open(KL, encoding="utf-8", errors="replace"):
    line = line.strip()
    if not line.startswith("key "):
        continue
    parts = line.split()
    if len(parts) >= 3:
        labels.append((parts[1], parts[2], parts[3:]))

print(f"{'scancode':>9}  {'label':<18} {'flags':<8} present?")
for scan, label, flags in labels:
    present = data.find(b"\0" + label.encode() + b"\0") >= 0
    print(f"{scan:>9}  {label:<18} {' '.join(flags):<8} {'yes' if present else 'NO  <-- rejects the file'}")
