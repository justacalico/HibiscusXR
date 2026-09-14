# website

Project site for `library` - the app window of vrhome on the Pico Neo 2
LineageOS port. Built with Flutter web, deployed to GitLab Pages.

## Layout

```
lib/
  main.dart                 wiring only
  l10n/                     ARB strings (en template, zh translation)
  src/
    links.dart              external URLs
    routes.dart             route table + nav destinations
    settings.dart           theme + locale controller (persisted)
    theme.dart              palette, typography, light/dark ThemeData
    router.dart             go_router setup
    ui/
      shell.dart            nav + scroll frame + footer
      nav_bar.dart          frosted nav, mobile menu, theme/lang menus
      footer.dart           link columns + legal row
      widgets.dart          PillButton, ChevronLink, Band, Reveal, ShotCard
      pages/                home, features, screenshots, downloads, about
test/                       settings, routes, l10n coverage, widgets
```

## Build and test

```sh
flutter pub get
flutter gen-l10n
flutter test
flutter analyze
flutter build web --release --base-href "/website/"
```

## Deploy

CI runs tests on every pipeline and publishes `build/web` to GitLab Pages
from `main`.

## License

AGPL-3.0, see `LICENSE`. Bundled Inter fonts are under the SIL OFL 1.1
(`assets/fonts/OFL.txt`).
