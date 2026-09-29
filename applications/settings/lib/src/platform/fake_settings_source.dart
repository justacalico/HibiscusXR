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
  final textsSet = <(ItemId, String)>[];
  final actionsPerformed = <ItemId>[];
  var rebootsRequested = 0;
  var wifiScans = 0;
  final wifiJoins = <WifiJoin>[];
  final wifiForgets = <int>[];
  var btScans = 0;
  final btPairs = <String>[];
  final btUnpairs = <String>[];
  final imesSet = <String>[];

  /// Test hook: pretend the OS changed something.
  void emit(SettingsSnapshot event) {
    _snapshot = SettingsSnapshot(
      toggles: {..._snapshot.toggles, ...event.toggles},
      sliders: {..._snapshot.sliders, ...event.sliders},
      texts: {..._snapshot.texts, ...event.texts},
      controllers: {..._snapshot.controllers, ...event.controllers},
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
  Future<void> setText(ItemId id, String value) async {
    textsSet.add((id, value));
  }

  @override
  Future<void> performAction(ItemId id) async {
    actionsPerformed.add(id);
  }

  @override
  Future<void> reboot() async {
    rebootsRequested++;
  }

  @override
  Future<void> scanWifi() async {
    wifiScans++;
  }

  @override
  Future<void> connectWifi(WifiJoin join) async {
    wifiJoins.add(join);
  }

  @override
  Future<void> forgetWifi(int networkId) async {
    wifiForgets.add(networkId);
  }

  @override
  Future<void> scanBt() async {
    btScans++;
  }

  @override
  Future<void> pairBt(String address) async {
    btPairs.add(address);
  }

  @override
  Future<void> unpairBt(String address) async {
    btUnpairs.add(address);
  }

  @override
  Future<void> setIme(String imeId) async {
    imesSet.add(imeId);
  }

  Future<void> dispose() => _events.close();
}
