param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
& $adb connect $Dev 2>&1 | Out-Null
$sh = @(
  'echo "--- live trackingstate / pose ---"',
  'logcat -d | grep -iE "trackingstate|Bad Pose|kLostDialog|CalculateDialogState" | tail -8',
  'echo "  (empty above = no lost-tracking path taken)"',
  'echo "--- is the compositor submitting frames? ---"',
  'logcat -d | grep -iE "TimeWarp|submitFrame|BeginFrame|EndFrame|fps|Frame rate" | tail -10',
  'echo "--- focused window ---"',
  'dumpsys window | grep -E "mCurrentFocus|mFocusedApp"',
  'echo "--- unity scene / shell activity ---"',
  'logcat -d | grep -iE "Unity|VrShell|Home|Scene|Boundary|Seethrough|SafetyArea" | tail -14',
  'echo "--- vr mode owner ---"',
  'logcat -d | grep -iE "VR mode|EnterVrMode|LeaveVrMode" | tail -6'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_look.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_look.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_look.sh'" 2>&1) | ForEach-Object { Write-Host ("  " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }
& $adb -s $Dev shell "screencap -p /sdcard/pn2_vr.png" 2>&1 | Out-Null
& $adb -s $Dev pull /sdcard/pn2_vr.png F:\PN2Lineage\notes\pn2_vr.png 2>&1 | Out-Null
