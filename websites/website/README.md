# website

Project site for Neosalsa - the LineageOS 17.1 port for the Pico Neo 2
VR headset. Covers the whole neosalsa group: the port, the software, the
repository map and the research notes. Built with Flutter web, deployed
to GitLab Pages.

Live at https://hibiscusxr-37c6a7.gitlab.io/

## Build and test

```sh
flutter pub get
flutter gen-l10n
flutter test
flutter analyze
flutter build web --release --base-href "/"
```

## License

AGPL-3.0, see `LICENSE`. Bundled Inter fonts are under the SIL OFL 1.1
(`assets/fonts/OFL.txt`).
