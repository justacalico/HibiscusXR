# vmd

Virtual machine driver for Hibiscus: runs the same system image on a
Linux PC so the OS can be tested without a headset.

```
make
out/vmd -desktopsim        # window, mouse + WASD drive the HMD pose
out/vmd -novr | -pc        # same thing
out/vmd -openxr            # OpenXR app mode - a WiVRn/Monado runtime
                           # feeds a real HMD's pose in, and vmd submits
                           # the guest framebuffer to its swapchains
out/vmd -selftest          # pose channel smoke test
```

Options: `-img <dir>` (default `$PN2_ROOT/out`), `-novm`,
`-kernel/-dtb/-append` for a virtio-capable kernel when the guest's own
cannot run on qemu's virt machine.

## How it connects to the OS

```
vmd (host)                       Hibiscus in qemu
  pose TCP server :7781  <----   drivers/vmd kit driver (10.0.2.2:7781)
  qemu + QMP screendump  ---->   guest framebuffer -> desktop / HMD
```

The in-OS side is a normal hsvr kit driver (`drivers/vmd/`). It is in the
image like every other driver - the image itself is identical, the driver
only activates while the host channel answers.

## Known limit

The stock sdm845 kernel cannot boot on qemu's virt machine. Until the
LineageOS source build produces a generic kernel, point `-kernel` at a
virtio-capable build; the system image and pose path are unchanged.
