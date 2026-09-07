#!/usr/bin/env python3
"""
Inject the unquickened dex into the /oem apks (python zipfile append, never
.NET ZipArchive Update - that corrupted every apk last time).
"""
import os
import shutil
import sys
import zipfile

SRC = "/mnt/f/PN2Lineage/oem_apps"
DEX = "/mnt/f/PN2Lineage/oem_dex"
OUT = "/mnt/f/PN2Lineage/oem_injected"

WANT = ["PVRLauncher", "PVRHome", "store2d", "provision2d", "ToBToolService"]


def find_apk(app):
    d = os.path.join(SRC, app)
    for root, _dirs, files in os.walk(d):
        for f in files:
            if f.endswith(".apk"):
                return os.path.join(root, f)
    return None


def main():
    shutil.rmtree(OUT, ignore_errors=True)
    os.makedirs(OUT, exist_ok=True)
    for app in WANT:
        apk = find_apk(app)
        dex = os.path.join(DEX, app, f"{app}_classes.dex")
        if not apk or not os.path.exists(dex):
            print(f"  {app:18} MISSING apk or dex")
            continue
        dst_dir = os.path.join(OUT, app)
        os.makedirs(dst_dir, exist_ok=True)
        dst = os.path.join(dst_dir, os.path.basename(apk))
        shutil.copyfile(apk, dst)
        with zipfile.ZipFile(dst, "a", zipfile.ZIP_DEFLATED) as z:
            if "classes.dex" not in z.namelist():
                z.write(dex, "classes.dex")
        with zipfile.ZipFile(dst) as z:
            ok = z.testzip() is None and "AndroidManifest.xml" in z.namelist()
            sz = z.getinfo("classes.dex").file_size
        print(f"  {app:18} {os.path.getsize(dst)//1024:7} KB  dex={sz:<9} ok={ok}")

        # carry the app-private libs across too
        libsrc = os.path.join(SRC, app, "lib")
        if os.path.isdir(libsrc):
            shutil.copytree(libsrc, os.path.join(dst_dir, "lib"))
            n = sum(len(f) for _r, _d, f in os.walk(os.path.join(dst_dir, "lib")))
            print(f"  {'':18} + {n} app-private libs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
