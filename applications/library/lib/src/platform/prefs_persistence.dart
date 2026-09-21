import 'dart:convert';

import 'package:shared_preferences/shared_preferences.dart';

import '../persistence.dart';

/// Snapshot storage in SharedPreferences as one JSON blob.
class PrefsPersistence implements LibraryPersistence {
  PrefsPersistence(this._prefs);

  static const String storageKey = 'library.snapshot.v1';

  final SharedPreferences _prefs;

  static Future<PrefsPersistence> open() async =>
      PrefsPersistence(await SharedPreferences.getInstance());

  @override
  Future<LibrarySnapshot?> load() async {
    final raw = _prefs.getString(storageKey);
    if (raw == null) return null;
    try {
      final decoded = jsonDecode(raw);
      if (decoded is! Map<String, dynamic>) return null;
      return LibrarySnapshot.fromJson(decoded);
    } on FormatException {
      return null;
    }
  }

  @override
  Future<void> save(LibrarySnapshot snapshot) =>
      _prefs.setString(storageKey, jsonEncode(snapshot.toJson()));
}
