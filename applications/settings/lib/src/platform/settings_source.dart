import '../models.dart';

/// Platform facts and side effects. Implementations must stay thin:
/// report state, forward intents, no decisions.
abstract class SettingsSource {
  /// One-shot read of everything the app shows at boot.
  Future<SettingsSnapshot> load();

  /// Change feed: radio state, sliders and texts pushed by the OS.
  Stream<SettingsSnapshot> get events;

  /// Move a slider-backed value (volume, brightness).
  Future<void> setSlider(ItemId id, double value);

  /// Ask the OS to flip a toggle. Platforms that cannot switch a radio
  /// directly open the matching system panel instead.
  Future<void> requestToggle(ItemId id, bool on);

  /// Write a string-backed row (the theme choice). The platform reports
  /// it back through the snapshot texts.
  Future<void> setText(ItemId id, String value);

  /// Fire a one-shot row: open a system page, recenter, reboot...
  Future<void> performAction(ItemId id);

  /// Restart the device. Reboot-gated rows land here once their
  /// confirm dialog is accepted; platforms without the permission
  /// leave this as a no-op.
  Future<void> reboot();

  /// Refresh the wifi scan list. Results come back through the
  /// snapshot's wifi list and the wifiScanning flag.
  Future<void> scanWifi();

  /// Join a network: the platform resolves the saved configuration for
  /// [join]'s ssid or writes a new one from security + password.
  Future<void> connectWifi(WifiJoin join);

  /// Drop a saved configuration by its WifiConfiguration id.
  Future<void> forgetWifi(int networkId);

  /// Start bluetooth discovery. Devices stream back through the bt
  /// list while btDiscovering stays true.
  Future<void> scanBt();

  /// Bond with a discovered address.
  Future<void> pairBt(String address);

  /// Remove a bond.
  Future<void> unpairBt(String address);

  /// Make an input method id the system default.
  Future<void> setIme(String imeId);
}
