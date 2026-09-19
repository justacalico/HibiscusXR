import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/src/models.dart';

void main() {
  group('SettingsSnapshot', () {
    test('roundtrips all four maps', () {
      const snap = SettingsSnapshot(
        toggles: {ItemId.wifiToggle: true, ItemId.micMute: false},
        sliders: {ItemId.volume: 0.7, ItemId.brightness: 0.2},
        choices: {ItemId.trackingFrequency: '60hz'},
        texts: {ItemId.wifiSsid: 'neosalsa-5g', ItemId.modelName: 'A7B10'},
      );
      final back = SettingsSnapshot.fromJson(snap.toJson());
      expect(back.toggles, snap.toggles);
      expect(back.sliders, snap.sliders);
      expect(back.choices, snap.choices);
      expect(back.texts, snap.texts);
    });

    test('parses int sliders as doubles', () {
      final snap = SettingsSnapshot.fromJson({
        'sliders': {'volume': 1},
      });
      expect(snap.sliders[ItemId.volume], 1.0);
    });

    test('drops unknown ids and wrong types', () {
      final snap = SettingsSnapshot.fromJson({
        'toggles': {'wifiToggle': 'yes', 'bogus': true},
        'sliders': {'volume': 'loud', 'bogus': 1},
        'choices': {'trackingFrequency': 5, 'bogus': 'x'},
        'texts': {'wifiSsid': 9, 'bogus': 'x'},
      });
      expect(snap.toggles, isEmpty);
      expect(snap.sliders, isEmpty);
      expect(snap.choices, isEmpty);
      expect(snap.texts, isEmpty);
    });

    test('missing maps produce empty maps', () {
      final snap = SettingsSnapshot.fromJson(const {});
      expect(snap.toggles, isEmpty);
      expect(snap.sliders, isEmpty);
      expect(snap.choices, isEmpty);
      expect(snap.texts, isEmpty);
    });
  });

  group('id lookup', () {
    test('itemIdByName resolves every enum value', () {
      for (final id in ItemId.values) {
        expect(itemIdByName(id.name), id);
      }
      expect(itemIdByName('nope'), isNull);
    });

    test('sectionIdByName resolves every enum value', () {
      for (final id in SectionId.values) {
        expect(sectionIdByName(id.name), id);
      }
      expect(sectionIdByName('nope'), isNull);
    });
  });
}
