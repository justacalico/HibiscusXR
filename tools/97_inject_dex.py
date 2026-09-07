#!/usr/bin/env python3
"""
Inject the unquickened classes.dex into each Pico APK.

Do NOT use .NET's ZipArchive 'Update' mode for this. It rewrites the whole
archive and corrupted every apk - aapt2 rejected them with
    Zip: size/crc32 mismatch. expected {679, 1864, d296ada9} was {524308, 8, ...}
    failed to open AndroidManifest.xml
because the local headers and central directory ended up disagreeing.

Python's zipfile in append mode leaves the existing entries byte for byte and
just appends, which is what we want. Signing happens afterwards.
"""
import os
import shutil
import sys
import zipfile

APPS = "/mnt/f/PN2Lineage/pvr_apps"
DEX = "/mnt/f/PN2Lineage/pvr_dex"
OUT = "/mnt/f/PN2Lineage/pvr_apps_injected"

WANT = [
    "PicoSettingsProvider", "configserverservice", "CVService", "pvrdisplay",
    "pvr_adapter", "PxrNotification", "PVRVerify", "ShortcutMenu", "VRShell2",
    "PicoToSvrService", "InitServer", "VRUserCenter2",
]


def find_apk(name):
    for root, _dirs, files in os.walk(APPS):
        if os.path.basename(root) == name:
            for f in files:
                if f.endswith(".apk"):
                    return os.path.join(root, f)
    return None


def main():
    shutil.rmtree(OUT, ignore_errors=True)
    os.makedirs(OUT, exist_ok=True)

    for name in WANT:
        apk = find_apk(name)
        dex = os.path.join(DEX, name, f"{name}_classes.dex")
        if not apk or not os.path.exists(dex):
            print(f"  {name:24} MISSING apk or dex")
            continue

        dst_dir = os.path.join(OUT, name)
        os.makedirs(dst_dir, exist_ok=True)
        dst = os.path.join(dst_dir, os.path.basename(apk))
        shutil.copyfile(apk, dst)

        with zipfile.ZipFile(dst, "a", zipfile.ZIP_DEFLATED) as z:
            existing = set(z.namelist())
            if "classes.dex" in existing:
                print(f"  {name:24} already has classes.dex, skipping inject")
            else:
                z.write(dex, "classes.dex")

        # verify: the archive must still open cleanly and contain the manifest
        try:
            with zipfile.ZipFile(dst) as z:
                bad = z.testzip()
                names = z.namelist()
                has_mf = "AndroidManifest.xml" in names
                dex_sz = z.getinfo("classes.dex").file_size
            ok = (bad is None) and has_mf
            print(f"  {name:24} {os.path.getsize(dst)//1024:8} KB  "
                  f"dex={dex_sz:<9} manifest={has_mf} integrity={'OK' if ok else 'BAD'}")
        except Exception as e:  # noqa: BLE001
            print(f"  {name:24} VERIFY FAILED: {e}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
