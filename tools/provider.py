#!/usr/bin/env python3
# Resolves drivers/*/driver.json into what the image build consumes.
#
# Usage:
#   provider.py list             active driver names (one per line)
#   provider.py steps <phase>    "run<TAB>log<TAB>marker" for image.steps.<phase>
#   provider.py inputs           union of image.inputs across active drivers
#   provider.py show <name>      dump one manifest
#
# Active drivers come from HSVR_DRIVERS (space separated names); unset means
# every drivers/<name>/driver.json in the tree. Step paths under tools/ are
# emitted relative to $PN2_ROOT, everything else inside its driver dir.

import glob
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load():
    sel = os.environ.get("HSVR_DRIVERS", "").split()
    paths = sorted(glob.glob(os.path.join(ROOT, "drivers", "*", "driver.json")))
    mans = {}
    for p in paths:
        m = json.load(open(p))
        m["_dir"] = os.path.dirname(p)
        mans[m["name"]] = m
    names = sel or list(mans)
    out = []
    for n in names:
        if n not in mans:
            sys.exit("provider: no driver.json for driver '%s'" % n)
        out.append(mans[n])
    return out


def resolve(m, path):
    if path.startswith("/"):
        return path
    if path.startswith("tools/"):
        root = os.environ.get("PN2_ROOT")
        if not root:
            sys.exit("provider: PN2_ROOT unset, cannot resolve %s" % path)
        return os.path.join(root, path)
    return os.path.join(m["_dir"], path)


def cmd_steps(phase):
    for m in load():
        for st in m.get("image", {}).get("steps", {}).get(phase, []):
            run = resolve(m, st["run"])
            print("\t".join([run, st.get("log", ""), st.get("marker", "")]))


def cmd_inputs():
    seen = []
    for m in load():
        for i in m.get("image", {}).get("inputs", []):
            if i not in seen:
                seen.append(i)
    for i in seen:
        print(i)


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else "list"
    if cmd == "list":
        for m in load():
            print(m["name"])
    elif cmd == "steps":
        cmd_steps(sys.argv[2])
    elif cmd == "inputs":
        cmd_inputs()
    elif cmd == "show":
        for m in load():
            if m["name"] == sys.argv[2]:
                print(json.dumps(m, indent=2))
                return
        sys.exit("provider: unknown driver '%s'" % sys.argv[2])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
