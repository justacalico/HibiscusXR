# settings

System settings app for the Pico Neo 2 running the LineageOS 17.1 port.

Two-pane layout modeled on the stock headset settings: a sidebar of
sections on the left, the selected section's rows on the right.

## Features

- Wi-Fi: live radio toggle, SSID display, in-app scan list with
  connect / forget (privileged WifiManager calls, no trip to the system
  settings app)
- Bluetooth: live radio toggle, in-app device list - bonded devices,
  discovery, pair and forget
- Display: brightness slider, OS theme picker (dark / light / OLED -
  writes `hibiscus_theme`, which pn2-themed mirrors onto
  `persist.hibiscus.theme` so the HUD chrome and the other panel apps
  follow), night mode
- Home environment: pick the home backdrop - passthrough, the built-in
  sky scene, or a zip package dropped in /data/local/tmp/hibiscus/envs
  (writes `hibiscus_environment`, which pn2-envd mirrors onto
  `persist.hibiscus.environment` for the home shell)
- Sound: volume slider, microphone switch
- Camera: seethrough toggle (project seam broadcast)
- Keyboard: in-app input method picker
- Language and Region, Time: jump to the matching system page
- Headset Tracking: tracking toggle, tracking frequency dropdown,
  boundary toggle, reset view
- Backup, Developer, Software Update, About (brand mark / model /
  Android version / build), Tips and Support

Rows the platform cannot service directly open the matching system
page instead.

## Build and test

```sh
flutter pub get
flutter gen-l10n
flutter test
flutter build apk --release
```

The dist pipeline re-signs the release apk with the platform key and
installs it under `/system/app` as `PN2Settings`.
