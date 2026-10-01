import 'dart:async';

import 'envs/env_info.dart';
import 'envs/env_source.dart';
import 'models.dart';
import 'persistence.dart';
import 'platform/settings_source.dart';
import 'settings_store.dart';

/// Glue between the platform [SettingsSource] and the [SettingsStore].
/// User intents go down to the source; platform facts come back through
/// events and land in the store. Holds no layout decisions.
class SettingsController {
  SettingsController({
    required this.source,
    required this.persistence,
    SettingsStore? store,
    EnvSource? envs,
  }) : store = store ?? SettingsStore(),
       envs = envs ?? const EmptyEnvSource();

  final SettingsSource source;
  final SettingsPersistence persistence;

  /// Environment zip backend. Separate from [source] because the env list
  /// comes from the filesystem, not the platform channel.
  final EnvSource envs;

  final SettingsStore store;
  StreamSubscription<SettingsSnapshot>? _events;
  bool _started = false;

  /// Restore the persisted section, pull the platform snapshot and
  /// start listening for OS changes. Safe to call once.
  Future<void> start() async {
    if (_started) return;
    _started = true;
    final saved = await persistence.load();
    if (saved != null) store.restore(saved);
    store.applySnapshot(await source.load());
    _events = source.events.listen(store.applySnapshot);
    _scanFor(store.section);
  }

  /// Entering a radio section kicks off a fresh scan so the list the
  /// user lands on is current.
  void _scanFor(SectionId id) {
    if (id == SectionId.wifi) source.scanWifi();
    if (id == SectionId.bluetooth) source.scanBt();
    if (id == SectionId.environment) refreshEnvs();
  }

  /// Flip a toggle: optimistic update, then tell the platform.
  Future<void> toggleItem(ItemId id) async {
    store.toggle(id);
    await source.requestToggle(id, store.isOn(id));
  }

  /// Set a two-state row explicitly. Choice rows pick a value instead of
  /// flipping, so re-picking the active one is a no-op. Rides the same
  /// channel as [toggleItem].
  Future<void> setItemState(ItemId id, bool on) async {
    if (store.isOn(id) == on) return;
    store.setToggle(id, on);
    await source.requestToggle(id, on);
  }

  /// Apply a two-state pick that needs a reboot to count. The caller
  /// reaches this only after the confirm dialog is accepted, so the
  /// write and the restart go down back to back; the store still moves
  /// first so the row shows the pick before the device goes down.
  Future<void> setItemStateAndReboot(ItemId id, bool on) async {
    if (store.isOn(id) == on) return;
    store.setToggle(id, on);
    await source.requestToggle(id, on);
    await source.reboot();
  }

  Future<void> setSlider(ItemId id, double v) async {
    store.setSlider(id, v);
    await source.setSlider(id, v);
  }

  /// Pick a string-backed row (the theme picker): optimistic update,
  /// then tell the platform.
  Future<void> setText(ItemId id, String v) async {
    if (store.textOf(id) == v) return;
    store.setText(id, v);
    await source.setText(id, v);
  }

  Future<void> runAction(ItemId id) => source.performAction(id);

  Future<void> scanWifi() => source.scanWifi();

  Future<void> connectWifi(WifiJoin join) => source.connectWifi(join);

  Future<void> forgetWifi(int networkId) => source.forgetWifi(networkId);

  Future<void> scanBt() => source.scanBt();

  Future<void> pairBt(String address) => source.pairBt(address);

  Future<void> unpairBt(String address) => source.unpairBt(address);

  Future<void> setIme(String imeId) => source.setIme(imeId);

  /// Rescan the environment dir into the store.
  Future<void> refreshEnvs() async => store.setEnvs(await envs.list());

  /// Pick the active home environment: kEnvPassthrough, kEnvBuiltin or an
  /// environment id. Rides the text channel onto hibiscus_environment.
  Future<void> selectEnv(String id) => setText(ItemId.homeEnv, id);

  /// Remove an environment zip. When the removed one was active the
  /// selection falls back to passthrough rather than pointing at a file
  /// that no longer exists.
  Future<void> deleteEnv(String id) async {
    if (!await envs.remove(id)) return;
    if (store.homeEnv == id) await selectEnv(kEnvPassthrough);
    await refreshEnvs();
  }

  Future<void> selectSection(SectionId id) async {
    store.selectSection(id);
    _scanFor(id);
    await persistence.save(store.snapshot());
  }

  Future<void> dispose() async {
    await _events?.cancel();
    store.dispose();
  }
}
