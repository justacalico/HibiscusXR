# Catch a reboot in the act.
#
# There is no pstore and no bootreason on this kernel, so the only way to learn
# why it dies is to be holding the log when it happens. Reconnect after every
# drop and keep appending, so whatever precedes the next reboot is on disk.
#
# Also records each boot's start so reboots are countable after the fact.
$adb  = 'C:\adb\adb.exe'
$log  = 'F:\PN2Lineage\notes\reboot_watch.log'
$mark = 'F:\PN2Lineage\notes\reboot_marks.log'

"=== watch started $(Get-Date) ===" | Out-File $mark -Append -Encoding utf8

while ($true) {
    & $adb wait-for-device 2>&1 | Out-Null
    $up = ((& $adb shell cut -d. -f1 /proc/uptime 2>&1) -join '').Trim()
    $bc = ((& $adb shell getprop sys.boot_completed 2>&1) -join '').Trim()
    $msg = "[{0}] CONNECTED uptime={1}s boot_completed={2}" -f (Get-Date -f 'HH:mm:ss'), $up, $bc
    $msg | Out-File $mark -Append -Encoding utf8
    Write-Host $msg

    "`n########## session $(Get-Date -f 'HH:mm:ss') uptime=${up}s ##########" |
        Out-File $log -Append -Encoding utf8

    # blocks until the device goes away
    & $adb logcat -v threadtime 2>&1 | Out-File $log -Append -Encoding utf8

    $msg = "[{0}] DISCONNECTED (device went down)" -f (Get-Date -f 'HH:mm:ss')
    $msg | Out-File $mark -Append -Encoding utf8
    Write-Host $msg
    Start-Sleep -Seconds 2
}
