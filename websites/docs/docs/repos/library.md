# library

[gitlab.com/neosalsa/library](https://gitlab.com/neosalsa/library)

Flutter app library (`gitlab.neosalsa.library`) - the Quest-style dark grid
of launchable apps meant to run as `vrhome`'s app window, also usable as a
plain launcher activity.

- Every decision lives in pure Dart under `lib/src/` and is covered by unit
  tests; the Kotlin side only queries `PackageManager`, rasterizes icons to
  PNG and fires intents over a MethodChannel
- Search, collections (All / Pinned / Apps / System / user groups), five
  sort modes, pinning, long-press-drag reorder, context menu with
  uninstall/details, APK install through the SAF picker
- D-pad navigation for the controller: arrows move focus, confirm launches,
  menu key opens the tile menu
- Pins, group membership and manual order persist in SharedPreferences;
  all UI strings sit in ARB files for localization
- Release builds sign with the committed `debug.keystore`, same convention
  as `vrhome`
