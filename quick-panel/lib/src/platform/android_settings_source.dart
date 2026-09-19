import 'package:flutter/services.dart';

import '../models.dart';
import 'settings_source.dart';

/// MethodChannel/EventChannel bridge to the Kotlin side. The Dart half
/// only packs and unpacks maps - parsing lives here so tests can drive
/// it with a mock messenger.
class AndroidSettingsSource implements SettingsSource {
  static const _channel = MethodChannel('gitlab.neosalsa.quicksettings/system');
  static const _events = EventChannel('gitlab.neosalsa.quicksettings/events');

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
  Future<void> setVolume(double volume) =>
      _channel.invokeMethod('setVolume', {'volume': volume});

  @override
  Future<void> setBrightness(double brightness) =>
      _channel.invokeMethod('setBrightness', {'brightness': brightness});

  @override
  Future<void> requestToggle(ToggleId id, bool on) =>
      _channel.invokeMethod('requestToggle', {'id': id.name, 'on': on});

  @override
  Future<void> performAction(ActionId id) =>
      _channel.invokeMethod('performAction', {'id': id.name});

  @override
  Future<void> dismissNotification(String key) =>
      _channel.invokeMethod('dismissNotification', {'key': key});

  @override
  Future<void> dismissAllNotifications() =>
      _channel.invokeMethod('dismissAllNotifications');
}
