import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/src/models.dart';
import 'package:pn2_settings/src/platform/android_settings_source.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  const systemChannel = MethodChannel('gitlab.neosalsa.settings/system');
  const eventsChannel = MethodChannel('gitlab.neosalsa.settings/events');
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
            'toggles': {'wifiToggle': true, 'micMute': true},
            'sliders': {'volume': 0.4, 'brightness': 0.65},
            'texts': {'wifiSsid': 'neosalsa-5g', 'modelName': 'A7B10'},
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
    expect(snap.toggles[ItemId.wifiToggle], isTrue);
    expect(snap.sliders[ItemId.volume], 0.4);
    expect(snap.sliders[ItemId.brightness], 0.65);
    expect(snap.texts[ItemId.wifiSsid], 'neosalsa-5g');
    expect(snap.texts[ItemId.modelName], 'A7B10');
  });

  test('intents forward with arguments', () async {
    final src = AndroidSettingsSource();
    await src.setSlider(ItemId.volume, 0.3);
    await src.requestToggle(ItemId.wifiToggle, false);
    await src.performAction(ItemId.wifiSettings);
    expect(calls.map((c) => c.method), [
      'setSlider',
      'requestToggle',
      'performAction',
    ]);
    expect(calls[0].arguments, {'id': 'volume', 'value': 0.3});
    expect(calls[1].arguments, {'id': 'wifiToggle', 'on': false});
    expect(calls[2].arguments, {'id': 'wifiSettings'});
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
