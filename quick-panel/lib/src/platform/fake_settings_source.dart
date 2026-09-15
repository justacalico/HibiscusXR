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

  final togglesRequested = <(ToggleId, bool)>[];
  final actionsPerformed = <ActionId>[];
  final volumesSet = <double>[];
  final brightnessSet = <double>[];

  /// Test hook: pretend the OS changed something.
  void emit(SettingsSnapshot event) {
    _snapshot = SettingsSnapshot(
      toggles: {..._snapshot.toggles, ...event.toggles},
      batteryLevel: event.batteryLevel ?? _snapshot.batteryLevel,
      wifiSsid: event.wifiSsid ?? _snapshot.wifiSsid,
      bluetoothDevice: event.bluetoothDevice ?? _snapshot.bluetoothDevice,
      volume: event.volume ?? _snapshot.volume,
      brightness: event.brightness ?? _snapshot.brightness,
    );
    _events.add(event);
  }

  @override
  Future<SettingsSnapshot> load() async => _snapshot;

  @override
  Stream<SettingsSnapshot> get events => _events.stream;

  @override
  Future<void> setVolume(double volume) async {
    volumesSet.add(volume);
  }

  @override
  Future<void> setBrightness(double brightness) async {
    brightnessSet.add(brightness);
  }

  @override
  Future<void> requestToggle(ToggleId id, bool on) async {
    togglesRequested.add((id, on));
  }

  @override
  Future<void> performAction(ActionId id) async {
    actionsPerformed.add(id);
  }

  Future<void> dispose() => _events.close();
}
