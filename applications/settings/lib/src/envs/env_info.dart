import 'dart:convert';
import 'dart:typed_data';

import 'env_zip.dart';

/// Reserved selection values for the hibiscus_environment global key.
/// Anything else is an environment id - the zip's filename without the
/// extension - resolved by the shell to `kEnvDir/<id>.zip`.
const kEnvPassthrough = 'passthrough';
const kEnvBuiltin = 'builtin';

/// Where environment zips are pushed (adb push x.zip $kEnvDir/). The dir
/// is created world-readable by init so both this app and the native home
/// shell can reach it.
const kEnvDir = '/data/local/tmp/hibiscus/envs';

/// Ids cross a shell-script mirror and become filenames, so the charset is
/// the same one the native side enforces: letters, digits, dot, dash,
/// underscore, 1-64 chars, never containing "..".
bool envIdValid(String id) {
  if (id.isEmpty || id.length > 64 || id.contains('..')) return false;
  for (final c in id.codeUnits) {
    final ok =
        (c >= 0x61 && c <= 0x7a) ||
        (c >= 0x41 && c <= 0x5a) ||
        (c >= 0x30 && c <= 0x39) ||
        c == 0x2e ||
        c == 0x2d ||
        c == 0x5f;
    if (!ok) return false;
  }
  return true;
}

/// "skyloft.zip" -> "skyloft"
String envIdFromZipName(String fileName) {
  final base = fileName.split('/').last;
  return base.toLowerCase().endsWith('.zip')
      ? base.substring(0, base.length - 4)
      : base;
}

/// The stored selection normalized for display: absent or empty means the
/// camera feed, same as a fresh install.
String envSelOr(String? id) =>
    id == null || id.isEmpty ? kEnvPassthrough : id;

/// Metadata the home-environment card shows for one zip. `thumb` carries
/// the optional map.png bytes; a zip that won't open or has no map.obj
/// still lists so the user can see - and delete - the bad file.
class EnvOption {
  const EnvOption({
    required this.id,
    this.name = '',
    this.version = '',
    this.license = '',
    this.git = '',
    this.homepage = '',
    this.created = '',
    this.thumb,
    this.readable = true,
    this.hasMap = true,
  });

  final String id;
  final String name;
  final String version;
  final String license;
  final String git;
  final String homepage;
  final String created;
  final Uint8List? thumb;
  final bool readable;
  final bool hasMap;

  /// Display name: the map.json name when present, else the id.
  String get label => name.isEmpty ? id : name;

  /// Selectable means the shell can actually render it.
  bool get selectable => readable && hasMap && envIdValid(id);
}

String _str(Map<String, dynamic> json, String key) =>
    json[key] is String ? json[key] as String : '';

/// map.json -> metadata. A missing or malformed file yields all-empty
/// fields; callers fall back to the zip's filename for the name.
EnvOption envFromZip(String id, Uint8List bytes) {
  final entries = zipEntries(bytes);
  if (entries == null) {
    return EnvOption(id: id, readable: false, hasMap: false);
  }
  final names = {for (final e in entries) e.name};
  final jsonBytes = zipEntryData(bytes, 'map.json');
  var name = '', version = '', license = '', git = '', homepage = '',
      created = '';
  if (jsonBytes != null) {
    try {
      final json = jsonDecode(utf8.decode(jsonBytes));
      if (json is Map<String, dynamic>) {
        name = _str(json, 'name');
        version = _str(json, 'version');
        license = _str(json, 'license');
        git = _str(json, 'git');
        homepage = _str(json, 'homepage');
        created = _str(json, 'created');
      }
    } on FormatException {
      // bad json still leaves a working map.obj-only entry
    }
  }
  return EnvOption(
    id: id,
    name: name,
    version: version,
    license: license,
    git: git,
    homepage: homepage,
    created: created,
    thumb: zipEntryData(bytes, 'map.png'),
    hasMap: names.contains('map.obj'),
  );
}
