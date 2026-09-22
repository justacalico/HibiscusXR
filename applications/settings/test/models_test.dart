import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/src/models.dart';

void main() {
  group('SettingsSnapshot', () {
    test('roundtrips the map fields', () {
      const snap = SettingsSnapshot(
        toggles: {ItemId.wifiToggle: true, ItemId.micMute: false},
        sliders: {ItemId.volume: 0.7, ItemId.brightness: 0.2},
        texts: {ItemId.wifiSsid: 'neosalsa-5g', ItemId.modelName: 'A7B10'},
      );
      final back = SettingsSnapshot.fromJson(snap.toJson());
      expect(back.toggles, snap.toggles);
      expect(back.sliders, snap.sliders);
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
        'texts': {'wifiSsid': 9, 'bogus': 'x'},
        'controllers': {
          'controllerLeft': 'yes',
          'bogus': {'state': 1},
        },
      });
      expect(snap.toggles, isEmpty);
      expect(snap.sliders, isEmpty);
      expect(snap.texts, isEmpty);
      expect(snap.controllers, isEmpty);
    });

    test('missing maps produce empty maps', () {
      final snap = SettingsSnapshot.fromJson(const {});
      expect(snap.toggles, isEmpty);
      expect(snap.sliders, isEmpty);
      expect(snap.texts, isEmpty);
      expect(snap.controllers, isEmpty);
    });

    test('controller entries roundtrip', () {
      const snap = SettingsSnapshot(
        controllers: {
          ItemId.controllerLeft: ControllerInfo(
            link: ControllerLink.connected,
            battery: 4,
            charging: true,
            mac: '2C:4D:79:00:00:01',
            serial: 'PA1111',
          ),
        },
      );
      final back = SettingsSnapshot.fromJson(snap.toJson());
      final left = back.controllers[ItemId.controllerLeft];
      expect(left, isNotNull);
      expect(left!.link, ControllerLink.connected);
      expect(left.battery, 4);
      expect(left.charging, isTrue);
      expect(left.mac, '2C:4D:79:00:00:01');
      expect(left.serial, 'PA1111');
    });
  });

  group('ControllerInfo', () {
    test('parses the wire shape', () {
      final info = ControllerInfo.fromJson({
        'state': 1,
        'battery': 3,
        'charging': false,
        'mac': 'AA:BB:CC:00:00:01',
        'serial': 'PB2222',
      });
      expect(info.link, ControllerLink.connected);
      expect(info.battery, 3);
      expect(info.charging, isFalse);
      expect(info.mac, 'AA:BB:CC:00:00:01');
      expect(info.serial, 'PB2222');
    });

    test('garbage input falls back to unknown', () {
      final info = ControllerInfo.fromJson({
        'state': 'soon',
        'battery': 'lots',
        'charging': 'maybe',
        'mac': 4,
        'serial': true,
      });
      expect(info.link, ControllerLink.unknown);
      expect(info.battery, -1);
      expect(info.charging, isFalse);
      expect(info.mac, isEmpty);
      expect(info.serial, isEmpty);
    });

    test('raw link values map to enum and back', () {
      expect(ControllerLink.fromRaw(0), ControllerLink.disconnected);
      expect(ControllerLink.fromRaw(1), ControllerLink.connected);
      expect(ControllerLink.fromRaw(2), ControllerLink.pairing);
      expect(ControllerLink.fromRaw(9), ControllerLink.unknown);
      expect(ControllerLink.fromRaw('x'), ControllerLink.unknown);
      for (final link in ControllerLink.values) {
        expect(ControllerLink.fromRaw(link.raw), link);
      }
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
