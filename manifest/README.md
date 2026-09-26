# Hibiscus source tree (LineageOS 17.1 fork)

Long term the OS is a real LineageOS source build, not the GSI patch
pipeline in `system/dist/`. This directory is the entry point: the device
tree already lives in the monorepo at `system/android/device/pico/A7B10`,
and the driver providers at `drivers/` describe each headset's payload.

## Bring up the tree

Needs ~250G free disk, ~16G RAM, and a few hours for the first sync.
LineageOS 17.1 is EOL upstream; the github mirror still serves it.

```
mkdir hibiscus-src && cd hibiscus-src
repo init -u https://github.com/LineageOS/android.git -b lineage-17.1
mkdir -p .repo/local_manifests
cp <this-repo>/manifest/local_manifest.xml .repo/local_manifests/hibiscus.xml
repo sync -j$(nproc)
```

Then extract the proprietary blobs (adb from a stock device, or a Pico
OTA zip - nothing proprietary is redistributed):

```
cd device/pico/A7B10 && ./extract-files.sh /path/to/update.zip
```

Build:

```
. build/envsetup.sh
lunch lineage_A7B10-userdebug
mka bacon
```

## What changes vs the GSI pipeline

- `system/dist/` keeps producing shipping images until the source build
  boots on hardware - both paths stay usable.
- The driver providers (`drivers/*/driver.json`) apply in both worlds:
  the GSI pipeline reads them through `tools/provider.py`, the source
  tree reads them through `vendor/hsvr` (linked into the monorepo) plus
  device.mk packages.
- The hsvr driver kit ships in the source build the same way it does in
  the GSI one - monado is still built out-of-tree by
  `system/hsvr/monado/build.sh` and installed as the system OpenXR
  runtime.
