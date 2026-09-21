import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/src/models.dart';
import 'package:pn2_quicksettings/src/platform/android_settings_source.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  const systemChannel = MethodChannel('gitlab.neosalsa.quicksettings/system');
  const eventsChannel = MethodChannel('gitlab.neosalsa.quicksettings/events');
  final messenger =
      TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger;

  final calls = <MethodCall>[];

  setUp(() {
    calls.clear();
    messenger.setMockMethodCallHandler(systemChannel, (call) async {
      calls.add(call);
      switch (call.method) {
        case 'load':
          return {
            'batteryLevel': 88,
            'wifiSsid': 'neosalsa-5g',
            'bluetoothDevice': 'Pico Controller R',
            'volume': 0.4,
            'brightness': 0.65,
            'toggles': {'wifi': true, 'bluetooth': true},
            'notifications': [
              {
                'key': '0|com.x|9',
                'app': 'Library',
                'title': 'Update',
                'text': 'Ready',
                'postMs': 44,
                'clearable': true,
              },
            ],
          };
      }
      return null;
    });
    messenger.setMockMethodCallHandler(eventsChannel, (call) async => null);
  });

  tearDown(() {
    messenger.setMockMethodCallHandler(systemChannel, null);
    messenger.setMockMethodCallHandler(eventsChannel, null);
  });

  test('load parses the platform map', () async {
    final src = AndroidSettingsSource();
    final snap = await src.load();
    expect(snap.batteryLevel, 88);
    expect(snap.wifiSsid, 'neosalsa-5g');
    expect(snap.bluetoothDevice, 'Pico Controller R');
    expect(snap.volume, 0.4);
    expect(snap.brightness, 0.65);
    expect(snap.toggles[ToggleId.wifi], isTrue);
    expect(snap.toggles[ToggleId.bluetooth], isTrue);
    expect(snap.notifications, hasLength(1));
    expect(snap.notifications!.first.key, '0|com.x|9');
    expect(snap.notifications!.first.title, 'Update');
    expect(snap.notifications!.first.clearable, isTrue);
  });

  test('intents forward with arguments', () async {
    final src = AndroidSettingsSource();
    await src.setVolume(0.3);
    await src.setBrightness(0.9);
    await src.requestToggle(ToggleId.wifi, false);
    await src.performAction(ActionId.resetView);
    await src.dismissNotification('0|com.x|9');
    await src.dismissAllNotifications();
    expect(calls.map((c) => c.method), [
      'setVolume',
      'setBrightness',
      'requestToggle',
      'performAction',
      'dismissNotification',
      'dismissAllNotifications',
    ]);
    expect(calls[0].arguments, {'volume': 0.3});
    expect(calls[1].arguments, {'brightness': 0.9});
    expect(calls[2].arguments, {'id': 'wifi', 'on': false});
    expect(calls[3].arguments, {'id': 'resetView'});
    expect(calls[4].arguments, {'key': '0|com.x|9'});
  });

  test('events stream subscribes the event channel', () async {
    var listened = false;
    messenger.setMockMethodCallHandler(eventsChannel, (call) async {
      if (call.method == 'listen') listened = true;
      return null;
    });
    final src = AndroidSettingsSource();
    final sub = src.events.listen((_) {});
    addTearDown(sub.cancel);
    await Future<void>.delayed(Duration.zero);
    expect(listened, isTrue);
  });
}
