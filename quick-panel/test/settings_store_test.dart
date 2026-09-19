import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/src/models.dart';
import 'package:pn2_quicksettings/src/settings_store.dart';

void main() {
  test('toggles default to off and flip', () {
    final s = SettingsStore();
    for (final t in ToggleId.values) {
      expect(s.isOn(t), isFalse);
    }
    s.setToggle(ToggleId.wifi, true);
    expect(s.isOn(ToggleId.wifi), isTrue);
    s.toggle(ToggleId.wifi);
    expect(s.isOn(ToggleId.wifi), isFalse);
  });

  test('sliders clamp to 0..1', () {
    final s = SettingsStore();
    s.setVolume(1.4);
    s.setBrightness(-0.2);
    expect(s.volume, 1.0);
    expect(s.brightness, 0.0);
    s.setVolume(0.3);
    expect(s.volume, 0.3);
  });

  test('applySnapshot ignores null fields', () {
    final s = SettingsStore();
    s.applySnapshot(
      const SettingsSnapshot(
        batteryLevel: 80,
        wifiSsid: 'neosalsa-5g',
        toggles: {ToggleId.wifi: true},
      ),
    );
    s.applySnapshot(const SettingsSnapshot(volume: 0.7));
    expect(s.batteryLevel, 80);
    expect(s.wifiSsid, 'neosalsa-5g');
    expect(s.isOn(ToggleId.wifi), isTrue);
    expect(s.volume, 0.7);
    expect(s.brightness, 0.5);
  });

  test('battery level clamps to 0..100', () {
    final s = SettingsStore();
    s.applySnapshot(const SettingsSnapshot(batteryLevel: 140));
    expect(s.batteryLevel, 100);
  });

  test('notifications replace wholesale and remove drops by key', () {
    const a = NotificationItem(
      key: 'a',
      app: 'A',
      title: 'ta',
      text: '',
      postMs: 1,
      clearable: true,
    );
    const b = NotificationItem(
      key: 'b',
      app: 'B',
      title: 'tb',
      text: '',
      postMs: 2,
      clearable: false,
    );
    final s = SettingsStore();
    s.applySnapshot(const SettingsSnapshot(notifications: [a, b]));
    expect(s.notifications, [a, b]);
    // a snapshot without the field leaves the list alone
    s.applySnapshot(const SettingsSnapshot(batteryLevel: 50));
    expect(s.notifications, [a, b]);
    s.applySnapshot(const SettingsSnapshot(notifications: [b]));
    expect(s.notifications, [b]);

    s.removeNotification('missing');
    expect(s.notifications, [b]);
    s.removeNotification('b');
    expect(s.notifications, isEmpty);
  });

  test('clock uses injected source and tick refreshes', () {
    var t = DateTime(2023, 8, 16, 15, 52);
    final s = SettingsStore(clock: () => t);
    expect(s.now, t);
    t = DateTime(2023, 8, 16, 16, 0);
    s.tick();
    expect(s.now, t);
  });

  test('wifi subtitle: ssid when connected, not connected when bare', () {
    final s = SettingsStore();
    expect(
      s.subtitleFor(ToggleId.wifi),
      const TileSubtitle(SubtitleKind.off),
    );
    s.applySnapshot(
      const SettingsSnapshot(toggles: {ToggleId.wifi: true}),
    );
    expect(
      s.subtitleFor(ToggleId.wifi),
      const TileSubtitle(SubtitleKind.notConnected),
    );
    s.applySnapshot(const SettingsSnapshot(wifiSsid: 'neosalsa-5g'));
    expect(
      s.subtitleFor(ToggleId.wifi),
      const TileSubtitle(SubtitleKind.custom, 'neosalsa-5g'),
    );
  });

  test('bluetooth subtitle follows device name', () {
    final s = SettingsStore();
    s.applySnapshot(
      const SettingsSnapshot(
        toggles: {ToggleId.bluetooth: true},
        bluetoothDevice: 'Pico Controller L',
      ),
    );
    expect(
      s.subtitleFor(ToggleId.bluetooth),
      const TileSubtitle(SubtitleKind.custom, 'Pico Controller L'),
    );
  });

  test('plain toggles subtitle on/off', () {
    final s = SettingsStore();
    expect(
      s.subtitleFor(ToggleId.nightMode),
      const TileSubtitle(SubtitleKind.off),
    );
    s.setToggle(ToggleId.nightMode, true);
    expect(
      s.subtitleFor(ToggleId.nightMode),
      const TileSubtitle(SubtitleKind.on),
    );
  });

  test('snapshot roundtrips toggle states only', () {
    final s = SettingsStore();
    s.setToggle(ToggleId.doNotDisturb, true);
    s.setToggle(ToggleId.seethrough, true);
    s.applySnapshot(const SettingsSnapshot(volume: 0.9));

    final restored = SettingsStore()..restore(s.snapshot());
    expect(restored.isOn(ToggleId.doNotDisturb), isTrue);
    expect(restored.isOn(ToggleId.seethrough), isTrue);
    expect(restored.isOn(ToggleId.wifi), isFalse);
    // sliders are not persisted
    expect(restored.volume, 0.5);
  });

  test('restore ignores unknown toggle names', () {
    final s = SettingsStore();
    s.restore({
      'toggles': {'wifi': true, 'notARealToggle': true},
    });
    expect(s.isOn(ToggleId.wifi), isTrue);
  });

  test('notifies listeners on change, not on no-op', () {
    final s = SettingsStore();
    var n = 0;
    s.addListener(() => n++);
    s.setToggle(ToggleId.wifi, true);
    s.setToggle(ToggleId.wifi, true);
    s.setVolume(0.5);
    s.setVolume(0.9);
    expect(n, 2);
  });
}
