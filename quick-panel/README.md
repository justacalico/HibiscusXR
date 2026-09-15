# quick-panel

Quick settings panel for the Pico Neo 2 running the LineageOS 17.1 port -
a Quest-style dark panel with radios, volume and brightness, built in
Flutter.

## Build and test

```sh
flutter pub get
flutter test
flutter analyze
flutter build apk --release   # debug-signed, re-sign before distributing
```

No signing key is committed. Configure a real release key via
`key.properties` / `signingConfigs` when distributing.

## License

AGPL-3.0, see `LICENSE`.
