#!/bin/bash
# Recon toolchain setup. Idempotent.
set -u
LOG=/mnt/f/PN2Lineage/notes/00_setup.log
exec >"$LOG" 2>&1

echo "=== distro ==="
cat /etc/os-release | head -3
echo
echo "=== sudo check ==="
if sudo -n true 2>/dev/null; then echo "passwordless sudo OK"; else echo "SUDO NEEDS PASSWORD"; fi
echo
echo "=== pre-existing tools ==="
for t in python3 pip3 brotli dtc readelf objdump strings unzip file simg2img mkfs.ext4 debugfs git cpio lz4 xz gzip; do
  printf '%-12s' "$t"
  if command -v "$t" >/dev/null 2>&1; then echo "present  $(command -v "$t")"; else echo "MISSING"; fi
done
echo

echo "=== installing missing packages ==="
export DEBIAN_FRONTEND=noninteractive
sudo -n apt-get update -qq
sudo -n apt-get install -y -qq \
  python3 python3-pip brotli device-tree-compiler binutils \
  e2fsprogs android-sdk-libsparse-utils unzip file cpio lz4 xz-utils \
  p7zip-full 2>&1 | tail -20
echo "apt exit: $?"
echo

echo "=== post-install verify ==="
for t in python3 brotli dtc readelf simg2img debugfs 7z; do
  printf '%-12s' "$t"
  if command -v "$t" >/dev/null 2>&1; then echo "present"; else echo "STILL MISSING"; fi
done
echo
echo "=== workspace ==="
ls -la /mnt/f/PN2Lineage/
echo
ls -la /mnt/f/PN2Lineage/images/ota_4.1.3/
echo "SETUP DONE"
