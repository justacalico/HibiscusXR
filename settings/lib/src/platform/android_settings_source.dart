import 'package:flutter/services.dart';

import '../models.dart';
import 'settings_source.dart';

/// MethodChannel/EventChannel bridge to the Kotlin side. The Dart half
/// only packs and unpacks maps - parsing lives here so tests can drive
/// it with a mock messenger.
class AndroidSettingsSource implements SettingsSource {
  static const _channel = MethodChannel('gitlab.neosalsa.settings/system');
  static const _events = EventChannel('gitlab.neosalsa.settings/events');

  @override
  Future<SettingsSnapshot> load() async {
    final raw = await _channel.invokeMapMethod<String, dynamic>('load');
    return raw == null
        ? const SettingsSnapshot()
        : SettingsSnapshot.fromJson(raw);
  }

  @override
  Stream<SettingsSnapshot> get events => _events
      .receiveBroadcastStream()
      .where((e) => e is Map)
      .map(
        (e) => SettingsSnapshot.fromJson(
          (e as Map).map((k, v) => MapEntry('$k', v)),
        ),
      );

  @override
  Future<void> setSlider(ItemId id, double value) =>
      _channel.invokeMethod('setSlider', {'id': id.name, 'value': value});

  @override
  Future<void> requestToggle(ItemId id, bool on) =>
      _channel.invokeMethod('requestToggle', {'id': id.name, 'on': on});

  @override
  Future<void> selectChoice(ItemId id, String value) => _channel
      .invokeMethod('selectChoice', {'id': id.name, 'value': value});

  @override
  Future<void> performAction(ItemId id) =>
      _channel.invokeMethod('performAction', {'id': id.name});
}
