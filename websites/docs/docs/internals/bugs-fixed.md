# Bugs fixed

The nine real bugs between "GSI boots" and "VR stack runs". Each took real
reverse engineering - the notes repo has the full trail.

## 1. vold deadlock

Stock `/vendor/manifest.xml` declares `android.hardware.boot@1.0::IBootControl`
but ships no implementation, on a device with no A/B partitions. 8.1 never
called it; Android 10 vold does, every boot, and libhidl blocks forever on a
VINTF-declared but unregistered interface. system_server was watchdog-killed
every ~80 s, permanently.

## 2. No sound card

The sdm845 audio drivers are kernel modules loaded at ~2.7 s by
`/vendor/bin/modprobe` - which under Android 10 fails with `cannot execve`
because it is `toybox_vendor`, linked against bionic in the runtime APEX that
apexd has not mounted yet.

## 3. Sensor asserts (two)

Pico's HAL emits event types 57, 58, 126 and 127; Android 10 fatally `CHECK`s
anything between 36 and 65535. Its `DYNAMIC_SENSOR_META` path also aborts on a
connect for an unregistered handle. Fixed by patching `libsensorservice.so`
(the `sensorpatch` repo).

## 4. ABI breaks in libpvrmodule_platform.so

`getBuiltInDisplay(int)` removed, `DisplayEventReceiver::Event` grew 24→32
bytes, `DisplayInfo` grew 48→56. The last one overwrote the display token
stored right after it - the crash address was literally the screen resolution:
`0x87000000f00` = `2160<<32 | 3840`.

## 5. Suspend

The device hangs entering kernel suspend and the watchdog reboots it. Stock
never hits this: its `pvrservice` holds a kernel wakelock permanently, so PUI
never suspends at all. We hold an equivalent one - matching stock behaviour,
not repairing suspend (idle drain is higher, as on stock).

## 6. The linker whitelist

`/system/etc/public.libraries.txt` came from the GSI, so it carried only the
AOSP list and dropped stock's 22 Pico entries. On Android 8+ that file is what
lets an app's linker namespace `dlopen` a non-public `/system` library -
without it **every** Pico library load returns null and Pico's code calls the
result unchecked. The single biggest blocker: fixing it took VRShell from
dying at startup to creating 16 compositor layers.

## 7. Six missing libraries and two missing daemons

`libvirtualinputclient.so` (defines `pvrVirtualInputCreate`), `libairclient.so`,
`libSafetyArea.so`, `libImageGrid.so`, `libdatabuffer.so`,
`libvirtualinput.so`, plus `/system/bin/airservice` and
`/system/bin/virtual_input` with their init entries.

## 8. qvrd never started

Qualcomm's VR service is defined in vendor init without a `seclabel`, and init
refuses a service with no SELinux domain even when permissive. A corrected
block under the same name is discarded as a duplicate, so it runs as
**`pn2_qvrd`** instead; socket names are unchanged, which is what clients
actually use.

## 9. Two x28 binary patches

Android 10's `art_quick_generic_jni_trampoline` parks the caller's stack
pointer in x28 across a JNI call; x28 comes back zero and `sp` becomes null.
Patched to use x29, which holds the same frame base and survives. The same
register loss inside `pvr_EnterVrMode` is worked around by recomputing the
struct base from x27.
