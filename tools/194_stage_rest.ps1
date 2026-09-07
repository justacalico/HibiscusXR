# Close the last gaps before re-applying the whitelist:
#   - libdatabuffer.so: libairclient.so's missing dependency (pull from stock)
#   - lib6DofReset.so / libpxrnotification.pxr.so: on our device but never staged,
#     so their closure was unverified
$adb   = 'C:\adb\adb.exe'
$OURS  = 'PA7B40NGE5300009W'
$STOCK = '192.168.0.139:5555'
& $adb connect $STOCK 2>&1 | Out-Null

# libdatabuffer.so from stock, both ABIs
foreach ($d in @('lib64','lib')) {
  & $adb -s $STOCK shell "su -c 'cp /system/$d/libdatabuffer.so /data/local/tmp/_db.so 2>/dev/null; chmod 644 /data/local/tmp/_db.so'" 2>&1 | Out-Null
  & $adb -s $STOCK pull /data/local/tmp/_db.so "F:\PN2Lineage\overlay_pvr\$d\libdatabuffer.so" 2>&1 | Out-Null
  $p = "F:\PN2Lineage\overlay_pvr\$d\libdatabuffer.so"
  if (Test-Path $p) { Write-Host ("  libdatabuffer.so {0}: {1} bytes" -f $d, (Get-Item $p).Length) }
  else { Write-Host ("  libdatabuffer.so {0}: NOT ON STOCK EITHER" -f $d) }
}

# the two unverified ones, from our own device (they are already installed here)
foreach ($l in @('lib6DofReset.so','libpxrnotification.pxr.so')) {
  & $adb -s $OURS shell "su -c 'cp /system/lib64/$l /data/local/tmp/_u.so 2>/dev/null; chmod 644 /data/local/tmp/_u.so'" 2>&1 | Out-Null
  & $adb -s $OURS pull /data/local/tmp/_u.so "F:\PN2Lineage\notes\lib64\$l" 2>&1 | Out-Null
  $p = "F:\PN2Lineage\notes\lib64\$l"
  if (Test-Path $p) { Write-Host ("  staged {0}: {1} bytes" -f $l, (Get-Item $p).Length) }
  else { Write-Host ("  {0}: could not stage" -f $l) }
}
