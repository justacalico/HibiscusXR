import '../models.dart';

/// Platform facts and side effects for the panel. Implementations must
/// stay thin: report state, forward intents, no decisions.
abstract class SettingsSource {
  /// One-shot read of everything the panel shows at boot.
  Future<SettingsSnapshot> load();

  /// Change feed: battery, radios and sliders pushed by the OS.
  Stream<SettingsSnapshot> get events;

  Future<void> setVolume(double volume);
  Future<void> setBrightness(double brightness);

  /// Ask the OS to flip a toggle. Platforms that cannot switch a radio
  /// directly (Android 10+ wifi/bluetooth) open the matching system
  /// panel instead and report the result through [events].
  Future<void> requestToggle(ToggleId id, bool on);

  /// Fire a one-shot tile: recenter, open settings, close the panel...
  Future<void> performAction(ActionId id);
}
