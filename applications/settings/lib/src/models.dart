/// What a row in a settings section does. Pure data - the platform
/// channel and the UI both key off these ids.
enum ItemKind {
  toggle,
  slider,
  action,
  info,
  controller,
  scanCard,
  brand,
  choice,
  theme,
  wifiList,
  btList,
  imeList,
}

/// Sidebar sections, top to bottom in display order.
enum SectionId {
  wifi,
  bluetooth,
  controllers,
  display,
  sound,
  language,
  time,
  keyboard,
  developer,
  about,
}

/// Every row id. Serialized over the platform channel by name, so the
/// Kotlin side uses the same strings.
enum ItemId {
  wifiToggle,
  wifiSsid,
  wifiList,
  bluetoothToggle,
  btList,
  controllerPair,
  controllerLeft,
  controllerRight,
  controllerUnbind,
  brightness,
  themeMode,
  ipd,
  deviceMode,
  nightMode,
  volume,
  micMute,
  languagePicker,
  timeZone,
  imeList,
  adbToggle,
  stayAwake,
  showTouches,
  debugHud,
  modelName,
  androidVersion,
  hibiscusVersion,
  aboutBrand,
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

/// How a wifi network protects its key, derived from the scan result's
/// capabilities string. Order matters: an AP that speaks WPA also
/// mentions WEP in some caps strings, so WPA wins.
enum WifiSecurity { open, wep, wpa }

WifiSecurity wifiSecurityOf(String capabilities) {
  final caps = capabilities.toUpperCase();
  if (caps.contains('WPA')) return WifiSecurity.wpa;
  if (caps.contains('WEP')) return WifiSecurity.wep;
  return WifiSecurity.open;
}

/// One network from the wifi scan list, merged with the saved-network
/// table on the platform side. `savedId` is the WifiConfiguration id
/// or -1 when the network was never configured.
class WifiNetwork {
  const WifiNetwork({
    this.ssid = '',
    this.capabilities = '',
    this.level = 0,
    this.connected = false,
    this.savedId = -1,
  });

  final String ssid;
  final String capabilities;

  /// Signal strength 0-4 (WifiManager.calculateSignalLevel buckets).
  final int level;
  final bool connected;
  final int savedId;

  WifiSecurity get security => wifiSecurityOf(capabilities);

  bool get saved => savedId >= 0;

  Map<String, dynamic> toJson() => {
    'ssid': ssid,
    'caps': capabilities,
    'level': level,
    'connected': connected,
    'saved': savedId,
  };

  static WifiNetwork fromJson(Map<String, dynamic> json) => WifiNetwork(
    ssid: json['ssid'] is String ? json['ssid'] as String : '',
    capabilities: json['caps'] is String ? json['caps'] as String : '',
    level: json['level'] is num ? (json['level'] as num).toInt() : 0,
    connected: json['connected'] == true,
    savedId: json['saved'] is num ? (json['saved'] as num).toInt() : -1,
  );
}

/// What the user asked the radio to join. `password` is empty for open
/// networks and for saved configurations the platform already knows.
class WifiJoin {
  const WifiJoin({
    required this.ssid,
    required this.security,
    this.password = '',
  });

  final String ssid;
  final WifiSecurity security;
  final String password;
}

/// One row of the bluetooth device list. `bonded` entries come from the
/// adapter's bonded set, unbonded ones from an in-flight discovery.
class BtDevice {
  const BtDevice({
    this.name = '',
    this.address = '',
    this.bonded = false,
    this.connected = false,
  });

  final String name;
  final String address;
  final bool bonded;
  final bool connected;

  Map<String, dynamic> toJson() => {
    'name': name,
    'address': address,
    'bonded': bonded,
    'connected': connected,
  };

  static BtDevice fromJson(Map<String, dynamic> json) => BtDevice(
    name: json['name'] is String ? json['name'] as String : '',
    address: json['address'] is String ? json['address'] as String : '',
    bonded: json['bonded'] == true,
    connected: json['connected'] == true,
  );
}

/// One installed input method. `active` marks the one in
/// Settings.Secure.DEFAULT_INPUT_METHOD.
class ImeOption {
  const ImeOption({this.id = '', this.label = '', this.active = false});

  final String id;
  final String label;
  final bool active;

  Map<String, dynamic> toJson() => {
    'id': id,
    'label': label,
    'active': active,
  };

  static ImeOption fromJson(Map<String, dynamic> json) => ImeOption(
    id: json['id'] is String ? json['id'] as String : '',
    label: json['label'] is String ? json['label'] as String : '',
    active: json['active'] == true,
  );
}

/// State pushed up from the platform in one shot or as a change event.
/// Maps hold only entries with data; everything else is "unchanged".
/// The list fields are nullable so a partial event can leave a list
/// alone entirely (null) or replace it with a new, possibly empty,
/// set (non-null).
class SettingsSnapshot {
  const SettingsSnapshot({
    this.toggles = const {},
    this.sliders = const {},
    this.texts = const {},
    this.controllers = const {},
    this.wifi,
    this.bt,
    this.imes,
    this.wifiScanning,
    this.btDiscovering,
  });

  final Map<ItemId, bool> toggles;
  final Map<ItemId, double> sliders;
  final Map<ItemId, String> texts;
  final Map<ItemId, ControllerInfo> controllers;
  final List<WifiNetwork>? wifi;
  final List<BtDevice>? bt;
  final List<ImeOption>? imes;
  final bool? wifiScanning;
  final bool? btDiscovering;

  Map<String, dynamic> toJson() => {
    'toggles': {for (final e in toggles.entries) e.key.name: e.value},
    'sliders': {for (final e in sliders.entries) e.key.name: e.value},
    'texts': {for (final e in texts.entries) e.key.name: e.value},
    'controllers': {
      for (final e in controllers.entries) e.key.name: e.value.toJson(),
    },
    if (wifi != null) 'wifi': [for (final n in wifi!) n.toJson()],
    if (bt != null) 'bt': [for (final d in bt!) d.toJson()],
    if (imes != null) 'imes': [for (final i in imes!) i.toJson()],
    if (wifiScanning != null) 'wifiScan': wifiScanning,
    if (btDiscovering != null) 'btScan': btDiscovering,
  };

  static SettingsSnapshot fromJson(Map<String, dynamic> json) =>
      SettingsSnapshot(
        toggles: _boolMap(json['toggles']),
        sliders: _doubleMap(json['sliders']),
        texts: _stringMap(json['texts']),
        controllers: _controllerMap(json['controllers']),
        wifi: _listOf(json['wifi'], WifiNetwork.fromJson),
        bt: _listOf(json['bt'], BtDevice.fromJson),
        imes: _listOf(json['imes'], ImeOption.fromJson),
        wifiScanning: json['wifiScan'] is bool
            ? json['wifiScan'] as bool
            : null,
        btDiscovering: json['btScan'] is bool
            ? json['btScan'] as bool
            : null,
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

  /// Present-but-empty parses to an empty list so the store can tell
  /// "the platform says nothing found" from "this key wasn't in the
  /// event". Entries with a bad shape are dropped.
  static List<T>? _listOf<T>(
    Object? raw,
    T Function(Map<String, dynamic>) parse,
  ) => raw is List
      ? [
          for (final e in raw)
            if (e is Map) parse(e.map((k, v) => MapEntry('$k', v))),
        ]
      : null;
}

final _itemIds = {for (final i in ItemId.values) i.name: i};
final _sectionIds = {for (final s in SectionId.values) s.name: s};

/// Lookup an [ItemId] by its serialized name, or null for unknown ids.
ItemId? itemIdByName(String name) => _itemIds[name];

/// Lookup a [SectionId] by its serialized name, or null for unknown ids.
SectionId? sectionIdByName(String name) => _sectionIds[name];
