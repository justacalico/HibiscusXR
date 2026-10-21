# Build the VR demo APK without Gradle.
#
# The APK has no Java: NativeActivity is a framework class, so all we ship is a
# manifest plus one .so. That means aapt2 -> add lib -> zipalign -> apksigner,
# and none of the AGP/JDK version coupling that Gradle would drag in (this box
# has JDK 11, which modern AGP refuses).
$ErrorActionPreference = 'Stop'

param(
  [string]$Sdk  = "$env:ANDROID_SDK_ROOT",
  [string]$Ndk  = "$env:ANDROID_NDK_ROOT",
  [string]$Root = "$env:HIBISCUS_ROOT"
)
if (-not $Sdk)  { throw 'set ANDROID_SDK_ROOT or pass -Sdk' }
if (-not $Ndk)  { throw 'set ANDROID_NDK_ROOT or pass -Ndk' }
if (-not $Root) { throw 'set HIBISCUS_ROOT or pass -Root' }

$NDK  = $Ndk
$BT   = (Get-ChildItem "$Sdk\build-tools" -Directory | Sort-Object Name | Select-Object -Last 1).FullName
$JAR  = (Get-ChildItem "$Sdk\platforms\android-*\android.jar" | Sort-Object Name | Select-Object -Last 1).FullName
$SRC  = "$Root\vrdemo"
$OUT  = "$Root\vrdemo\out"
$KS   = "$Root\build\debug.keystore"   # deliberately outside the source dir

$CLANG = "$NDK\toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android26-clang++.cmd"
$CC    = "$NDK\toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android26-clang.cmd"
$GLUE  = "$NDK\sources\android\native_app_glue"

foreach ($p in @($CLANG, "$GLUE\android_native_app_glue.c", $JAR, "$BT\aapt2.exe")) {
    if (-not (Test-Path $p)) { throw "missing: $p" }
}
New-Item -ItemType Directory -Force -Path $OUT, "$OUT\lib\arm64-v8a", "$Root\build" | Out-Null

Write-Host "[1/5] compiling native lib"
# glue is C and must be built by the C driver; clang++ miscompiles its
# implicit void* conversions
& $CC -c -fPIC -O2 -I "$GLUE" -o "$OUT\glue.o" "$GLUE\android_native_app_glue.c"
if ($LASTEXITCODE -ne 0) { throw "glue compile failed" }

# -static-libstdc++ so we don't have to ship libc++_shared.so alongside; the
# renderer barely touches the STL and this keeps the APK a single .so
& $CLANG `
    -shared -fPIC -O2 -std=c++17 -static-libstdc++ `
    -I "$GLUE" `
    -o "$OUT\lib\arm64-v8a\libpn2vr.so" `
    "$SRC\src\main.cpp" "$OUT\glue.o" `
    -landroid -lEGL -lGLESv2 -llog -lm -u ANativeActivity_onCreate
if ($LASTEXITCODE -ne 0) { throw "compile failed" }
Write-Host ("      libpn2vr.so = {0} bytes" -f (Get-Item "$OUT\lib\arm64-v8a\libpn2vr.so").Length)

Write-Host "[2/5] aapt2 link (manifest only, no resources)"
& "$BT\aapt2.exe" link `
    -I $JAR `
    --manifest "$SRC\AndroidManifest.xml" `
    --min-sdk-version 26 --target-sdk-version 29 `
    -o "$OUT\base.apk"
if ($LASTEXITCODE -ne 0) { throw "aapt2 link failed" }

Write-Host "[3/5] injecting native lib into the apk"
Copy-Item "$OUT\base.apk" "$OUT\unsigned.apk" -Force
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::Open("$OUT\unsigned.apk", 'Update')
try {
    $existing = $zip.GetEntry('lib/arm64-v8a/libpn2vr.so')
    if ($existing) { $existing.Delete() }
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
        $zip, "$OUT\lib\arm64-v8a\libpn2vr.so", 'lib/arm64-v8a/libpn2vr.so') | Out-Null
} finally { $zip.Dispose() }

Write-Host "[4/5] zipalign"
Remove-Item "$OUT\aligned.apk" -EA SilentlyContinue
& "$BT\zipalign.exe" -p -f 4 "$OUT\unsigned.apk" "$OUT\aligned.apk"
if ($LASTEXITCODE -ne 0) { throw "zipalign failed" }

if (-not (Test-Path $KS)) {
    Write-Host "      generating debug keystore (kept out of the source tree)"
    & keytool -genkeypair -keystore $KS -alias pn2 -storepass android -keypass android `
        -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=pn2vr, OU=dev, O=dev, C=US" 2>&1 | Out-Null
}

Write-Host "[5/5] signing"
Remove-Item "$OUT\pn2vr.apk" -EA SilentlyContinue
& "$BT\apksigner.bat" sign --ks $KS --ks-pass pass:android --key-pass pass:android `
    --out "$OUT\pn2vr.apk" "$OUT\aligned.apk"
if ($LASTEXITCODE -ne 0) { throw "signing failed" }

Write-Host ("DONE -> {0} ({1} bytes)" -f "$OUT\pn2vr.apk", (Get-Item "$OUT\pn2vr.apk").Length)
