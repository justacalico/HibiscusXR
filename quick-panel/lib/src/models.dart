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

/// One entry in the system notification shade. [key] is the platform's
/// stable id for the notification and is what dismissal acts on.
class NotificationItem {
  const NotificationItem({
    required this.key,
    required this.app,
    required this.title,
    required this.text,
    required this.postMs,
    required this.clearable,
  });

  final String key;
  final String app;
  final String title;
  final String text;
  final int postMs;
  final bool clearable;

  Map<String, dynamic> toJson() => {
    'key': key,
    'app': app,
    'title': title,
    'text': text,
    'postMs': postMs,
    'clearable': clearable,
  };

  static NotificationItem? fromJson(Object? raw) {
    if (raw is! Map) return null;
    final key = raw['key'];
    if (key is! String || key.isEmpty) return null;
    return NotificationItem(
      key: key,
      app: raw['app'] is String ? raw['app'] as String : '',
      title: raw['title'] is String ? raw['title'] as String : '',
      text: raw['text'] is String ? raw['text'] as String : '',
      postMs: raw['postMs'] is num ? (raw['postMs'] as num).toInt() : 0,
      clearable: raw['clearable'] == true,
    );
  }

  @override
  bool operator ==(Object other) =>
      other is NotificationItem &&
      other.key == key &&
      other.app == app &&
      other.title == title &&
      other.text == text &&
      other.postMs == postMs &&
      other.clearable == clearable;

  @override
  int get hashCode => Object.hash(key, app, title, text, postMs, clearable);
}

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
    this.notifications,
  });

  final Map<ToggleId, bool> toggles;
  final int? batteryLevel;
  final String? wifiSsid;
  final String? bluetoothDevice;
  final double? volume;
  final double? brightness;

  /// The full active-notification list when present. Unlike the scalar
  /// fields this replaces wholesale rather than merging per entry.
  final List<NotificationItem>? notifications;

  Map<String, dynamic> toJson() => {
    'toggles': {for (final e in toggles.entries) e.key.name: e.value},
    if (batteryLevel != null) 'batteryLevel': batteryLevel,
    if (wifiSsid != null) 'wifiSsid': wifiSsid,
    if (bluetoothDevice != null) 'bluetoothDevice': bluetoothDevice,
    if (volume != null) 'volume': volume,
    if (brightness != null) 'brightness': brightness,
    if (notifications != null)
      'notifications': [for (final n in notifications!) n.toJson()],
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
      notifications: json['notifications'] is List
          ? (json['notifications'] as List)
                .map(NotificationItem.fromJson)
                .nonNulls
                .toList()
          : null,
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
