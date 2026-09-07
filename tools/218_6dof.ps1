# What is missing for 6DoF?
#
# The shell renders but reports "movement tracking lost". pvrservice logs
# "force to 3dof by wake up" and reports a position that never changes, so the
# head tracker is rotation-only. 6DoF on this device is visual SLAM: cameras ->
# ORB features -> pose. Check every piece of that chain against stock.
$adb   = 'C:\adb\adb.exe'
$OURS  = '192.168.0.172:5555'
$STOCK = '192.168.0.139:5555'
& $adb connect $OURS 2>&1 | Out-Null
& $adb connect $STOCK 2>&1 | Out-Null

$probe = 'echo "--- ORB vocabulary / SLAM data ---"; ls -l /system/etc/pvr/ 2>/dev/null | head -20; echo "--- 6dof libs ---"; ls /system/lib64/ /system/lib/ 2>/dev/null | grep -iE "6dof|slam|orb|sixdof|tracking|fusion" | sort -u; echo "--- tracking related services ---"; ps -A 2>/dev/null | grep -iE "pvr|cv|track|qvr|slam" ; echo "--- 6dof props ---"; getprop | grep -iE "6dof|track|slam|pxr.*dof" | head -20'

foreach ($pair in @(@('OURS',$OURS), @('STOCK',$STOCK))) {
  Write-Host ""
  Write-Host ("######## " + $pair[0])
  (& $adb -s $pair[1] shell "su -c '$probe'" 2>&1) | ForEach-Object { Write-Host ("  " + $_) }
}
