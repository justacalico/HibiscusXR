import 'dart:convert';

/// One downloadable build of an app, parsed from a `versions` entry in
/// the F-Droid index-v2 format.
class AppVersion {
  const AppVersion({
    required this.versionCode,
    required this.versionName,
    required this.apkPath,
    required this.size,
    required this.added,
    this.minSdk,
  });

  final int versionCode;
  final String versionName;

  /// `file.name` verbatim - it may carry a leading slash, join it with
  /// [repoFileUri] instead of string math.
  final String apkPath;
  final int size;
  final DateTime added;
  final int? minSdk;
}

/// An app entry from the repository index.
class StoreApp {
  const StoreApp({
    required this.packageName,
    required this.name,
    required this.summary,
    required this.description,
    required this.categories,
    required this.added,
    required this.lastUpdated,
    required this.versions,
    this.author,
    this.license,
    this.iconPath,
    this.screenshots = const [],
  });

  final String packageName;
  final String name;
  final String summary;
  final String description;
  final List<String> categories;
  final DateTime added;
  final DateTime lastUpdated;

  /// Sorted newest first by versionCode; empty when the index lists no
  /// downloadable builds.
  final List<AppVersion> versions;

  final String? author;
  final String? license;
  final String? iconPath;
  final List<String> screenshots;

  AppVersion? get latest => versions.isEmpty ? null : versions.first;
}

/// A parsed `index-v2.json`.
class RepoIndex {
  const RepoIndex({
    required this.name,
    required this.address,
    required this.timestamp,
    required this.apps,
  });

  final String name;

  /// The repository's own address field. File paths in the index are
  /// relative to this, not to wherever the index was fetched from.
  final String address;
  final DateTime timestamp;
  final List<StoreApp> apps;
}

/// Repo-relative file resolution. Index `file.name` entries sometimes
/// lead with a slash, and the base may or may not end with one.
Uri repoFileUri(Uri repo, String name) {
  final base = repo.path.endsWith('/') ? repo.path : '${repo.path}/';
  final rel = name.startsWith('/') ? name.substring(1) : name;
  return repo.replace(path: base + rel);
}

/// Localized values look like `{"en-US": "text", "de": "..."}`. Pick the
/// requested locale, then its language, then en-US, then anything.
String localized(
  Map<String, dynamic>? values, {
  String locale = 'en-US',
  String fallback = '',
}) {
  if (values == null || values.isEmpty) return fallback;
  final lang = locale.split('-').first;
  for (final key in [locale, lang, 'en-US', 'en']) {
    final hit = values[key];
    if (hit is String && hit.isNotEmpty) return hit;
  }
  final first = values.values.whereType<String>().firstOrNull;
  return first ?? fallback;
}

/// Localized file entries look like `{"en-US": {"name": ...}}`.
String? localizedFileName(Map<String, dynamic>? values, {String locale = 'en-US'}) {
  if (values == null || values.isEmpty) return null;
  final lang = locale.split('-').first;
  for (final key in [locale, lang, 'en-US', 'en']) {
    final entry = values[key];
    if (entry is Map && entry['name'] is String) {
      return entry['name'] as String;
    }
  }
  for (final entry in values.values) {
    if (entry is Map && entry['name'] is String) {
      return entry['name'] as String;
    }
  }
  return null;
}

DateTime _millis(Object? v) =>
    DateTime.fromMillisecondsSinceEpoch(v is int ? v : 0, isUtc: true);

Map<String, dynamic> _asMap(Object? v) =>
    v is Map ? v.cast<String, dynamic>() : const {};

Map<String, dynamic>? _asMapOrNull(Object? v) =>
    v is Map ? v.cast<String, dynamic>() : null;

AppVersion _parseVersion(Map<String, dynamic> v) {
  final manifest = _asMap(v['manifest']);
  final file = _asMap(v['file']);
  final usesSdk = _asMap(manifest['usesSdk']);
  return AppVersion(
    versionCode: manifest['versionCode'] as int? ?? 0,
    versionName: manifest['versionName'] as String? ?? '',
    apkPath: file['name'] as String? ?? '',
    size: file['size'] as int? ?? 0,
    added: _millis(v['added']),
    minSdk: usesSdk['minSdkVersion'] as int?,
  );
}

/// Screenshots are grouped per form factor; prefer phone, then walk the
/// larger buckets before wear.
List<String> _screenshots(Map<String, dynamic>? shots, String locale) {
  if (shots == null) return const [];
  for (final group in ['phone', 'sevenInch', 'tenInch', 'tv', 'wear']) {
    final localizedGroup = shots[group];
    if (localizedGroup is! Map) continue;
    final map = localizedGroup.cast<String, dynamic>();
    final list = map[locale] ??
        map[locale.split('-').first] ??
        map['en-US'] ??
        map.values.whereType<List>().firstOrNull;
    if (list is List && list.isNotEmpty) {
      return [
        for (final shot in list)
          if (shot is Map && shot['name'] is String) shot['name'] as String,
      ];
    }
  }
  return const [];
}

/// Parse a decoded index-v2 document. Throws [FormatException] when the
/// top level does not look like a repo index.
RepoIndex parseFdroidIndex(Map<String, dynamic> json) {
  final repo = json['repo'];
  final packages = json['packages'];
  if (repo is! Map || packages is! Map) {
    throw const FormatException('not an index-v2 document');
  }
  final repoMap = repo.cast<String, dynamic>();
  final packagesMap = packages.cast<String, dynamic>();
  final address = repoMap['address'] as String? ?? '';
  final apps = <StoreApp>[
    for (final entry in packagesMap.entries)
      if (entry.value is Map)
        _parseApp(entry.key, (entry.value as Map).cast<String, dynamic>()),
  ];
  return RepoIndex(
    name: localized(_asMapOrNull(repoMap['name']), fallback: address),
    address: address,
    timestamp: _millis(repo['timestamp']),
    apps: apps,
  );
}

RepoIndex decodeFdroidIndex(String body) =>
    parseFdroidIndex(jsonDecode(body) as Map<String, dynamic>);

StoreApp _parseApp(String packageName, Map<String, dynamic> pkg) {
  final meta = _asMap(pkg['metadata']);
  final rawVersions = pkg['versions'];
  final versions = <AppVersion>[
    if (rawVersions is Map)
      for (final v in rawVersions.values)
        if (v is Map) _parseVersion(v.cast<String, dynamic>()),
  ]..sort((a, b) => b.versionCode.compareTo(a.versionCode));
  final rawCategories = meta['categories'];
  return StoreApp(
    packageName: packageName,
    name: localized(_asMapOrNull(meta['name']), fallback: packageName),
    summary: localized(_asMapOrNull(meta['summary'])),
    description: localized(_asMapOrNull(meta['description'])),
    categories: [
      if (rawCategories is List)
        for (final c in rawCategories)
          if (c is String) c,
    ],
    added: _millis(meta['added']),
    lastUpdated: _millis(meta['lastUpdated']),
    versions: versions,
    author: meta['authorName'] as String?,
    license: meta['license'] as String?,
    iconPath: localizedFileName(_asMapOrNull(meta['icon'])),
    screenshots: _screenshots(_asMapOrNull(meta['screenshots']), 'en-US'),
  );
}
