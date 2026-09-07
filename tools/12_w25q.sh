#!/bin/bash
# The flasher binary is the authoritative answer for how to reach the FPGA's SPI flash.
set -u
T=/mnt/f/PN2Lineage/ndi_firmware/EYE_pui4.1.3_firehose/w25q_write_bin
L=/mnt/f/PN2Lineage/notes/12_w25q.log
exec >"$L" 2>&1

echo "=== identity ==="
file -b "$T"
ls -l "$T"
echo
echo "=== NEEDED ==="
readelf -d "$T" 2>/dev/null | grep NEEDED | sed 's/^ *//'
echo
echo "=== imported functions (what syscalls/APIs it uses) ==="
readelf --dyn-syms -W "$T" 2>/dev/null | awk '$7=="UND" && $4=="FUNC" {print $8}' | sort -u
echo
echo "=== ALL strings (binary is 11KB, nothing withheld) ==="
strings -n 3 "$T" | sort -u
echo
echo "=== strings that look like paths or devices ==="
strings -n 3 "$T" | grep -E '^/|dev|spi|mtd|sys|proc|\.bin' | sort -u
echo DONE
