import 'dart:convert';

import 'package:shared_preferences/shared_preferences.dart';

import '../persistence.dart';

/// SharedPreferences-backed snapshot storage.
class PrefsPersistence implements SettingsPersistence {
  PrefsPersistence._(this._prefs);

  static const _key = 'settings_snapshot';

  final SharedPreferences _prefs;

  static Future<PrefsPersistence> open() async =>
      PrefsPersistence._(await SharedPreferences.getInstance());

  @override
  Future<Map<String, dynamic>?> load() async {
    final raw = _prefs.getString(_key);
    if (raw == null) return null;
    final decoded = jsonDecode(raw);
    return decoded is Map ? decoded.cast<String, dynamic>() : null;
  }

  @override
  Future<void> save(Map<String, dynamic> snapshot) =>
      _prefs.setString(_key, jsonEncode(snapshot)).then((_) {});
}
