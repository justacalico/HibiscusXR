# Inventory every artifact that has to go into system-pn2-full.img.
# Nothing gets written until this list is complete - the device is the only copy
# of some of these right now.
$items = @(
  @{ n='public.libraries.txt (49 entries)'; p='F:\PN2Lineage\overlay_pvr\public.libraries.txt'; d='/etc/public.libraries.txt' },
  @{ n='libvirtualinputclient 64';  p='F:\PN2Lineage\overlay_pvr\lib64\libvirtualinputclient.so'; d='/lib64/libvirtualinputclient.so' },
  @{ n='libvirtualinputclient 32';  p='F:\PN2Lineage\overlay_pvr\lib\libvirtualinputclient.so';   d='/lib/libvirtualinputclient.so' },
  @{ n='libairclient 64';           p='F:\PN2Lineage\overlay_pvr\lib64\libairclient.so';          d='/lib64/libairclient.so' },
  @{ n='libairclient 32';           p='F:\PN2Lineage\overlay_pvr\lib\libairclient.so';            d='/lib/libairclient.so' },
  @{ n='libSafetyArea 64';          p='F:\PN2Lineage\overlay_pvr\lib64\libSafetyArea.so';         d='/lib64/libSafetyArea.so' },
  @{ n='libSafetyArea 32';          p='F:\PN2Lineage\overlay_pvr\lib\libSafetyArea.so';           d='/lib/libSafetyArea.so' },
  @{ n='libImageGrid 64';           p='F:\PN2Lineage\overlay_pvr\lib64\libImageGrid.so';          d='/lib64/libImageGrid.so' },
  @{ n='libImageGrid 32';           p='F:\PN2Lineage\overlay_pvr\lib\libImageGrid.so';            d='/lib/libImageGrid.so' },
  @{ n='libdatabuffer 64';          p='F:\PN2Lineage\overlay_pvr\lib64\libdatabuffer.so';         d='/lib64/libdatabuffer.so' },
  @{ n='libdatabuffer 32';          p='F:\PN2Lineage\overlay_pvr\lib\libdatabuffer.so';           d='/lib/libdatabuffer.so' },
  @{ n='libvirtualinput 64';        p='F:\PN2Lineage\airsvc\lib64\libvirtualinput.so';            d='/lib64/libvirtualinput.so' },
  @{ n='libvirtualinput 32';        p='F:\PN2Lineage\airsvc\lib\libvirtualinput.so';              d='/lib/libvirtualinput.so' },
  @{ n='airservice binary';         p='F:\PN2Lineage\airsvc\bin\airservice';                      d='/bin/airservice' },
  @{ n='virtual_input binary';      p='F:\PN2Lineage\airsvc\bin\virtual_input';                   d='/bin/virtual_input' },
  @{ n='pvr_air libairservice';     p='F:\PN2Lineage\airsvc\lib64\libairservice.so';              d='/lib64/pvr_air/libairservice.so' },
  @{ n='pvr_air libaircamera';      p='F:\PN2Lineage\airsvc\lib64\libaircamera.so';               d='/lib64/pvr_air/libaircamera.so' },
  @{ n='pvr_air libskia (stub)';    p='F:\PN2Lineage\shim\libskia_stub.so';                       d='/lib64/pvr_air/libskia.so' },
  @{ n='pvr_air libshim_air';       p='F:\PN2Lineage\shim\libshim_air.so';                        d='/lib64/pvr_air/libshim_air.so' },
  @{ n='libshim_pvr (log guard)';   p='F:\PN2Lineage\shim\libshim_pvr.so';                        d='/lib64/libshim_pvr.so' },
  @{ n='init pn2-airservice.rc';    p='F:\PN2Lineage\overlay\etc\init\pn2-airservice.rc';         d='/etc/init/pn2-airservice.rc' },
  @{ n='init pn2-qvrd.rc';          p='F:\PN2Lineage\overlay\etc\init\pn2-qvrd.rc';               d='/etc/init/pn2-qvrd.rc' },
  @{ n='init pn2-adbwifi.rc';       p='F:\PN2Lineage\overlay\etc\init\pn2-adbwifi.rc';            d='/etc/init/pn2-adbwifi.rc' },
  @{ n='ART patched libart';        p='F:\PN2Lineage\notes\libart-patched.so';                    d='/apex/com.android.runtime.release/lib64/libart.so' },
  @{ n='VRShell x28-patched SDK';   p='F:\PN2Lineage\notes\vrshell_lib\libPvr_UnitySDK.patched2.so'; d='/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so' },
  @{ n='seethrough apk (signed)';   p='F:\PN2Lineage\seethrough\seethroughsetting-signed.apk';    d='/priv-app/seethroughsetting/seethroughsetting.apk' }
)

$missing = 0
$total = 0
foreach ($i in $items) {
  if (Test-Path $i.p) {
    $sz = (Get-Item $i.p).Length
    $total += $sz
    "{0,-32} {1,12:N0}  ->  {2}" -f $i.n, $sz, $i.d
  } else {
    "{0,-32} {1,12}  ->  {2}" -f $i.n, 'MISSING', $i.d
    $missing++
  }
}
""
"seethrough native libs:"
$stl = Get-ChildItem 'F:\PN2Lineage\seethrough\lib\arm64' -Filter *.so -EA SilentlyContinue
foreach ($f in $stl) { $total += $f.Length; "  {0,-30} {1,12:N0}" -f $f.Name, $f.Length }
""
"total to add: {0:N1} MB   missing: {1}" -f ($total/1MB), $missing
