import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/src/models.dart';
import 'package:pn2_settings/src/settings_store.dart';

void main() {
  test('toggles default off, notify on change only', () {
    final store = SettingsStore();
    var ticks = 0;
    store.addListener(() => ticks++);

    expect(store.isOn(ItemId.wifiToggle), isFalse);
    store.setToggle(ItemId.wifiToggle, false);
    expect(ticks, 0);
    store.setToggle(ItemId.wifiToggle, true);
    expect(ticks, 1);
    expect(store.isOn(ItemId.wifiToggle), isTrue);

    store.toggle(ItemId.wifiToggle);
    expect(store.isOn(ItemId.wifiToggle), isFalse);
  });

  test('sliders clamp and notify', () {
    final store = SettingsStore();
    var ticks = 0;
    store.addListener(() => ticks++);

    expect(store.sliderValue(ItemId.volume), 0.5);
    store.setSlider(ItemId.volume, 1.4);
    expect(store.sliderValue(ItemId.volume), 1.0);
    store.setSlider(ItemId.volume, -2);
    expect(store.sliderValue(ItemId.volume), 0.0);
    store.setSlider(ItemId.volume, 0.0);
    expect(ticks, 2);
  });

  test('choice falls back to the first catalog option', () {
    final store = SettingsStore();
    expect(store.choiceOf(ItemId.trackingFrequency), 'auto');
    store.setChoice(ItemId.trackingFrequency, '60hz');
    expect(store.choiceOf(ItemId.trackingFrequency), '60hz');
  });

  test('applySnapshot merges only carried keys', () {
    final store = SettingsStore()
      ..applySnapshot(const SettingsSnapshot(
        toggles: {ItemId.wifiToggle: true},
        texts: {ItemId.wifiSsid: 'net-a'},
      ));
    store.applySnapshot(const SettingsSnapshot(
      sliders: {ItemId.volume: 0.9},
      texts: {ItemId.modelName: 'A7B10'},
    ));
    expect(store.isOn(ItemId.wifiToggle), isTrue);
    expect(store.sliderValue(ItemId.volume), 0.9);
    expect(store.textOf(ItemId.wifiSsid), 'net-a');
    expect(store.textOf(ItemId.modelName), 'A7B10');
  });

  test('controllerOf returns placeholder until the platform reports', () {
    final store = SettingsStore();
    expect(store.controllerOf(ItemId.controllerLeft).link,
        ControllerLink.unknown);
    expect(store.controllerOf(ItemId.controllerLeft).battery, -1);

    store.applySnapshot(const SettingsSnapshot(controllers: {
      ItemId.controllerLeft: ControllerInfo(
        link: ControllerLink.connected,
        battery: 5,
      ),
    }));
    expect(store.controllerOf(ItemId.controllerLeft).link,
        ControllerLink.connected);
    expect(store.controllerOf(ItemId.controllerLeft).battery, 5);
    // a slot that was not reported keeps its placeholder
    expect(store.controllerOf(ItemId.controllerRight).link,
        ControllerLink.unknown);
  });

  test('selectSection switches and dedupes', () {
    final store = SettingsStore();
    var ticks = 0;
    store.addListener(() => ticks++);

    expect(store.section, SectionId.wifi);
    store.selectSection(SectionId.wifi);
    expect(ticks, 0);
    store.selectSection(SectionId.about);
    expect(store.section, SectionId.about);
    expect(ticks, 1);
  });

  test('snapshot/restore roundtrips section and choices', () {
    final store = SettingsStore()
      ..selectSection(SectionId.headsetTracking)
      ..setChoice(ItemId.trackingFrequency, '50hz')
      ..setToggle(ItemId.boundary, true);
    final back = SettingsStore()..restore(store.snapshot());
    expect(back.section, SectionId.headsetTracking);
    expect(back.choiceOf(ItemId.trackingFrequency), '50hz');
    // toggles are not persisted - the platform reports them on load
    expect(back.isOn(ItemId.boundary), isFalse);
  });

  test('restore ignores garbage', () {
    final store = SettingsStore();
    store.restore({
      'section': 42,
      'choices': {'trackingFrequency': 9, 'bogus': 'x'},
    });
    expect(store.section, SectionId.wifi);
    expect(store.choiceOf(ItemId.trackingFrequency), 'auto');
  });
}
