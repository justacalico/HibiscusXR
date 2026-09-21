# Repository map

One tree per component inside the
[HibiscusXR monorepo](https://gitlab.com/neosalsa/HibiscusXR).
All of it carries AGPL v3 for our own work; dump trees additionally state that
the dumped binaries belong to Pico/its vendors and are never distributed.

## Working repos (source & documentation)

| tree | what it is |
|---|---|
| [tools](tools.md) | every script for the port, sorted by job |
| [android](android.md) | LineageOS device tree `device/pico/A7B10` |
| [overlay](overlay.md) | files laid over the GSI + proprietary manifest |
| [notes](notes.md) | the research log - ~280 files of raw findings |
| [extracted](extracted.md) | decompiled boot images, dtbs, props, VR binaries |
| [shim](shim.md) | source for the ABI shim libraries |
| [vrdemo](vrdemo.md) | minimal native VR test app (`pn2vr`) |
| [pn2xr](pn2xr.md) | OpenXR stack: Monado + pn2 driver + patched Turnip |
| [library](library.md) | Flutter app library for vrhome's app window |
| [vendor_patch](vendor_patch.md) | vendor-side init/vintf patch files |
| [lens](lens.md) | `/vendor/etc/qvr` lens/distortion configs |
| [keylayout](keylayout.md) | input keylayout files |
| [persist_calib](persist_calib.md) | `/persist` calibration files |
| [pvr_dex](pvr_dex.md) | deodexed dex code of the PVR apps |
| [pvr_stack](pvr_stack.md) | pulled PVR service binaries + resources |
| [fullstage](fullstage.md) | staging tree for the full image build |
| [docs](docs.md) | this site |

## Firmware dumps (binaries gitignored, never distributed)

| tree | contents |
|---|---|
| [images](dumps/images.md) | stock PUI 4.1.3 OTA + rebuilt images + LUN0 snapshot |
| [backup_nonEye](dumps/backup_nonEye.md) | full partition backup of the non-Eye unit |
| [gsi](dumps/gsi.md) | LineageOS 17.1 GSI + raw conversion |
| [pvr_apps](dumps/pvr_apps.md) | all /system PVR apps (apk + oat) |
| [pvr_applibs](dumps/pvr_applibs.md) | app-private lib/ dirs beside each apk |
| [pvr_apps_dexed](dumps/pvr_apps_dexed.md) | deodex stage |
| [pvr_apps_signed](dumps/pvr_apps_signed.md) | re-sign stage |
| [pvr_apps_injected](dumps/pvr_apps_injected.md) | lib-injection stage |
| [pvr_apps_final](dumps/pvr_apps_final.md) | final repacked apps |
| [oem_apps](dumps/oem_apps.md) | /oem partition apps, adb-pulled |
| [oem_dex](dumps/oem_dex.md) | oem deodex stage |
| [oem_injected](dumps/oem_injected.md) | oem injection stage |
| [oem_final](dumps/oem_final.md) | final repacked oem apps |
| [seethrough](dumps/seethrough.md) | seethroughsetting app + libs |
| [sensorpatch](dumps/sensorpatch.md) | libsensorservice patch work area |
| [airsvc](dumps/airsvc.md) | airservice + virtual_input daemons |
| [fan](dumps/fan.md) | fancontrol / thermalserviced |
| [overlay_pvr](dumps/overlay_pvr.md) | Pico overlay + public.libraries.txt |
| [cdsp](dumps/cdsp.md) | CDSP RPC libraries |
| [rfsa](dumps/rfsa.md) | Hexagon DSP skel libraries |
| [qvr](dumps/qvr.md) | QVR service client libs |
| [qvrlibs](dumps/qvrlibs.md) | QVR vendor libs (incl. Tobii stubs) |
| [ndi_firmware](dumps/ndi_firmware.md) | NDI eye-tracker firmware + flashers |
| [deadunit](dumps/deadunit.md) | SPI dumps from a dead unit's eye board |
| [eyeunit](dumps/eyeunit.md) | SPI dumps from a live eye-tracking unit |
| [build](dumps/build.md) | signing keys (locally generated, never committed) |
| [out](dumps/out.md) | built images + our compiled shims |
| [ref](dumps/ref.md) | local clone of alvr-pico-legacy for reference |
