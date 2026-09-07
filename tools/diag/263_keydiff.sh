#!/system/bin/sh
echo "=== input devices and their keys ==="
getevent -pl 2>/dev/null | grep -iE '^add device|name:|KEY_' | head -50
echo
echo "=== keylayout files NOT part of stock AOSP (pico-specific) ==="
ls /system/usr/keylayout/ 2>/dev/null | grep -viE '^(AVRCP|Generic|qwerty|Vendor_)'
echo
echo "=== /vendor keylayouts (vendor is untouched) ==="
ls /vendor/usr/keylayout/ 2>/dev/null
for f in /vendor/usr/keylayout/*.kl; do echo "--- $f ---"; cat "$f"; done 2>/dev/null | head -40
