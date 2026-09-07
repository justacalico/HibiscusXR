# lib2dToVr.so exists but its symbol comes back null. On Android 10 the linker
# refuses dlopen of non-public /system libraries from an app namespace, which
# would produce exactly this: dlopen fails -> dlsym null -> called -> pc=0.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'

Write-Host "=== linker / dlopen errors from the last VRShell run ==="
(Select-String -Path F:\PN2Lineage\notes\184_render.log -Pattern 'dlopen|linker|not accessible|namespace|library .* not found|cannot locate' -AllMatches |
  Select-Object -First 25).Line | ForEach-Object { Write-Host ("  " + $_) }

Write-Host ""
Write-Host "=== public.libraries.txt on OURS ==="
(& $adb -s $DEV shell "cat /system/etc/public.libraries.txt 2>/dev/null" 2>&1) | ForEach-Object { Write-Host ("  " + $_) }

Write-Host ""
Write-Host "=== does lib2dToVr.so appear in any namespace config? ==="
(& $adb -s $DEV shell "su -c 'grep -rl 2dToVr /system/etc/ 2>/dev/null'" 2>&1) | ForEach-Object { Write-Host ("  " + $_) }

Write-Host ""
Write-Host "=== STOCK public.libraries.txt (8.1) ==="
& $adb connect 192.168.0.139:5555 2>&1 | Out-Null
(& $adb -s 192.168.0.139:5555 shell "cat /system/etc/public.libraries.txt 2>/dev/null" 2>&1) | ForEach-Object { Write-Host ("  " + $_) }
