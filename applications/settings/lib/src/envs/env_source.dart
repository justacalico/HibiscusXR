import 'dart:io';

import 'env_info.dart';

/// Where the environment list comes from. Implementations stay thin:
/// scan a dir, read the zips, delete one on request. The selection itself
/// is a Settings.Global write and rides the regular text channel.
abstract class EnvSource {
  /// Every *.zip in the directory, best-effort parsed. Unreadable files
  /// still come back as invalid entries so they can be managed.
  Future<List<EnvOption>> list();

  /// Delete one environment zip by id. False when the file is gone or the
  /// filesystem refused.
  Future<bool> remove(String id);
}

/// Real backend: kEnvDir on shared storage. dart:io file access is the
/// only platform touch; all parsing is in env_info.dart.
class DirEnvSource implements EnvSource {
  DirEnvSource([this.path = kEnvDir]);

  final String path;

  @override
  Future<List<EnvOption>> list() async {
    final dir = Directory(path);
    final out = <EnvOption>[];
    try {
      if (!await dir.exists()) return out;
      final files = await dir
          .list()
          .where((e) => e is File && e.path.toLowerCase().endsWith('.zip'))
          .toList();
      files.sort((a, b) => a.path.compareTo(b.path));
      for (final f in files) {
        final id = envIdFromZipName(f.path);
        try {
          out.add(envFromZip(id, await File(f.path).readAsBytes()));
        } catch (_) {
          out.add(EnvOption(id: id, readable: false, hasMap: false));
        }
      }
    } catch (_) {
      // unreadable dir -> empty list, the UI shows the install hint
    }
    return out;
  }

  @override
  Future<bool> remove(String id) async {
    if (!envIdValid(id)) return false;
    try {
      await File('$path/$id.zip').delete();
      return true;
    } catch (_) {
      return false;
    }
  }
}

/// For tests and desktop previews: nothing installed, nothing managed.
class EmptyEnvSource implements EnvSource {
  const EmptyEnvSource();

  @override
  Future<List<EnvOption>> list() async => const [];

  @override
  Future<bool> remove(String id) async => false;
}
