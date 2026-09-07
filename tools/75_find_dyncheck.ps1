# Locate the second fatal CHECK:
#   'Check failed: it != mConnectedDynamicSensors.end()'
# in SensorDevice::handleDynamicSensorConnection. Pico's HAL reports a dynamic
# sensor DISCONNECT for a handle it never reported as connected, so the map
# lookup misses and Android 10 aborts system_server.
$RD = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$OD = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-objdump.exe'
$LIB = 'F:\PN2Lineage\sensorpatch\libsensorservice.so'   # already the PATCHED one on device

"=== symbol ==="
& $RD --dyn-syms $LIB 2>&1 | Select-String -Pattern 'handleDynamicSensorConnection'

"`n=== all symbols mentioning Dynamic ==="
& $RD --syms $LIB 2>&1 | Select-String -Pattern 'Dynamic' | Select-Object -First 10
