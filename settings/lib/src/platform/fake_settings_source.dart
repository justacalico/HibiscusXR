import 'dart:async';

import '../models.dart';
import 'settings_source.dart';

/// In-memory source for tests and previews. Records every intent so
/// tests can assert on what the UI asked the platform to do.
class FakeSettingsSource implements SettingsSource {
  FakeSettingsSource({SettingsSnapshot initial = const SettingsSnapshot()})
    : _snapshot = initial;

  SettingsSnapshot _snapshot;
  final _events = StreamController<SettingsSnapshot>.broadcast();

  final togglesRequested = <(ItemId, bool)>[];
  final slidersSet = <(ItemId, double)>[];
  final choicesSelected = <(ItemId, String)>[];
  final actionsPerformed = <ItemId>[];

  /// Test hook: pretend the OS changed something.
  void emit(SettingsSnapshot event) {
    _snapshot = SettingsSnapshot(
      toggles: {..._snapshot.toggles, ...event.toggles},
      sliders: {..._snapshot.sliders, ...event.sliders},
      choices: {..._snapshot.choices, ...event.choices},
      texts: {..._snapshot.texts, ...event.texts},
    );
    _events.add(event);
  }

  @override
  Future<SettingsSnapshot> load() async => _snapshot;

  @override
  Stream<SettingsSnapshot> get events => _events.stream;

  @override
  Future<void> setSlider(ItemId id, double value) async {
    slidersSet.add((id, value));
  }

  @override
  Future<void> requestToggle(ItemId id, bool on) async {
    togglesRequested.add((id, on));
  }

  @override
  Future<void> selectChoice(ItemId id, String value) async {
    choicesSelected.add((id, value));
  }

  @override
  Future<void> performAction(ItemId id) async {
    actionsPerformed.add(id);
  }

  Future<void> dispose() => _events.close();
}
