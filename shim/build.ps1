# Build libshim_pvr.so.
#
# Linked directly against the device's own libgui.so so lld records the right
# DT_NEEDED (from its SONAME) and can resolve the forward targets. --allow-shlib-
# undefined because libgui's own transitive deps are not present here and we do
# not care - we only need its symbol table.
$ErrorActionPreference = 'Stop'
$NDK   = 'F:\Android\Sdk\ndk\23.1.7779620'
$CC    = "$NDK\toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android29-clang.cmd"
$OD    = "$NDK\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-objdump.exe"
$RD    = "$NDK\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe"
$SRC   = 'F:\PN2Lineage\shim\shim_pvr.S'
$LIBGUI= 'F:\PN2Lineage\sensorpatch\libgui.so'
$OUT   = 'F:\PN2Lineage\shim\libshim_pvr.so'

# the -Wl,... args MUST be quoted: PowerShell otherwise treats the commas as
# argument separators and mangles them into separate parameters.
# No -nostdlib now: shim_events.c needs dlsym and liblog.
& $CC -shared -O2 '-Wl,--allow-shlib-undefined' '-Wl,-soname,libshim_pvr.so' `
      -o $OUT $SRC 'F:\PN2Lineage\shim\shim_events.c' $LIBGUI -ldl -llog
if ($LASTEXITCODE -ne 0) { throw "link failed" }
"built $OUT ($((Get-Item $OUT).Length) bytes)"

"`n=== exported (must be DEFINED, not UND) ==="
& $RD --dyn-syms $OUT 2>&1 | Select-String -Pattern 'getBuiltInDisplay|DisplayEventReceiverC1'

"`n=== imports it needs from libgui ==="
& $RD --dyn-syms $OUT 2>&1 | Select-String -Pattern 'UND' | Select-String -Pattern 'getInternalDisplayToken|DisplayEventReceiverC1.*ConfigChanged'

"`n=== DT_NEEDED ==="
& $RD --dynamic $OUT 2>&1 | Select-String -Pattern 'NEEDED|SONAME'

"`n=== the actual code ==="
& $OD -d $OUT 2>&1 | Select-String -Pattern 'getBuiltInDisplay|DisplayEventReceiver|^\s+[0-9a-f]+:' | Select-Object -First 14
