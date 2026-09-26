import 'package:flutter/services.dart';

import '../models.dart';
import '../units.dart';
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
    return raw == null ? const SettingsSnapshot() : _fromWire(raw);
  }

  @override
  Stream<SettingsSnapshot> get events => _events
      .receiveBroadcastStream()
      .where((e) => e is Map)
      .map((e) => _fromWire((e as Map).map((k, v) => MapEntry('$k', v))));

  @override
  Future<void> setSlider(ItemId id, double value) => _channel.invokeMethod(
    'setSlider',
    // Unit sliders cross the channel in real units, not 0..1: the Kotlin
    // side stores what it is handed and never needs the conversion table.
    {'id': id.name, 'value': id == ItemId.ipd ? ipdFromSlider(value) : value},
  );

  @override
  Future<void> requestToggle(ItemId id, bool on) =>
      _channel.invokeMethod('requestToggle', {'id': id.name, 'on': on});

  @override
  Future<void> setText(ItemId id, String value) =>
      _channel.invokeMethod('setText', {'id': id.name, 'value': value});

  @override
  Future<void> performAction(ItemId id) =>
      _channel.invokeMethod('performAction', {'id': id.name});

  @override
  Future<void> reboot() => _channel.invokeMethod('reboot');

  /// Reverse of [setSlider]: the platform reports unit sliders in real
  /// units, the store only holds normalized positions.
  static SettingsSnapshot _fromWire(Map<String, dynamic> raw) {
    final sliders = raw['sliders'];
    if (sliders is Map && sliders['ipd'] is num) {
      raw = Map<String, dynamic>.from(raw);
      raw['sliders'] = Map.of(sliders)
        ..['ipd'] = ipdToSlider((sliders['ipd'] as num).toDouble());
    }
    return SettingsSnapshot.fromJson(raw);
  }
}
