#!/system/bin/sh
echo "=== /oem present? ==="
ls -d /oem 2>/dev/null || echo "  no /oem"
mount 2>/dev/null | grep -i oem
echo
echo "=== /oem/priv-app contents ==="
ls /oem/priv-app/ 2>/dev/null
echo
echo "=== provision2d ==="
ls -laR /oem/priv-app/provision2d/ 2>/dev/null
echo
echo "=== package state ==="
dumpsys package com.picovr.provision 2>/dev/null | grep -iE 'codePath|versionName|primaryCpuAbi|enabled=|sharedUser|nativeLibrary'
