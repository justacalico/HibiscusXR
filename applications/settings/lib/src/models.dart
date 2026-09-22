/// What a row in a settings section does. Pure data - the platform
/// channel and the UI both key off these ids.
enum ItemKind {
  toggle,
  slider,
  choice,
  action,
  info,
  controller,
  scanCard,
  brand,
}

/// Sidebar sections, top to bottom in display order.
enum SectionId {
  wifi,
  bluetooth,
  controllers,
  display,
  sound,
  camera,
  language,
  time,
  keyboard,
  headsetTracking,
  backup,
  developer,
  softwareUpdate,
  about,
  tips,
}

/// Every row id. Serialized over the platform channel by name, so the
/// Kotlin side uses the same strings.
enum ItemId {
  wifiToggle,
  wifiSsid,
  wifiSettings,
  bluetoothToggle,
  bluetoothSettings,
  controllerPair,
  controllerLeft,
  controllerRight,
  controllerUnbind,
  brightness,
  nightMode,
  volume,
  micMute,
  seethrough,
  languagePicker,
  timeZone,
  keyboardPicker,
  trackingToggle,
  trackingFrequency,
  boundary,
  resetView,
  backupNow,
  devOptions,
  updateCheck,
  buildNumber,
  modelName,
  androidVersion,
  aboutBrand,
  tipsBody,
}

/// Link state of one hand controller. `raw` is the value the service
/// reports over binder: 0 idle/disconnected, 1 connected, 2 pairing.
enum ControllerLink {
  unknown(-1),
  disconnected(0),
  connected(1),
  pairing(2);

  const ControllerLink(this.raw);

  final int raw;

  static ControllerLink fromRaw(Object? value) => value is num
      ? ControllerLink.values.firstWhere(
          (l) => l.raw == value.toInt(),
          orElse: () => ControllerLink.unknown,
        )
      : ControllerLink.unknown;
}

/// Everything the settings page knows about one controller slot.
class ControllerInfo {
  const ControllerInfo({
    this.link = ControllerLink.unknown,
    this.battery = -1,
    this.charging = false,
    this.mac = '',
    this.serial = '',
  });

  /// Link state as last reported by the service.
  final ControllerLink link;

  /// Battery bar level 0-5 from the controller key report, -1 unknown.
  final int battery;
  final bool charging;
  final String mac;
  final String serial;

  Map<String, dynamic> toJson() => {
    'state': link.raw,
    'battery': battery,
    'charging': charging,
    'mac': mac,
    'serial': serial,
  };

  static ControllerInfo fromJson(Map<String, dynamic> json) => ControllerInfo(
    link: ControllerLink.fromRaw(json['state']),
    battery: json['battery'] is num ? (json['battery'] as num).toInt() : -1,
    charging: json['charging'] == true,
    mac: json['mac'] is String ? json['mac'] as String : '',
    serial: json['serial'] is String ? json['serial'] as String : '',
  );
}

/// State pushed up from the platform in one shot or as a change event.
/// Maps hold only entries with data; everything else is "unchanged".
class SettingsSnapshot {
  const SettingsSnapshot({
    this.toggles = const {},
    this.sliders = const {},
    this.choices = const {},
    this.texts = const {},
    this.controllers = const {},
  });

  final Map<ItemId, bool> toggles;
  final Map<ItemId, double> sliders;
  final Map<ItemId, String> choices;
  final Map<ItemId, String> texts;
  final Map<ItemId, ControllerInfo> controllers;

  Map<String, dynamic> toJson() => {
    'toggles': {for (final e in toggles.entries) e.key.name: e.value},
    'sliders': {for (final e in sliders.entries) e.key.name: e.value},
    'choices': {for (final e in choices.entries) e.key.name: e.value},
    'texts': {for (final e in texts.entries) e.key.name: e.value},
    'controllers': {
      for (final e in controllers.entries) e.key.name: e.value.toJson(),
    },
  };

  static SettingsSnapshot fromJson(Map<String, dynamic> json) =>
      SettingsSnapshot(
        toggles: _boolMap(json['toggles']),
        sliders: _doubleMap(json['sliders']),
        choices: _stringMap(json['choices']),
        texts: _stringMap(json['texts']),
        controllers: _controllerMap(json['controllers']),
      );

  static Map<ItemId, bool> _boolMap(Object? raw) => raw is Map
      ? {
          for (final e in raw.entries)
            if (itemIdByName('${e.key}') != null && e.value is bool)
              itemIdByName('${e.key}')!: e.value as bool,
        }
      : const {};

  static Map<ItemId, double> _doubleMap(Object? raw) => raw is Map
      ? {
          for (final e in raw.entries)
            if (itemIdByName('${e.key}') != null && e.value is num)
              itemIdByName('${e.key}')!: (e.value as num).toDouble(),
        }
      : const {};

  static Map<ItemId, String> _stringMap(Object? raw) => raw is Map
      ? {
          for (final e in raw.entries)
            if (itemIdByName('${e.key}') != null && e.value is String)
              itemIdByName('${e.key}')!: e.value as String,
        }
      : const {};

  static Map<ItemId, ControllerInfo> _controllerMap(Object? raw) => raw is Map
      ? {
          for (final e in raw.entries)
            if (itemIdByName('${e.key}') != null && e.value is Map)
              itemIdByName('${e.key}')!: ControllerInfo.fromJson(
                (e.value as Map).map((k, v) => MapEntry('$k', v)),
              ),
        }
      : const {};
}

final _itemIds = {for (final i in ItemId.values) i.name: i};
final _sectionIds = {for (final s in SectionId.values) s.name: s};

/// Lookup an [ItemId] by its serialized name, or null for unknown ids.
ItemId? itemIdByName(String name) => _itemIds[name];

/// Lookup a [SectionId] by its serialized name, or null for unknown ids.
SectionId? sectionIdByName(String name) => _sectionIds[name];
