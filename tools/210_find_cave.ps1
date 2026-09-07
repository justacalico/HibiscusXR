# Find executable padding in the app's libPvr_UnitySDK.so big enough for a small
# trampoline.
#
# pvr_EnterVrMode does `bl SetClientStatusCallback` and then `ldr x0,[x28,#8]`,
# but x28 comes back 0. Since that call is a direct intra-library branch, LD_PRELOAD
# cannot interpose it. Instead redirect the bl to a trampoline that saves x28,
# calls the real target, restores x28, and returns:
#
#     stp x28, x30, [sp, #-16]!
#     bl  <real target>
#     ldp x28, x30, [sp], #16
#     ret
#
# That is 16 bytes, so any 32-byte run of alignment padding inside an executable
# section will do.
$NDK = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin'
$so  = 'F:\PN2Lineage\notes\vrshell_lib\libPvr_UnitySDK.so'

Write-Host "=== executable sections ==="
& "$NDK\llvm-readelf.exe" -SW $so 2>&1 |
  Where-Object { $_ -match '\sAX\s|Name' } |
  ForEach-Object { Write-Host ("  " + $_.Trim()) }

$bytes = [System.IO.File]::ReadAllBytes($so)
Write-Host ""
Write-Host "=== hunting for >=32 byte runs of 0x00 in .text ==="

# .text bounds from readelf above; parse them properly
$sec = & "$NDK\llvm-readelf.exe" -SW $so 2>&1 | Where-Object { $_ -match '\.text' } | Select-Object -First 1
if ($sec -match '\.text\s+\S+\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)') {
    $addr = [Convert]::ToInt64($matches[1],16)
    $off  = [Convert]::ToInt64($matches[2],16)
    $size = [Convert]::ToInt64($matches[3],16)
    Write-Host ("  .text vaddr={0:x} off={1:x} size={2:x}" -f $addr,$off,$size)

    $runs = @()
    $start = -1
    for ($i = $off; $i -lt ($off + $size); $i++) {
        if ($bytes[$i] -eq 0) {
            if ($start -lt 0) { $start = $i }
        } else {
            if ($start -ge 0 -and ($i - $start) -ge 32) {
                $runs += [pscustomobject]@{
                    FileOff = $start
                    VAddr   = $addr + ($start - $off)
                    Len     = $i - $start
                }
            }
            $start = -1
        }
    }
    Write-Host ("  found {0} candidate caves" -f $runs.Count)
    $runs | Sort-Object -Property Len -Descending | Select-Object -First 8 |
      ForEach-Object { Write-Host ("    vaddr 0x{0:x}  fileoff 0x{1:x}  {2} bytes" -f $_.VAddr, $_.FileOff, $_.Len) }
} else {
    Write-Host "  could not parse .text header"
}
