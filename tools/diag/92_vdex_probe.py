#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""
Read the vdex headers before committing to a tool.

vdex version 010 (Android 8.1) header:
    uint8  magic[4]            "vdex"
    uint8  version[4]          "010\0"
    uint32 number_of_dex_files
    uint32 dex_size
    uint32 verifier_deps_size
    uint32 quickening_info_size
    uint32 checksums[number_of_dex_files]
    ... dex files (dex_size bytes total) ...
    ... verifier deps ...
    ... quickening info ...

The number that decides everything is quickening_info_size. If it is 0 the
embedded dex files are plain, unquickened dex and we can lift them straight out
and drop them into the apk. If it is non-zero the bytecode has been rewritten
with *-quick opcodes that only make sense against the original boot image, and
they have to be reverted using the quickening info - which is what vdexExtractor
does and what we would otherwise have to reimplement.
"""
import glob
import os
import struct
import sys

DEX_MAGIC = b"dex\n"


def probe(path):
    with open(path, "rb") as f:
        data = f.read()

    magic = data[0:4]
    version = data[4:8].rstrip(b"\0").decode(errors="replace")
    if magic != b"vdex":
        return f"{os.path.basename(path):38} NOT A VDEX ({magic!r})"

    n_dex, dex_size, deps_size, quick_size = struct.unpack_from("<IIII", data, 8)
    hdr = 24 + 4 * n_dex

    # sanity: count dex magics actually present
    dex_count = data.count(DEX_MAGIC)
    first = data.find(DEX_MAGIC)
    dex_ver = ""
    if first >= 0:
        dex_ver = data[first + 4:first + 8].rstrip(b"\0").decode(errors="replace")

    verdict = "PLAIN DEX - can extract directly" if quick_size == 0 \
              else "QUICKENED - needs unquickening"
    return (f"{os.path.basename(path):38} v{version} dexfiles={n_dex} "
            f"dex_size={dex_size} deps={deps_size} quick={quick_size:<8} "
            f"hdr={hdr} dexmagics={dex_count} dexver={dex_ver}  {verdict}")


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else PN2_ROOT + "/pvr_apps"
    files = sorted(glob.glob(os.path.join(root, "**", "*.vdex"), recursive=True))
    if not files:
        print("no vdex files found under", root)
        return 1
    for p in files:
        print(probe(p))
    return 0


if __name__ == "__main__":
    sys.exit(main())
