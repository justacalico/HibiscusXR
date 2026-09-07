# Full partition backup of the non-Eye Pico Neo 2 (PA7B40NGE5300009W).
# dd -> /sdcard -> adb pull -> delete on device, one partition at a time so
# device storage never holds more than one image.
$ErrorActionPreference = 'Continue'
$adb  = 'C:\adb\adb.exe'
$DEAD = 'PA7B40NGE5300009W'
$dst  = 'F:\PN2Lineage\backup_nonEye'
$log  = 'F:\PN2Lineage\notes\backup_nonEye.log'

function Log($m) { $line = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; $line; Add-Content -Path $log -Value $line }

New-Item -ItemType Directory -Force -Path $dst | Out-Null
Set-Content -Path $log -Value "backup started $(Get-Date)"

# hard guard: never back up (or touch) the wrong unit
$s = @((& $adb devices) | Select-String '\sdevice$' | ForEach-Object { ($_ -split '\s+')[0] })
if ($s.Count -ne 1)  { Log "ABORT: expected 1 device, found $($s.Count)"; exit 1 }
if ($s[0] -ne $DEAD) { Log "ABORT: wrong device $($s[0])"; exit 1 }
Log "device OK: $($s[0])"

# Ordered most-irreplaceable first, so a mid-run disconnect still saves what matters.
$parts = @(
  'persist','picocfg','ndi_placeholder_skip',
  'modemst1','modemst2','fsg','fsc',
  'boot','dtbo','vbmeta','recovery',
  'misc','frp','keystore','ssd','devinfo','sec','cdt','ddr',
  'xbl','xbl_config','abl','aop','tz','hyp','keymaster','cmnlib','cmnlib64',
  'devcfg','qupfw','storsec','ImageFv','dip','apdp','msadp','limits','spunvm',
  'sti','toolsfv','logfs','splash','mdtp','mdtpsecapp',
  'bluetooth','dsp','modem',
  'vendor','oem','system'
) | Where-Object { $_ -ne 'ndi_placeholder_skip' }

& $adb shell "su -c 'mkdir -p /sdcard/bk'" | Out-Null

$ok = 0; $fail = 0
foreach ($p in $parts) {
  $out = Join-Path $dst "$p.img"
  if (Test-Path $out) { Log "skip $p (already have it)"; $ok++; continue }

  # toybox dd on 8.1 has no size suffixes; bytes only
  $r = (& $adb shell "su -c 'dd if=/dev/block/bootdevice/by-name/$p of=/sdcard/bk/$p.img bs=1048576 2>&1; echo RC=`$?'") -join ' '
  if ($r -notmatch 'RC=0') { Log "FAIL dd $p : $r"; $fail++; continue }

  & $adb pull "/sdcard/bk/$p.img" $out 2>&1 | Out-Null
  & $adb shell "su -c 'rm -f /sdcard/bk/$p.img'" | Out-Null

  if (Test-Path $out) {
    $mb = [math]::Round((Get-Item $out).Length/1MB, 2)
    Log ("OK   {0,-14} {1,10} MB" -f $p, $mb)
    $ok++
  } else {
    Log "FAIL pull $p"; $fail++
  }
}

& $adb shell "su -c 'rmdir /sdcard/bk'" | Out-Null
$tot = [math]::Round(((Get-ChildItem $dst -File | Measure-Object Length -Sum).Sum)/1GB, 2)
Log "DONE  ok=$ok fail=$fail  total=$tot GB  -> $dst"
