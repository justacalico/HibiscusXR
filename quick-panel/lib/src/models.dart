/// Toggle tiles keep an on/off state. Action tiles fire once.
enum ToggleId {
  wifi,
  bluetooth,
  seethrough,
  boundary,
  microphone,
  nightMode,
  doNotDisturb,
  airplaneMode,
  batterySaver,
}

enum ActionId { resetView, reportProblem, aboutDevice, openSettings }

/// State pushed up from the platform in one shot or as a change event.
/// Null fields mean "no information / unchanged".
class SettingsSnapshot {
  const SettingsSnapshot({
    this.toggles = const {},
    this.batteryLevel,
    this.wifiSsid,
    this.bluetoothDevice,
    this.volume,
    this.brightness,
  });

  final Map<ToggleId, bool> toggles;
  final int? batteryLevel;
  final String? wifiSsid;
  final String? bluetoothDevice;
  final double? volume;
  final double? brightness;

  Map<String, dynamic> toJson() => {
    'toggles': {for (final e in toggles.entries) e.key.name: e.value},
    if (batteryLevel != null) 'batteryLevel': batteryLevel,
    if (wifiSsid != null) 'wifiSsid': wifiSsid,
    if (bluetoothDevice != null) 'bluetoothDevice': bluetoothDevice,
    if (volume != null) 'volume': volume,
    if (brightness != null) 'brightness': brightness,
  };

  static SettingsSnapshot fromJson(Map<String, dynamic> json) {
    final raw = json['toggles'];
    return SettingsSnapshot(
      toggles: raw is Map
          ? {
              for (final e in raw.entries)
                if (toggleIdByName('${e.key}') != null)
                  toggleIdByName('${e.key}')!: e.value == true,
            }
          : const {},
      batteryLevel: _num(json['batteryLevel'])?.toInt(),
      wifiSsid: _str(json['wifiSsid']),
      bluetoothDevice: _str(json['bluetoothDevice']),
      volume: _num(json['volume'])?.toDouble(),
      brightness: _num(json['brightness'])?.toDouble(),
    );
  }

  static num? _num(Object? v) => v is num ? v : null;
  static String? _str(Object? v) => v is String ? v : null;
}

final _toggleIds = {for (final t in ToggleId.values) t.name: t};

/// Lookup a [ToggleId] by its serialized name, or null for unknown ids.
ToggleId? toggleIdByName(String name) => _toggleIds[name];

/// What shows under a large tile's title.
enum SubtitleKind { none, on, off, notConnected, custom }

class TileSubtitle {
  const TileSubtitle(this.kind, [this.text]);

  final SubtitleKind kind;
  final String? text;

  @override
  bool operator ==(Object other) =>
      other is TileSubtitle && other.kind == kind && other.text == text;

  @override
  int get hashCode => Object.hash(kind, text);
}
