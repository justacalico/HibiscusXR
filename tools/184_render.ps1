# Why does the Pico render thread never initialize?
#
# Stock:  InitRenderThreadRoutine after a wait -> PVR_InitRenderThread() -> pvr_EnterVrMode
# Ours:   "Get sensor failed. Render thread not initialized." then a call through
#         a null function pointer (pc = 0x0) on UnityMain.
#
# Unity drives this with GL.IssuePluginEvent(GetRenderEventFunc(), 1024) on the
# render thread, so a null GetRenderEventFunc would land exactly on pc=0. Capture
# Unity's own crash backtrace (it handles the signal itself, so there is no
# tombstone) plus the surface/EGL markers the render thread depends on.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$out = 'F:\PN2Lineage\notes\184_render.log'

& $adb -s $DEV shell "am force-stop com.pvr.vrshell" 2>&1 | Out-Null
& $adb -s $DEV shell "logcat -G 16M" 2>&1 | Out-Null
Start-Sleep -Seconds 2
& $adb -s $DEV shell "logcat -c" 2>&1 | Out-Null
& $adb -s $DEV shell "am start -n com.pvr.vrshell/.MainActivity" 2>&1 | Out-Null
Start-Sleep -Seconds 16

$all = & $adb -s $DEV shell "logcat -d -v brief" 2>&1
Set-Content $out ($all -join "`n")
Write-Host ("captured " + $all.Count + " lines -> $out")

Write-Host ""
Write-Host "=== surface / EGL / render-thread markers ==="
$all | Where-Object { $_ -match 'nativesurface|surfaceCreated|surfaceChanged|EGL|egl|InitRenderThread|RenderEventFunc|IssuePlugin|EnterVrMode|SetEyeBuffer|Surface' } |
    Select-Object -First 40 | ForEach-Object { Write-Host ("  " + $_) }

Write-Host ""
Write-Host "=== Unity's own crash backtrace (no tombstone: Unity traps it) ==="
$all | Where-Object { $_ -match 'E/CRASH' } | Select-Object -First 60 | ForEach-Object { Write-Host ("  " + $_) }

Write-Host ""
Write-Host "=== last 15 UnityPlugin / VrApi lines before the crash ==="
$all | Where-Object { $_ -match 'UnityPlugin|VrApi|PVRShell' } | Select-Object -Last 15 | ForEach-Object { Write-Host ("  " + $_) }
