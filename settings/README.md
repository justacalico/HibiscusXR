# settings

System settings app for the Pico Neo 2 running the LineageOS 17.1 port.

Two-pane layout modeled on the stock headset settings: a sidebar of
sections on the left, the selected section's rows on the right.

## Features

- Wi-Fi, Bluetooth: live radio toggles, SSID display, links into the
  system pages
- Display: brightness slider, night mode
- Sound: volume slider, microphone switch
- Camera: seethrough toggle (project seam broadcast)
- Language and Region, Time, Keyboard: jump to the matching system page
- Headset Tracking: tracking toggle, tracking frequency dropdown,
  boundary toggle, reset view
- Backup, Developer, Software Update, Power (battery saver, sleep,
  restart), About (model / Android version / build), Tips and Support

Rows the platform cannot service directly open the matching system
page instead. Sleep and restart call `PowerManager` through reflection
and only fire when the app is platform-signed.

## Build and test

```sh
flutter pub get
flutter gen-l10n
flutter test
flutter build apk --release
```

The dist pipeline re-signs the release apk with the platform key and
installs it under `/system/app` as `PN2Settings`.
