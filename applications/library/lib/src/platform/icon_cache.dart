import 'dart:typed_data';

import 'app_source.dart';

/// Caches icon futures so the grid does not re-hit the platform channel on
/// every rebuild. [invalidate] drops one package or, with no argument,
/// everything (used when the catalog changes).
class IconCache {
  IconCache(this._source);

  final AppSource _source;
  final _cache = <String, Future<Uint8List?>>{};

  Future<Uint8List?> get(String packageName) =>
      _cache.putIfAbsent(packageName, () => _source.icon(packageName));

  void invalidate([String? packageName]) {
    if (packageName == null) {
      _cache.clear();
    } else {
      _cache.remove(packageName);
    }
  }
}
