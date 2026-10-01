import 'package:shared_preferences/shared_preferences.dart';

import '../persistence.dart';

/// SharedPreferences-backed store. The snapshot is tiny (the repo URL),
/// so it travels as flat keys.
class PrefsPersistence implements StorePersistence {
  PrefsPersistence._(this._prefs);

  final SharedPreferences _prefs;

  static const _kRepoUrl = 'repoUrl';

  static Future<PrefsPersistence> open() async =>
      PrefsPersistence._(await SharedPreferences.getInstance());

  @override
  Future<Map<String, dynamic>?> load() async {
    final url = _prefs.getString(_kRepoUrl);
    if (url == null) return null;
    return {_kRepoUrl: url};
  }

  @override
  Future<void> save(Map<String, dynamic> snapshot) async {
    final url = snapshot[_kRepoUrl];
    if (url is String) {
      await _prefs.setString(_kRepoUrl, url);
    }
  }
}
