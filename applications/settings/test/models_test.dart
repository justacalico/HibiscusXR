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

  group('radio models', () {
    test('wifi security parses the capabilities string', () {
      expect(wifiSecurityOf('[WPA2-PSK-CCMP][ESS]'), WifiSecurity.wpa);
      expect(wifiSecurityOf('[WPA-PSK][WEP]'), WifiSecurity.wpa);
      expect(wifiSecurityOf('[WEP][ESS]'), WifiSecurity.wep);
      expect(wifiSecurityOf('[ESS]'), WifiSecurity.open);
      expect(wifiSecurityOf(''), WifiSecurity.open);
    });

    test('wifi entry roundtrips', () {
      const net = WifiNetwork(
        ssid: 'office-5g',
        capabilities: '[WPA2-PSK-CCMP]',
        level: 3,
        connected: true,
        savedId: 4,
      );
      final back = WifiNetwork.fromJson(net.toJson());
      expect(back.ssid, 'office-5g');
      expect(back.security, WifiSecurity.wpa);
      expect(back.level, 3);
      expect(back.connected, isTrue);
      expect(back.saved, isTrue);
      expect(back.savedId, 4);
    });

    test('bt device and ime option roundtrip', () {
      const dev = BtDevice(
        name: 'buds',
        address: 'AA:BB:CC:00:00:01',
        bonded: true,
        connected: true,
      );
      final back = BtDevice.fromJson(dev.toJson());
      expect(back.bonded, isTrue);
      expect(back.connected, isTrue);

      const ime = ImeOption(id: 'a/.b', label: 'Board', active: true);
      final ib = ImeOption.fromJson(ime.toJson());
      expect(ib.active, isTrue);
      expect(ib.id, 'a/.b');
    });
  });

  group('snapshot lists', () {
    test('lists and scan flags roundtrip', () {
      const snap = SettingsSnapshot(
        wifi: [WifiNetwork(ssid: 'n', level: 2)],
        bt: [BtDevice(address: 'AA:BB:CC:00:00:01')],
        imes: [ImeOption(id: 'i')],
        wifiScanning: true,
        btDiscovering: true,
      );
      final back = SettingsSnapshot.fromJson(snap.toJson());
      expect(back.wifi!.single.ssid, 'n');
      expect(back.bt!.single.address, 'AA:BB:CC:00:00:01');
      expect(back.imes!.single.id, 'i');
      expect(back.wifiScanning, isTrue);
      expect(back.btDiscovering, isTrue);
    });

    test('missing lists stay null, malformed entries drop', () {
      final snap = SettingsSnapshot.fromJson({
        'wifi': ['junk', {'ssid': 'ok'}],
        'bt': 'notalist',
      });
      expect(snap.wifi!.single.ssid, 'ok');
      expect(snap.bt, isNull);
      expect(snap.imes, isNull);
      expect(snap.wifiScanning, isNull);
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
