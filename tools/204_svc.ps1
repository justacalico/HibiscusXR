# pvr_EnterVrMode takes its ERROR branch: the virtual call on the VR service
# client (vtable+0x20) returns non-zero, so the hmdInfo struct is never allocated
# and Pico's error path then dereferences the uninitialised x28. Find what the
# service side reported at that moment.
$f = 'F:\PN2Lineage\notes\184_render.log'
Write-Host "=== everything from pvrservice + VrApi around EnterVrMode ==="
$lines = Get-Content $f
$idx = ($lines | Select-String -Pattern 'pvr_EnterVrMode' | Select-Object -First 1).LineNumber
Write-Host ("EnterVrMode at line " + $idx)
$from = [Math]::Max(0, $idx - 45)
$lines[$from..([Math]::Min($lines.Count-1, $idx + 12))] |
  Where-Object { $_ -match 'PvrService|VrApi|VrServiceApi|PvrServiceClient|UnityPlugin|Distortion|hmdInfo|VRModeState' } |
  ForEach-Object { Write-Host ("  " + $_) }

Write-Host ""
Write-Host "=== STOCK: what it prints right after the banner ==="
$s = Get-Content 'F:\PN2Lineage\notes\168_stock_early.log'
$si = ($s | Select-String -Pattern 'pvr_EnterVrMode' | Select-Object -First 1).LineNumber
$s[([Math]::Max(0,$si-12))..([Math]::Min($s.Count-1,$si+16))] |
  Where-Object { $_ -match 'PvrService|VrApi|hmdInfo|VRModeState|Distortion' } |
  ForEach-Object { Write-Host ("  " + $_) }
