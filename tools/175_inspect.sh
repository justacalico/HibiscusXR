#!/bin/bash
F=/mnt/f/PN2Lineage/notes/lib64/libpvrservice.so
LOG=/mnt/f/PN2Lineage/notes/175_inspect.txt
exec >"$LOG" 2>&1
ls -l "$F"
echo "=== file type ==="
head -c 20 "$F" | od -An -tx1
echo "=== section headers ==="
readelf -SW "$F" | grep -E 'symtab|dynsym|text|Name' | head
echo "=== dynsym count ==="
nm -D --defined-only "$F" | wc -l
echo "=== symbols containing Status ==="
nm -D --defined-only "$F" | grep -i status
echo "=== all PVR:: exports (first 40) ==="
nm -D --defined-only "$F" | grep PVR | head -40
echo DONE
