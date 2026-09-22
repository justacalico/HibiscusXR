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
}
