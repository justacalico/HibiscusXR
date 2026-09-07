#!/bin/bash
set -u
V=/home/justin/pn2/vendor
L=/mnt/f/PN2Lineage/notes/31_fstab.log
exec >"$L" 2>&1

echo "=== vendor fstab files ==="
find "$V" -name 'fstab*' -printf '%p (%s bytes)\n' 2>/dev/null
echo

for f in $(find "$V" -name 'fstab*' 2>/dev/null); do
  echo "################ $f ################"
  cat "$f"
  echo
done

echo "################ encryption flags found ################"
grep -ho -E '(fileencryption|forceencrypt|forcefdeorfbe|encryptable|metadata_encryption)=[^ ,]*' $(find "$V" -name 'fstab*' 2>/dev/null) 2>/dev/null | sort -u
echo

echo "################ keymaster / crypto HALs in vendor ################"
ls "$V"/bin/hw/ 2>/dev/null | grep -iE 'keymaster|gatekeeper|secure'
ls "$V"/lib64/hw/ 2>/dev/null | grep -iE 'keymaster|gatekeeper'
echo
echo "NOTE: 'ice' = Qualcomm Inline Crypto Engine. A generic Android 10 vold"
echo "      often cannot initialise ice FBE against an 8.1 vendor, which hangs"
echo "      boot when /data is fresh and has to be encrypted from scratch."
echo DONE
