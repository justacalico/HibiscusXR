# pvr_stack

[HibiscusXR/pvr/pvr_stack](https://gitlab.com/neosalsa/HibiscusXR/-/tree/main/pvr/pvr_stack)

The pulled PVR service stack - the daemons and resources that make up the
runtime, kept as a study/install set.

```
bin/pvrservice        main Pico VR daemon (dlopens libpvrservice.so)
bin/pvr_compute       compute/offload helper for the VR pipeline
bin/qvrservice        Qualcomm VR service, 32-bit - owns cameras/IMU
bin/vr                shell wrapper into com.android.commands.vr.Vr
etc/init/pvrservice.rc
etc/pvr/...           runtime resources: boundary images (goback*.png,
                      dialog_logo), configs, res.json, slam/ vocabulary
```

The binaries themselves are gitignored - what is committed is the layout and
the text resources. Extracted from stock `system.img` per
`overlay/PROPRIETARY-PVR.md`.
