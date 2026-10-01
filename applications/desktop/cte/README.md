# HCTE - Hibiscus Controller Testing Environment

Desktop companion app for Hibiscus headsets. It connects over USB adb,
wireless adb, or the on-device `cted` service (see `system/cted/`) and
gives you the day-to-day development surface:

- **Overview** - what the headset is (model, driver target, Android and
  Hibiscus versions, tracking mode) and what controllers are paired, with
  battery and tracking state.
- **Display** - live mirror of the headset screen at a few frames per
  second.
- **Install** - push an APK to the device (`adb install` or the cted
  install channel).
- **Tracking** - live 6DoF/3DoF head-pose stream (orientation, position,
  sample rate, top-down trail) plus per-controller poses.
- **Debug** - full `getprop` table and a rolling logcat tail.

## Transports

| Transport | How it connects | Notes |
| --- | --- | --- |
| `adb` | `adb devices`, or `adb connect <ip>:5555` | Full feature set. Wireless needs wireless adb enabled on the headset. |
| `cted` | TCP socket to `<ip>:7340` | Needs no host-side adb. Requires `persist.hibiscus.cted=1` on the device. |

The wireless connect field tries cted first and falls back to
`adb connect`, so the same box handles both.

Head-pose data only flows while a VR app is running on the headset -
poses come out of the driver's posedump path (`debug.pn2.posedump` /
`hibiscuspose` tag), which only produces output during a session.

## Build

```sh
flutter pub get
flutter build linux   # or windows / macos
flutter test
```

The app is desktop-only by design - the headset-side counterpart of this
channel is `system/cted/`.

## License

AGPL-3.0, same as the rest of HibiscusXR. See `LICENSE`.
