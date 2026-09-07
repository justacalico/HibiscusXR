# STANDING RULE: every proprietary Pico/Qualcomm blob this port puts to use gets
# recorded here, with where it came from, what it does, and its hash.
#
# The point is that the shipped image contains NONE of these. The end user
# extracts them from their own device's stock firmware and drops them in. So this
# manifest has to be complete and accurate enough to be executable by someone who
# is not us: source path in the stock image, destination on the built system,
# size, sha256, and what the thing actually does.
#
# Re-run after adding any new blob. Never hand-edit the generated table.
$SRC = 'F:\PN2Lineage\pvr_stack'
$MD  = 'F:\PN2Lineage\overlay\PROPRIETARY-PVR.md'
$TXT = 'F:\PN2Lineage\android\device\pico\A7B10\proprietary-files-pvr.txt'

# what each blob is for. Anything not listed is reported as UNDOCUMENTED so the
# manifest never silently accumulates mystery files.
$purpose = @{
  'bin/pvrservice'                        = 'Main Pico VR daemon. Thin launcher; dlopens libpvrservice.so. Started by pvrservice.rc, gated on sys.pvr.vrservice.state.'
  'bin/qvrservice'                        = 'Qualcomm VR Service daemon (32-bit). Owns camera/IMU acquisition and the pose ring buffer. Clients talk to it over UNIX sockets + shared memory, not binder.'
  'bin/pvr_compute'                       = 'Compute/offload helper for the VR pipeline.'
  'bin/vr'                                = 'Shell wrapper: app_process into com.android.commands.vr.Vr (needs framework/vr.jar). AOSP-derived, tiny.'
  'lib64/libcompositor.pxr.so'            = 'THE closed compositor: async timewarp, distortion mesh, direct-mode presentation. The piece needed for backwards compatibility with existing PVR titles.'
  'lib64/libruntime.pxr.so'               = 'Pico VR runtime core; session/frame lifecycle.'
  'lib64/libloader.pxr.so'                = 'Runtime loader / entry point resolution for PVR apps.'
  'lib64/libplugin.pxr.so'                = 'Plugin host for the runtime modules.'
  'lib64/libpvrservice.so'                = 'Service-side implementation loaded by the pvrservice binary.'
  'lib64/libpvrserviceclient.so'          = 'Client library apps use to talk to pvrservice.'
  'lib64/libpvrmodule_orientationtracker.so' = '3DoF orientation fusion module.'
  'lib64/libpvrmodule_platform.so'        = 'Platform abstraction module (display/power/device identity).'
  'lib64/libpxr_6dof_optimization.so'     = '6DoF pose solver / bundle adjustment. Consumes the ORB-SLAM vocabulary.'
  'lib64/libCVControllerClient.pxr.so'    = 'Controller tracking client (NDI electromagnetic controllers).'
  'lib64/libconfigurationclient.pxr.so'   = 'Reads the pxr/qvr configuration set.'
  'lib64/libpxrnotification.pxr.so'       = 'In-VR notification surface.'
  'lib64/libpxrserviceclient.so'          = 'Client shim for the pxr service.'
  'lib64/libqvrcamera_client_system.so'   = 'System-side QVR camera client (tracking cameras).'
  'lib/libqvrservice.so'                  = 'QVR service implementation (32-bit, loaded by qvrservice).'
  'lib/libqvr_eyetracking_plugin.so'      = 'Eye tracking plugin. Neo 2 Eye only; harmless on non-Eye.'
  'lib/libqvr_mapper_stub.so'             = 'QVR mapper stub.'
  'lib/libqvr_cdsp_driver_stub.so'        = 'CDSP driver stub for QVR.'
  'lib/libqvr_cam_cdsp_driver_stub.so'    = 'Camera CDSP driver stub for QVR.'
  'framework/pxr_sdk_api.jar'             = 'Pico SDK Java API surface. Needed on the bootclasspath for PVR apps that call the Pico SDK.'
  'framework/vr.jar'                      = 'AOSP vr shell command support class.'
  'etc/pvr/libsvrapi.so'                  = 'Qualcomm Snapdragon VR API (svrapi). Reads svrapi_config.txt - the file that carries the real lens intrinsics.'
  'etc/pvr/libwvr_api.so'                 = 'WaveVR-compatible API shim.'
  'etc/pvr/slam/ORBvoc.bin'               = 'ORB-SLAM bag-of-words vocabulary (~44MB). Required for relocalisation after tracking loss. Pre-trained ORB descriptor tree.'
  'etc/pvr/slam/config_of_mono_reloc.txt' = 'Mono relocalisation tuning for the SLAM front end.'
  'etc/pvr/psmvrapi_config.txt'           = 'PSM VR API config. NOTE: contents are NOT plaintext (appears encrypted/packed).'
  'etc/pvr/psmvrapi_config1.txt'          = 'Same, variant 1. Target of the /persist/pvr/psmvrapi_config.txt symlink.'
  'etc/pvr/psmvrapi_config2.txt'          = 'Same, variant 2.'
  'etc/pvr/pxr_config.txt'                = 'Top-level pxr runtime configuration.'
  'etc/pvr/res.json'                      = 'Runtime resource descriptor.'
  'etc/pvr/pxr_canary_version.txt'        = 'Version stamp.'
  'etc/init/pvrservice.rc'                = 'init service definition for pvrservice (class core, root, property-driven).'
}

$rows = @()
Get-ChildItem $SRC -Recurse -File | Sort-Object FullName | ForEach-Object {
  $rel = $_.FullName.Substring($SRC.Length + 1) -replace '\\', '/'
  # the boundary/*.png set is bulk UI artwork; roll it up rather than list 50 files
  if ($rel -like 'etc/pvr/boundary/*') { return }
  # 32-bit libs are the same component as their lib64 twin; inherit the
  # description rather than leaving a hole, but mark the arch.
  $p = if ($purpose.ContainsKey($rel)) {
    $purpose[$rel]
  } elseif ($rel -like 'lib/*' -and $purpose.ContainsKey(($rel -replace '^lib/', 'lib64/'))) {
    '(32-bit build of the same component) ' + $purpose[($rel -replace '^lib/', 'lib64/')]
  } else {
    'UNDOCUMENTED - describe this before shipping'
  }
  $rows += [pscustomobject]@{
    Rel     = $rel
    Src     = "/$rel" -replace '^/etc/', '/etc/'
    Size    = $_.Length
    Sha256  = (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower()
    Purpose = $p
  }
}
# App-private native libs live in lib/<arch>/ NEXT TO each apk, not inside it,
# and are NOT in /system/lib{,64}. Missing them is what made VRShell die on
# libmain.so, and made CVService look as though its controller library did not
# exist on this SKU at all - libCVController.so, libNDIExec.so, lib6DofFusion.so
# and libHeadImuCalibrate_int.so are all here.
$APPLIBS = 'F:\PN2Lineage\pvr_applibs'
$applibDesc = @{
  'libmain.so'            = 'Unity native entry point.'
  'libunity.so'           = 'Unity engine runtime.'
  'libil2cpp.so'          = 'Unity IL2CPP-compiled managed code for the app.'
  'libnative.so'          = 'App native glue.'
  'libtracking_module.so' = 'Per-app head tracking module.'
  'libinput_virtual_display.so' = 'Virtual display input plumbing.'
  'libPvr_UnitySDK.so'    = 'App-private Pico Unity SDK build. Distinct from every /system variant - do not substitute one for the other.'
  'libUserCenterJni.so'   = 'VRUserCenter JNI.'
  'libCVController.so'    = 'Controller tracking core. NOT present in /system - only here.'
  'libNDIExec.so'         = 'NDI electromagnetic controller executor - the Neo 2 controller hardware.'
  'lib6DofFusion.so'      = '6DoF pose fusion.'
  'libSixDofProcessor.so' = '6DoF processing.'
  'libHandImuCalibrate.so'     = 'Controller IMU calibration.'
  'libHeadImuCalibrate_int.so' = 'Head IMU calibration. pvrservice looks for this by name.'
  'libPvr_UnitySDKCV.so'  = 'Pico Unity SDK, CV-controller variant.'
  'libclientWrapper.so'   = 'Client wrapper for the controller service.'
  'libqvrservice_client.so' = 'QVR service client (app-private copy).'
  'libsqlcipher.so'       = 'Encrypted SQLite used by PVRVerify.'
  'libmt-jni.so'          = 'InitServer JNI.'
  'libPvr_NativeSDK.so'   = 'Pico native (non-Unity) SDK.'
  'libsharedmem.so'       = 'Shared memory helper.'
  'libgnustl_shared.so'   = 'GNU STL runtime (bundled).'
  'libc++.so'             = 'libc++ runtime (bundled by the app).'
}
Get-ChildItem $APPLIBS -Recurse -File -Filter '*.so' -EA SilentlyContinue |
  Sort-Object FullName | ForEach-Object {
    $rel = 'app-private/' + ($_.FullName.Substring($APPLIBS.Length + 1) -replace '\\', '/')
    $d = if ($applibDesc.ContainsKey($_.Name)) { $applibDesc[$_.Name] }
         else { 'UNDOCUMENTED - describe this before shipping' }
    $rows += [pscustomobject]@{
      Rel     = $rel
      Src     = "/$rel"
      Size    = $_.Length
      Sha256  = (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower()
      Purpose = $d
    }
  }

$bnd = Get-ChildItem "$SRC\etc\pvr\boundary" -File -EA SilentlyContinue
$bndBytes = ($bnd | Measure-Object Length -Sum).Sum

$sb = [System.Text.StringBuilder]::new()
[void]$sb.AppendLine('# Proprietary blobs used by this port')
[void]$sb.AppendLine()
[void]$sb.AppendLine('GENERATED by tools/68_gen_proprietary.ps1 - do not hand-edit.')
[void]$sb.AppendLine()
[void]$sb.AppendLine('**The shipped image contains none of these.** They are extracted by the end')
[void]$sb.AppendLine('user from their own device''s stock firmware. Every path below is relative to')
[void]$sb.AppendLine('the root of the stock `system.img` (which is itself the `/system` tree), and')
[void]$sb.AppendLine('installs to the same path under the built system.')
[void]$sb.AppendLine()
[void]$sb.AppendLine('Extract with `debugfs` - no mounting, no writing to the donor image:')
[void]$sb.AppendLine()
[void]$sb.AppendLine('```')
[void]$sb.AppendLine('debugfs -R "dump /lib64/libcompositor.pxr.so out.so" system.img')
[void]$sb.AppendLine('debugfs -R "rdump /etc/pvr ./out" system.img')
[void]$sb.AppendLine('```')
[void]$sb.AppendLine()
[void]$sb.AppendLine("Source: stock Pico Neo 2 (A7B10) firmware, OTA 4.1.3.")
[void]$sb.AppendLine()
[void]$sb.AppendLine('| Path | Size | SHA256 (first 16) | Purpose |')
[void]$sb.AppendLine('|---|---:|---|---|')
foreach ($r in $rows) {
  [void]$sb.AppendLine("| ``$($r.Rel)`` | $($r.Size) | ``$($r.Sha256.Substring(0,16))`` | $($r.Purpose) |")
}
[void]$sb.AppendLine("| ``etc/pvr/boundary/`` | $bndBytes | (dir, $($bnd.Count) files) | Guardian/boundary UI artwork and grid_point_coord.txt. Localised PNGs. |")
[void]$sb.AppendLine()
$undoc = ($rows | Where-Object { $_.Purpose -like 'UNDOCUMENTED*' }).Count
[void]$sb.AppendLine("Documented: $($rows.Count - $undoc) / $($rows.Count). Undocumented: $undoc.")
Set-Content -Path $MD -Value $sb.ToString() -Encoding UTF8

# machine-readable, LineageOS extract-files style
$t = [System.Text.StringBuilder]::new()
[void]$t.AppendLine('# Proprietary PVR/QVR blobs pulled from the stock system image.')
[void]$t.AppendLine('# GENERATED by tools/68_gen_proprietary.ps1 - do not hand-edit.')
[void]$t.AppendLine('# Not shipped. Extracted by the end user from their own device.')
[void]$t.AppendLine('# See overlay/PROPRIETARY-PVR.md for what each one does.')
[void]$t.AppendLine()
foreach ($r in $rows) { [void]$t.AppendLine("system/$($r.Rel)|$($r.Sha256)") }
foreach ($b in $bnd)  {
  $h = (Get-FileHash $b.FullName -Algorithm SHA256).Hash.ToLower()
  [void]$t.AppendLine("system/etc/pvr/boundary/$($b.Name)|$h")
}
Set-Content -Path $TXT -Value $t.ToString() -Encoding ASCII

"wrote $MD"
"wrote $TXT"
"entries: $($rows.Count) listed + $($bnd.Count) boundary files"
if ($undoc) { Write-Host "WARNING: $undoc undocumented blob(s) - describe them before shipping" -ForegroundColor Yellow }
