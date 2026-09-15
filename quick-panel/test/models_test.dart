import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/src/catalog.dart';
import 'package:pn2_quicksettings/src/models.dart';

void main() {
  test('snapshot json roundtrips', () {
    const snap = SettingsSnapshot(
      toggles: {ToggleId.wifi: true, ToggleId.nightMode: false},
      batteryLevel: 42,
      wifiSsid: 'ap',
      bluetoothDevice: 'ctl',
      volume: 0.3,
      brightness: 0.7,
    );
    final back = SettingsSnapshot.fromJson(snap.toJson());
    expect(back.toggles, snap.toggles);
    expect(back.batteryLevel, 42);
    expect(back.wifiSsid, 'ap');
    expect(back.bluetoothDevice, 'ctl');
    expect(back.volume, 0.3);
    expect(back.brightness, 0.7);
  });

  test('fromJson drops unknown toggle names and bad shapes', () {
    final s = SettingsSnapshot.fromJson({
      'toggles': {'wifi': true, 'bogus': 'yes'},
      'batteryLevel': 'not a number',
    });
    expect(s.toggles, {ToggleId.wifi: true});
    expect(s.batteryLevel, isNull);
  });

  test('catalog covers every toggle exactly once', () {
    final toggled = panelTiles
        .where((t) => t.kind == TileKind.toggle)
        .map((t) => t.toggleId)
        .toSet();
    expect(toggled, ToggleId.values.toSet());
  });

  test('four large tiles then the small ones', () {
    expect(largeTiles, hasLength(4));
    expect(largeTiles.map((t) => t.toggleId), [
      ToggleId.wifi,
      ToggleId.boundary,
      ToggleId.bluetooth,
      ToggleId.seethrough,
    ]);
    expect(smallTiles, hasLength(8));
    expect(panelTiles, [...largeTiles, ...smallTiles]);
  });

  test('unimplemented tiles are flagged', () {
    final off = panelTiles.where((t) => !t.implemented);
    expect(off.map((t) => t.toggleId ?? t.actionId), [
      ToggleId.boundary,
      ToggleId.seethrough,
      ActionId.resetView,
      ActionId.reportProblem,
    ]);
  });
}
