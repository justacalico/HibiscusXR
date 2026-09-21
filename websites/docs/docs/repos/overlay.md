# overlay

[HibiscusXR/system/overlay](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/overlay)

The files written on top of the GSI when building `system-pn2.img`. This is
the actual fix set - everything here is our own work.

```
bin/pn2-fand                  our fan daemon
etc/init/pn2-adbwifi.rc       wireless adb toggle (persist.pn2.adbwifi)
etc/init/pn2-airservice.rc    airservice + virtual_input init entries
etc/init/pn2-fanservice.rc    thermal/fan service
etc/init/pn2-power.rc         the permanent wakelock (stock parity)
etc/init/pn2-qvrd.rc          pn2_qvrd - renamed so init accepts it
etc/init/pn2-settings.rc      settings provider fixups
etc/init/pn2-snd.rc           late audio-module load (the execve fix)
etc/init/pn2-vintf.rc         manifest override for the vold deadlock
etc/init/pvrservice.rc        pvrservice init entry
etc/pn2/vendor_manifest.xml   VINTF manifest override
lib64/libsensorservice.so     patched (sensor event asserts)
lib64/libshim_pvr.so          ABI shim for libpvrmodule_platform
props.append                  build.prop additions
PROPRIETARY-PVR.md            manifest of every blob the user restores
README.md
```

`PROPRIETARY-PVR.md` is generated (by the proprietary-list script) - path,
size, sha256 prefix and purpose for each blob, all relative to stock
`system.img`. The shipped image contains none of them; they come from the
user's own firmware.
