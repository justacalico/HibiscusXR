import 'package:flutter/foundation.dart';

import 'models.dart';

/// All panel state: toggles, sliders, battery, radios and the clock.
/// Pure Dart - every value the UI shows is computed here so tests can
/// reach it. The platform source pushes facts in through
/// [applySnapshot]; the UI reads getters.
class SettingsStore extends ChangeNotifier {
  SettingsStore({DateTime Function()? clock})
    : _clock = clock ?? DateTime.now;

  final DateTime Function() _clock;

  final Map<ToggleId, bool> _toggles = {
    for (final t in ToggleId.values) t: false,
  };
  double _volume = 0.5;
  double _brightness = 0.5;
  int _batteryLevel = 0;
  String? _wifiSsid;
  String? _bluetoothDevice;
  DateTime? _now;

  double get volume => _volume;
  double get brightness => _brightness;
  int get batteryLevel => _batteryLevel;
  String? get wifiSsid => _wifiSsid;
  String? get bluetoothDevice => _bluetoothDevice;
  DateTime get now => _now ?? _clock();

  bool isOn(ToggleId id) => _toggles[id] ?? false;

  void setToggle(ToggleId id, bool on) {
    if (_toggles[id] == on) return;
    _toggles[id] = on;
    notifyListeners();
  }

  void toggle(ToggleId id) => setToggle(id, !isOn(id));

  void setVolume(double v) {
    final c = v.clamp(0.0, 1.0);
    if (c == _volume) return;
    _volume = c;
    notifyListeners();
  }

  void setBrightness(double v) {
    final c = v.clamp(0.0, 1.0);
    if (c == _brightness) return;
    _brightness = c;
    notifyListeners();
  }

  /// Refresh the cached clock. The controller calls this on a timer.
  void tick() {
    _now = _clock();
    notifyListeners();
  }

  /// Merge a platform snapshot or change event. Null fields are left
  /// untouched so partial updates work.
  void applySnapshot(SettingsSnapshot snap) {
    snap.toggles.forEach((id, on) => _toggles[id] = on);
    if (snap.batteryLevel != null) {
      _batteryLevel = snap.batteryLevel!.clamp(0, 100);
    }
    if (snap.wifiSsid != null) _wifiSsid = snap.wifiSsid;
    if (snap.bluetoothDevice != null) {
      _bluetoothDevice = snap.bluetoothDevice;
    }
    if (snap.volume != null) _volume = snap.volume!.clamp(0.0, 1.0);
    if (snap.brightness != null) {
      _brightness = snap.brightness!.clamp(0.0, 1.0);
    }
    notifyListeners();
  }

  /// Subtitle shown under a large tile, derived from current state.
  TileSubtitle subtitleFor(ToggleId id) {
    switch (id) {
      case ToggleId.wifi:
        if (!isOn(id)) return const TileSubtitle(SubtitleKind.off);
        final ssid = _wifiSsid;
        return ssid != null && ssid.isNotEmpty
            ? TileSubtitle(SubtitleKind.custom, ssid)
            : const TileSubtitle(SubtitleKind.notConnected);
      case ToggleId.bluetooth:
        if (!isOn(id)) return const TileSubtitle(SubtitleKind.off);
        final name = _bluetoothDevice;
        return name != null && name.isNotEmpty
            ? TileSubtitle(SubtitleKind.custom, name)
            : const TileSubtitle(SubtitleKind.notConnected);
      case ToggleId.microphone:
        // Microphone tile shows the muted state as "off".
        return isOn(id)
            ? const TileSubtitle(SubtitleKind.on)
            : const TileSubtitle(SubtitleKind.off);
      default:
        return isOn(id)
            ? const TileSubtitle(SubtitleKind.on)
            : const TileSubtitle(SubtitleKind.off);
    }
  }

  /// Persisted form: toggle states only. Sliders and radios come back
  /// from the platform on every load.
  Map<String, dynamic> snapshot() => {
    'version': 1,
    'toggles': {for (final e in _toggles.entries) e.key.name: e.value},
  };

  void restore(Map<String, dynamic> json) {
    final raw = json['toggles'];
    if (raw is Map) {
      for (final e in raw.entries) {
        final id = toggleIdByName('${e.key}');
        if (id != null) _toggles[id] = e.value == true;
      }
    }
    notifyListeners();
  }
}
