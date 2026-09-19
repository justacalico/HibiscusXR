import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/src/models.dart';
import 'package:pn2_quicksettings/src/persistence.dart';
import 'package:pn2_quicksettings/src/platform/fake_settings_source.dart';
import 'package:pn2_quicksettings/src/settings_controller.dart';

void main() {
  test('start restores persistence then applies platform snapshot', () async {
    final src = FakeSettingsSource(
      initial: const SettingsSnapshot(
        batteryLevel: 96,
        wifiSsid: 'neosalsa-5g',
        volume: 0.6,
        toggles: {ToggleId.bluetooth: true},
      ),
    );
    final persistence = MemoryPersistence({
      'toggles': {'wifi': true},
    });
    final c = SettingsController(source: src, persistence: persistence);
    addTearDown(c.dispose);

    await c.start();
    expect(c.store.batteryLevel, 96);
    expect(c.store.wifiSsid, 'neosalsa-5g');
    expect(c.store.volume, 0.6);
    // persisted wifi survived, platform bluetooth applied
    expect(c.store.isOn(ToggleId.wifi), isTrue);
    expect(c.store.isOn(ToggleId.bluetooth), isTrue);
  });

  test('toggleTile flips state, saves and notifies the platform', () async {
    final src = FakeSettingsSource();
    final persistence = MemoryPersistence();
    final c = SettingsController(source: src, persistence: persistence);
    addTearDown(c.dispose);
    await c.start();

    await c.toggleTile(ToggleId.airplaneMode);
    expect(c.store.isOn(ToggleId.airplaneMode), isTrue);
    expect(src.togglesRequested, [
      (ToggleId.airplaneMode, true),
    ]);
    expect(persistence.saves, 1);
    final saved = persistence.stored!['toggles'] as Map;
    expect(saved['airplaneMode'], isTrue);
  });

  test('sliders forward to the platform', () async {
    final src = FakeSettingsSource();
    final c = SettingsController(
      source: src,
      persistence: MemoryPersistence(),
    );
    addTearDown(c.dispose);
    await c.start();

    await c.setVolume(0.25);
    await c.setBrightness(0.8);
    expect(c.store.volume, 0.25);
    expect(src.volumesSet, [0.25]);
    expect(src.brightnessSet, [0.8]);
  });

  test('platform events land in the store', () async {
    final src = FakeSettingsSource();
    final c = SettingsController(
      source: src,
      persistence: MemoryPersistence(),
    );
    addTearDown(c.dispose);
    await c.start();

    src.emit(
      const SettingsSnapshot(
        batteryLevel: 55,
        toggles: {ToggleId.wifi: true},
        wifiSsid: 'field-ap',
      ),
    );
    await Future<void>.delayed(Duration.zero);
    expect(c.store.batteryLevel, 55);
    expect(c.store.isOn(ToggleId.wifi), isTrue);
    expect(c.store.wifiSsid, 'field-ap');
  });

  test('actions dispatch to the platform', () async {
    final src = FakeSettingsSource();
    final c = SettingsController(
      source: src,
      persistence: MemoryPersistence(),
    );
    addTearDown(c.dispose);
    await c.start();

    await c.runAction(ActionId.resetView);
    await c.runAction(ActionId.openSettings);
    expect(src.actionsPerformed, [ActionId.resetView, ActionId.openSettings]);
  });

  test('dismissNotification drops the row and forwards the key', () async {
    const n = NotificationItem(
      key: 'k1',
      app: 'A',
      title: 't',
      text: '',
      postMs: 1,
      clearable: true,
    );
    final src = FakeSettingsSource(
      initial: const SettingsSnapshot(notifications: [n]),
    );
    final c = SettingsController(source: src, persistence: MemoryPersistence());
    addTearDown(c.dispose);
    await c.start();
    expect(c.store.notifications, [n]);

    await c.dismissNotification('k1');
    expect(c.store.notifications, isEmpty);
    expect(src.notificationsDismissed, ['k1']);
  });

  test('dismissAllNotifications skips locked rows', () async {
    const open = NotificationItem(
      key: 'k1',
      app: 'A',
      title: 't',
      text: '',
      postMs: 1,
      clearable: true,
    );
    const locked = NotificationItem(
      key: 'k2',
      app: 'B',
      title: 't',
      text: '',
      postMs: 2,
      clearable: false,
    );
    final src = FakeSettingsSource(
      initial: const SettingsSnapshot(notifications: [open, locked]),
    );
    final c = SettingsController(source: src, persistence: MemoryPersistence());
    addTearDown(c.dispose);
    await c.start();

    await c.dismissAllNotifications();
    expect(c.store.notifications, [locked]);
    expect(src.dismissAllCount, 1);
  });

  test('start is idempotent', () async {
    final src = FakeSettingsSource();
    final c = SettingsController(
      source: src,
      persistence: MemoryPersistence(),
    );
    addTearDown(c.dispose);
    await c.start();
    await c.start();
  });
}
