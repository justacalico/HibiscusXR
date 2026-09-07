# Same vdex header probe as the python version, without needing WSL.
#
# vdex 010 (Android 8.1) header:
#   magic[4]="vdex", version[4]="010\0", u32 number_of_dex_files, u32 dex_size,
#   u32 verifier_deps_size, u32 quickening_info_size, u32 checksums[n]
#
# quickening_info_size is the number that matters. 0 => the embedded dex is
# plain and we can lift it straight into the apk. Non-zero => the bytecode was
# rewritten with *-quick opcodes and must be reverted first.
$root = if ($args[0]) { $args[0] } else { 'F:\PN2Lineage\pvr_apps' }
$files = Get-ChildItem $root -Recurse -Filter *.vdex -EA SilentlyContinue | Sort-Object Name
if (-not $files) { "no vdex under $root"; exit 1 }

foreach ($f in $files) {
    $d = [System.IO.File]::ReadAllBytes($f.FullName)
    $magic = [System.Text.Encoding]::ASCII.GetString($d[0..3])
    if ($magic -ne 'vdex') { "{0,-36} NOT A VDEX" -f $f.Name; continue }
    $ver   = ([System.Text.Encoding]::ASCII.GetString($d[4..7])) -replace "`0", ''
    $nDex  = [BitConverter]::ToUInt32($d, 8)
    $dexSz = [BitConverter]::ToUInt32($d, 12)
    $deps  = [BitConverter]::ToUInt32($d, 16)
    $quick = [BitConverter]::ToUInt32($d, 20)
    $hdr   = 24 + 4 * $nDex

    # where does the first dex actually start, and what version is it
    $dexOff = -1
    for ($i = $hdr; $i -lt [Math]::Min($d.Length - 8, $hdr + 4096); $i++) {
        if ($d[$i] -eq 0x64 -and $d[$i+1] -eq 0x65 -and $d[$i+2] -eq 0x78 -and $d[$i+3] -eq 0x0A) { $dexOff = $i; break }
    }
    $dexVer = if ($dexOff -ge 0) { ([System.Text.Encoding]::ASCII.GetString($d[($dexOff+4)..($dexOff+7)])) -replace "`0", '' } else { '?' }
    $verdict = if ($quick -eq 0) { 'PLAIN - extract directly' } else { 'QUICKENED - needs unquicken' }

    "{0,-34} v{1} n={2} dex={3,-9} deps={4,-8} quick={5,-9} dexoff={6} dexver={7}  {8}" -f `
        $f.Name, $ver, $nDex, $dexSz, $deps, $quick, $dexOff, $dexVer, $verdict
}
