import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/src/models.dart';
import 'package:pn2_settings/src/persistence.dart';
import 'package:pn2_settings/src/platform/fake_settings_source.dart';
import 'package:pn2_settings/src/settings_controller.dart';
import 'package:pn2_settings/src/settings_store.dart';

SettingsController makeController(
  FakeSettingsSource source,
  MemoryPersistence persistence,
) =>
    SettingsController(
      source: source,
      persistence: persistence,
      store: SettingsStore(),
    );

void main() {
  test('start loads snapshot, restores saved state, subscribes', () async {
    final source = FakeSettingsSource(
      initial: const SettingsSnapshot(
        toggles: {ItemId.wifiToggle: true},
        texts: {ItemId.wifiSsid: 'net-a'},
      ),
    );
    addTearDown(source.dispose);
    final persistence = MemoryPersistence({'section': 'sound'});
    final c = makeController(source, persistence);
    addTearDown(c.dispose);

    await c.start();
    expect(c.store.isOn(ItemId.wifiToggle), isTrue);
    expect(c.store.textOf(ItemId.wifiSsid), 'net-a');
    expect(c.store.section, SectionId.sound);

    source.emit(const SettingsSnapshot(
      toggles: {ItemId.bluetoothToggle: true},
    ));
    await Future<void>.delayed(Duration.zero);
    expect(c.store.isOn(ItemId.bluetoothToggle), isTrue);
  });

  test('start is idempotent', () async {
    final source = FakeSettingsSource();
    addTearDown(source.dispose);
    final c = makeController(source, MemoryPersistence());
    addTearDown(c.dispose);
    await c.start();
    await c.start();
  });

  test('toggleItem is optimistic then forwards', () async {
    final source = FakeSettingsSource();
    addTearDown(source.dispose);
    final c = makeController(source, MemoryPersistence());
    addTearDown(c.dispose);
    await c.start();

    await c.toggleItem(ItemId.micMute);
    expect(c.store.isOn(ItemId.micMute), isTrue);
    expect(source.togglesRequested, [(ItemId.micMute, true)]);
  });

  test('setSlider forwards', () async {
    final source = FakeSettingsSource();
    addTearDown(source.dispose);
    final c = makeController(source, MemoryPersistence());
    addTearDown(c.dispose);
    await c.start();

    await c.setSlider(ItemId.brightness, 0.8);
    expect(c.store.sliderValue(ItemId.brightness), 0.8);
    expect(source.slidersSet, [(ItemId.brightness, 0.8)]);
  });

  test('setText is optimistic then forwards, and dedupes', () async {
    final source = FakeSettingsSource();
    addTearDown(source.dispose);
    final c = makeController(source, MemoryPersistence());
    addTearDown(c.dispose);
    await c.start();

    await c.setText(ItemId.themeMode, 'oled');
    expect(c.store.textOf(ItemId.themeMode), 'oled');
    expect(source.textsSet, [(ItemId.themeMode, 'oled')]);

    // re-picking the active value reaches neither the store nor the wire
    await c.setText(ItemId.themeMode, 'oled');
    expect(source.textsSet, hasLength(1));
  });

  test('selectSection persists', () async {
    final source = FakeSettingsSource();
    addTearDown(source.dispose);
    final persistence = MemoryPersistence();
    final c = makeController(source, persistence);
    addTearDown(c.dispose);
    await c.start();

    await c.selectSection(SectionId.about);
    expect(c.store.section, SectionId.about);
    expect(persistence.stored!['section'], 'about');
  });

  test('runAction forwards', () async {
    final source = FakeSettingsSource();
    addTearDown(source.dispose);
    final c = makeController(source, MemoryPersistence());
    addTearDown(c.dispose);
    await c.start();

    await c.runAction(ItemId.wifiSettings);
    expect(source.actionsPerformed, [ItemId.wifiSettings]);
  });

  test('setItemState forwards once per real change', () async {
    final source = FakeSettingsSource();
    addTearDown(source.dispose);
    final c = makeController(source, MemoryPersistence());
    addTearDown(c.dispose);
    await c.start();

    await c.setItemState(ItemId.deviceMode, true);
    expect(c.store.isOn(ItemId.deviceMode), isTrue);
    expect(source.togglesRequested, [(ItemId.deviceMode, true)]);

    // re-picking the active mode reaches neither the store nor the wire
    await c.setItemState(ItemId.deviceMode, true);
    expect(source.togglesRequested, hasLength(1));

    await c.setItemState(ItemId.deviceMode, false);
    expect(c.store.isOn(ItemId.deviceMode), isFalse);
    expect(source.togglesRequested, [
      (ItemId.deviceMode, true),
      (ItemId.deviceMode, false),
    ]);
  });

  test('setItemStateAndReboot writes then restarts', () async {
    final source = FakeSettingsSource();
    addTearDown(source.dispose);
    final c = makeController(source, MemoryPersistence());
    addTearDown(c.dispose);
    await c.start();

    await c.setItemStateAndReboot(ItemId.deviceMode, true);
    expect(c.store.isOn(ItemId.deviceMode), isTrue);
    expect(source.togglesRequested, [(ItemId.deviceMode, true)]);
    expect(source.rebootsRequested, 1);

    // re-picking the active mode never reaches the wire nor reboots
    await c.setItemStateAndReboot(ItemId.deviceMode, true);
    expect(source.togglesRequested, hasLength(1));
    expect(source.rebootsRequested, 1);
  });
}
