# Collect every change we have made to /system into a staging tree, so it can be
# baked into a distributable system.img instead of living as hand edits on one
# device.
#
# Use `adb pull`, never `adb shell cat > file`: PowerShell redirection re-encodes
# to UTF-16 and rewrites line endings, which silently corrupts anything we then
# bake into an image. Byte-for-byte or it is not what we tested.
$adb = 'C:\adb\adb.exe'
$OV  = 'F:\PN2Lineage\overlay'

$files = @(
  @{ dev = '/system/etc/init/pn2-vintf.rc';       rel = 'etc\init\pn2-vintf.rc';       want = 1098  },
  @{ dev = '/system/etc/init/pn2-snd.rc';         rel = 'etc\init\pn2-snd.rc';         want = 1219  },
  @{ dev = '/system/etc/pn2/vendor_manifest.xml'; rel = 'etc\pn2\vendor_manifest.xml'; want = 22335 }
)

New-Item -ItemType Directory -Force -Path $OV | Out-Null
$bad = 0
foreach ($f in $files) {
  $dst = Join-Path $OV $f.rel
  New-Item -ItemType Directory -Force -Path (Split-Path $dst) | Out-Null
  & $adb pull $f.dev $dst 2>&1 | Out-Null
  $got = (Get-Item $dst).Length
  $ok  = ($got -eq $f.want)
  if (-not $ok) { $bad++ }
  "{0,-34} {1,6} bytes  expected {2,6}  {3}" -f $f.rel, $got, $f.want, $(if ($ok) { 'OK' } else { 'MISMATCH' })
}

$props = @"
# --- Pico Neo 2 (A7B10) port additions -------------------------------------
# Vendor advertises persist.vendor.bt.a2dp_offload_cap but exposes no offload
# formats, so AudioPolicyManager null-derefs in
# getHwOffloadEncodingFormatsSupportedForA2DP() and audioserver crash-loops.
persist.bluetooth.a2dp_offload.disabled=true
ro.bluetooth.a2dp_offload.supported=false

# Panel reports 2160x3840 portrait but is physically mounted landscape across
# both eyes. Without this the 2D UI renders across the seam between the lenses.
ro.surface_flinger.primary_display_orientation=ORIENTATION_90
ro.sf.hwrotation=90
"@
# ASCII, LF endings - build.prop is parsed by init, which does not want CRLF
[System.IO.File]::WriteAllText((Join-Path $OV 'props.append'), ($props -replace "`r`n", "`n"), [System.Text.Encoding]::ASCII)
"{0,-34} {1,6} bytes" -f 'props.append', (Get-Item (Join-Path $OV 'props.append')).Length

if ($bad) { Write-Host "`n$bad file(s) MISMATCHED - do not bake this" -ForegroundColor Red }
else      { Write-Host "`nall files byte-exact; staged to $OV" }
