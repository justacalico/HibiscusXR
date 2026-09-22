import 'package:flutter/foundation.dart';

import 'models.dart';

/// All app state: toggle values, slider positions, platform-reported
/// texts and the selected sidebar section. Pure Dart -
/// every value the UI shows is computed here so tests can reach it.
class SettingsStore extends ChangeNotifier {
  final Map<ItemId, bool> _toggles = {};
  final Map<ItemId, double> _sliders = {};
  final Map<ItemId, String> _texts = {};
  final Map<ItemId, ControllerInfo> _controllers = {};
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

  /// Last reported state of a controller row, or a placeholder when the
  /// service has not answered yet.
  ControllerInfo controllerOf(ItemId id) =>
      _controllers[id] ?? const ControllerInfo();

  /// Merge a platform snapshot or change event. Only the keys the
  /// snapshot carries are touched, so partial updates work.
  void applySnapshot(SettingsSnapshot snap) {
    _toggles.addAll(snap.toggles);
    _sliders.addAll(snap.sliders);
    _texts.addAll(snap.texts);
    _controllers.addAll(snap.controllers);
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
