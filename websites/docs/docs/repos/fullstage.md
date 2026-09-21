# fullstage

[HibiscusXR/system/fullstage](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/system/fullstage)

The staging tree that becomes `system-pn2-full.img`: the PVR apps already
deodexed and carrying their private `lib/arm64` dirs, the service binaries,
and the `etc/pvr` resources - arranged exactly as they land under `/system`.

```
app/<App>/<App>.apk + lib/arm64/*.so    priv-app and app payloads
bin/                                    pvrservice, pvr_compute, qvrservice, vr
etc/pvr/                                resources and configs
```

Written into the image by `tools/build/144_stage_full.sh` /
`145_build_full.sh`. The apks here come out of the repack pipeline
(`pvr_apps_final`, `oem_final`).
