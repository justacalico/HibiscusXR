import 'package:flutter/foundation.dart';

import 'envs/env_info.dart';
import 'models.dart';

/// All app state: toggle values, slider positions, platform-reported
/// texts and the selected sidebar section. Pure Dart -
/// every value the UI shows is computed here so tests can reach it.
class SettingsStore extends ChangeNotifier {
  final Map<ItemId, bool> _toggles = {};
  final Map<ItemId, double> _sliders = {};
  final Map<ItemId, String> _texts = {};
  final Map<ItemId, ControllerInfo> _controllers = {};
  List<WifiNetwork> _wifi = const [];
  List<BtDevice> _bt = const [];
  List<ImeOption> _imes = const [];
  List<EnvOption> _envs = const [];
  bool _wifiScanning = false;
  bool _btDiscovering = false;
  SectionId _section = SectionId.wifi;

  SectionId get section => _section;

  void selectSection(SectionId id) {
    if (_section == id) return;
    _section = id;
    notifyListeners();
  }

  bool isOn(ItemId id) => _toggles[id] ?? false;

  void setToggle(ItemId id, bool on) {
    if (isOn(id) == on) return;
    _toggles[id] = on;
    notifyListeners();
  }

  void toggle(ItemId id) => setToggle(id, !isOn(id));

  double sliderValue(ItemId id) => _sliders[id] ?? 0.5;

  void setSlider(ItemId id, double v) {
    final c = v.clamp(0.0, 1.0);
    if (c == _sliders[id]) return;
    _sliders[id] = c;
    notifyListeners();
  }

  String? textOf(ItemId id) => _texts[id];

  /// Local write for a string-backed row (the theme picker). The
  /// platform confirms through the next snapshot.
  void setText(ItemId id, String v) {
    if (_texts[id] == v) return;
    _texts[id] = v;
    notifyListeners();
  }

  /// Last reported state of a controller row, or a placeholder when the
  /// service has not answered yet.
  ControllerInfo controllerOf(ItemId id) =>
      _controllers[id] ?? const ControllerInfo();

  /// Last wifi scan list the platform pushed.
  List<WifiNetwork> get wifiNetworks => _wifi;

  bool get wifiScanning => _wifiScanning;

  /// Bonded plus discovered bluetooth devices.
  List<BtDevice> get btDevices => _bt;

  bool get btDiscovering => _btDiscovering;

  /// Installed input methods, active one flagged.
  List<ImeOption> get imeOptions => _imes;

  /// Environment zips found on the last scan of the shared env dir.
  List<EnvOption> get envOptions => _envs;

  /// Replace the environment list after a rescan.
  void setEnvs(List<EnvOption> envs) {
    _envs = envs;
    notifyListeners();
  }

  /// The picked environment id, normalized: an unset key is passthrough.
  String get homeEnv => envSelOr(textOf(ItemId.homeEnv));

  /// Merge a platform snapshot or change event. Only the keys the
  /// snapshot carries are touched, so partial updates work.
  void applySnapshot(SettingsSnapshot snap) {
    _toggles.addAll(snap.toggles);
    _sliders.addAll(snap.sliders);
    _texts.addAll(snap.texts);
    _controllers.addAll(snap.controllers);
    if (snap.wifi != null) _wifi = snap.wifi!;
    if (snap.bt != null) _bt = snap.bt!;
    if (snap.imes != null) _imes = snap.imes!;
    if (snap.wifiScanning != null) _wifiScanning = snap.wifiScanning!;
    if (snap.btDiscovering != null) _btDiscovering = snap.btDiscovering!;
    notifyListeners();
  }

  /// Persisted form: just the open section. Toggles, sliders and texts
  /// come back from the platform on every load.
  Map<String, dynamic> snapshot() => {
    'version': 1,
    'section': _section.name,
  };

  void restore(Map<String, dynamic> json) {
    final s = sectionIdByName('${json['section']}');
    if (s != null) _section = s;
    notifyListeners();
  }
}
