#!/usr/bin/env python3
import argparse
import os
import shutil
import subprocess
import tempfile
import zipfile

import lief

HERE = os.path.dirname(os.path.abspath(__file__))
LIB_NAME = "libhsvrovrp.so"
PLUGIN_NAME = "libOVRPlugin.so"
ABI_DIR = os.path.join("lib", "arm64-v8a")


def find_plugin(root):
    for dirpath, _, files in os.walk(root):
        if PLUGIN_NAME in files:
            return os.path.join(dirpath, PLUGIN_NAME)
    raise SystemExit(f"{PLUGIN_NAME} not found in apk")


def add_needed(path, needed):
    lib = lief.parse(path)
    deps = [d.name if hasattr(d, "name") else d for d in lib.libraries]
    if needed in deps:
        print(f"{needed} already in DT_NEEDED")
        return
    lib.add_library(needed)
    lib.write(path)
    print(f"added DT_NEEDED {needed} -> {os.path.basename(path)}")


def repack(root, out):
    with zipfile.ZipFile(out, "w") as z:
        for dirpath, _, files in os.walk(root):
            for f in files:
                full = os.path.join(dirpath, f)
                rel = os.path.relpath(full, root)
                stored = ((rel.startswith("lib/") and rel.endswith(".so"))
                          or (rel.startswith("assets/")
                              and not rel.startswith("assets/bin/")))
                z.write(full, rel,
                        compress_type=zipfile.ZIP_STORED if stored
                        else zipfile.ZIP_DEFLATED)


def main():
    ap = argparse.ArgumentParser(
        description="Inject libhsvrovrp.so into an OVRPort-patched apk")
    ap.add_argument("input")
    ap.add_argument("output")
    ap.add_argument("--compat-lib", default=os.path.join(HERE, LIB_NAME))
    ap.add_argument("--zipalign", default="zipalign")
    ap.add_argument("--apksigner", default="apksigner")
    ap.add_argument("--ks")
    ap.add_argument("--ks-pass")
    ap.add_argument("--key-pass")
    ap.add_argument("--ks-key-alias")
    args = ap.parse_args()

    tmp = tempfile.mkdtemp(prefix="ovrp-inject-")
    try:
        with zipfile.ZipFile(args.input) as z:
            z.extractall(tmp)

        dst = os.path.join(tmp, ABI_DIR, LIB_NAME)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copy2(args.compat_lib, dst)

        add_needed(find_plugin(tmp), LIB_NAME)

        raw = args.output + ".raw"
        repack(tmp, raw)
        subprocess.run([args.zipalign, "-p", "-f", "4", raw, args.output],
                       check=True)
        os.remove(raw)

        if args.ks:
            cmd = [args.apksigner, "sign", "--ks", args.ks]
            if args.ks_pass:
                cmd += ["--ks-pass", f"pass:{args.ks_pass}"]
            if args.key_pass:
                cmd += ["--key-pass", f"pass:{args.key_pass}"]
            if args.ks_key_alias:
                cmd += ["--ks-key-alias", args.ks_key_alias]
            cmd += [args.output]
            subprocess.run(cmd, check=True)
        print(f"wrote {args.output}")
    finally:
        shutil.rmtree(tmp)


if __name__ == "__main__":
    main()
