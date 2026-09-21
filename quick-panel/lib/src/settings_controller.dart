import 'dart:async';

import 'models.dart';
import 'persistence.dart';
import 'platform/settings_source.dart';
import 'settings_store.dart';

/// Glue between the platform [SettingsSource] and the [SettingsStore].
/// User intents go down to the source; platform facts come back through
/// events and land in the store. Holds no layout decisions.
class SettingsController {
  SettingsController({
    required this._source,
    required this._persistence,
    SettingsStore? store,
  }) : store = store ?? SettingsStore();
  final SettingsSource _source;
  final SettingsPersistence _persistence;

  final SettingsStore store;
  StreamSubscription<SettingsSnapshot>? _events;
  bool _started = false;

  /// Restore persisted toggles, pull the platform snapshot and start
  /// listening for OS changes. Safe to call once.
  Future<void> start() async {
    if (_started) return;
    _started = true;
    final saved = await _persistence.load();
    if (saved != null) store.restore(saved);
    store.applySnapshot(await _source.load());
    store.tick();
    _events = _source.events.listen(store.applySnapshot);
  }

  /// Flip a tile: optimistic update, then tell the platform.
  Future<void> toggleTile(ToggleId id) async {
    store.toggle(id);
    await _persistence.save(store.snapshot());
    await _source.requestToggle(id, store.isOn(id));
  }

  Future<void> setVolume(double volume) async {
    store.setVolume(volume);
    await _source.setVolume(volume);
  }

  Future<void> setBrightness(double brightness) async {
    store.setBrightness(brightness);
    await _source.setBrightness(brightness);
  }

  Future<void> runAction(ActionId id) => _source.performAction(id);

  /// Remove the row at once, then ask the platform to cancel it. The
  /// listener pushes the authoritative list back through [events].
  Future<void> dismissNotification(String key) async {
    store.removeNotification(key);
    await _source.dismissNotification(key);
  }

  Future<void> dismissAllNotifications() async {
    for (final n in store.notifications) {
      if (n.clearable) store.removeNotification(n.key);
    }
    await _source.dismissAllNotifications();
  }

  Future<void> dispose() async {
    await _events?.cancel();
    store.dispose();
  }
}
