# Build libshim_air.so - the libgui/libui ABI shims for the 8.1 passthrough stack.
#
# Linked against the DEVICE's own libgui/libui/libutils so lld records the right
# SONAMEs and can resolve the forward targets. --allow-shlib-undefined because
# their transitive deps are not staged here and we only need their symbol tables.
$ErrorActionPreference = 'Stop'
$NDK  = 'F:\Android\Sdk\ndk\23.1.7779620'
$CC   = "$NDK\toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android29-clang++.cmd"
$RD   = "$NDK\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe"
$Q    = 'F:\PN2Lineage\notes\qlibs'
$OUT  = 'F:\PN2Lineage\shim\libshim_air.so'

# -nostdlib++ is essential: without it the NDK links libc++_shared.so, which
# system images do not ship (they use libc++.so). An LD_PRELOAD that cannot link
# breaks EVERY binary in the process's environment - it took out sleep and grep in
# the shell before I caught it. The shim uses no C++ runtime, so drop it.
& $CC -shared -O2 -fno-exceptions -fno-rtti -nostdlib++ `
      '-Wl,--allow-shlib-undefined' '-Wl,-soname,libshim_air.so' `
      -o $OUT F:\PN2Lineage\shim\shim_air.cpp `
      "$Q\libgui.so" "$Q\libui.so" "$Q\libutils.so" "$Q\libcamera_client.so" "$Q\libtinyxml2.so" -llog
if ($LASTEXITCODE -ne 0) { throw "link failed" }
"built $OUT ($((Get-Item $OUT).Length) bytes)"

"`n=== DT_NEEDED must not include libc++_shared ==="
$needed = & $RD -dW $OUT 2>&1 | Select-String 'NEEDED' | ForEach-Object { ($_ -replace '.*\[([^\]]+)\].*','$1') }
$needed | ForEach-Object { "  $_" }
if ($needed -contains 'libc++_shared.so') { throw "libc++_shared.so still required - would break every binary it is preloaded into" }

"`n=== the five symbols must be DEFINED here (not UND) ==="
$want = @(
  'BufferItemConsumer7setName',
  'OutputConfigurationC1ERNS_2spINS_22IGraphicBufferProducerEEEii',
  '5FenceD1Ev',
  'getLockedImageInfo',
  'lockImageFromBuffer'
)
$syms = & $RD --dyn-syms -W $OUT 2>&1
foreach ($w in $want) {
  $line = $syms | Select-String $w | Select-Object -First 1
  $state = if (-not $line) { 'ABSENT' } elseif ($line -match '\sUND\s') { 'UND (bad)' } else { 'defined' }
  "  {0,-64} {1}" -f $w, $state
}

"`n=== forward targets it imports ==="
$syms | Select-String 'UND' | Select-String 'ConsumerBase7setName|String16C1|OutputConfigurationC1.*String16' |
  ForEach-Object { "  " + ($_ -replace '\s+',' ') }
