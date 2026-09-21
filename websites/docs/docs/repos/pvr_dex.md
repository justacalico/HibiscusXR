# pvr_dex

[HibiscusXR/pvr/pvr_dex](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/pvr/pvr_dex)

The deodexed dex code of every PVR system app - `<app>_classes.dex` plus an
`_extract.log` per app. This is the study copy used to find which native libs
each app actually loads (`strings` on the dex), which is how the missing-lib
gaps were found.

Covers: configserverservice, ControllerUpgrade, CVService, InitServer,
PicoSettingsProvider, PicoToSvrService, and the rest of the PVR app set.

Pipeline: `pvr_apps` -> `pvr_dex` (deodex) -> `pvr_apps_dexed`/`signed`/
`injected`/`final` (repack stages) - see the
[pvr_apps](dumps/pvr_apps.md) page for the full flow.
