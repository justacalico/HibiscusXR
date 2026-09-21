# Rules for AI agents

## Testability

Every new line of Dart must be 100% testable. Logic belongs in the pure
modules under `lib/src/` (models, catalog, labels, settings_store,
persistence, settings_controller) that run on the host. Flutter- and
platform-facing code stays thin: widgets and the MethodChannel glue hold
no decisions. If a new function cannot be reached by `flutter test`, the
design is wrong - move the logic somewhere that can.

## Tests

`flutter test` must pass before a change is called done. No exceptions, no
"should work". New logic ships with new coverage in `test/`. Visual
changes update the goldens under `test/golden/goldens/` via
`flutter test test/golden --update-goldens`.

## Strings

No user-facing string gets hardcoded. Everything visible goes through
`lib/l10n/app_en.arb` and `AppLocalizations`.

## File size

No god files. If a file is turning into a dumping ground, split it by
responsibility before adding more. Refactor toward an A+ codebase rather
than layering hacks on a working mess.
