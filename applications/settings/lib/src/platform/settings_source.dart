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

  /// Record a dropdown choice (tracking frequency, ...).
  Future<void> selectChoice(ItemId id, String value);

  /// Fire a one-shot row: open a system page, recenter, reboot...
  Future<void> performAction(ItemId id);
}
