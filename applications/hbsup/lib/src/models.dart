/// One partition in the headset's by-name table.
class Partition {
  const Partition(this.name, this.path, this.sizeBytes);

  /// by-name alias, e.g. `boot`.
  final String name;

  /// Kernel block device path the alias resolves to, e.g. /dev/block/sde17.
  final String path;

  final int sizeBytes;

  /// Name of the dump file inside the backup folder.
  String get fileName => '$name.img';
}

final _nameRe = RegExp(r'^[A-Za-z0-9._-]+$');
final _pathRe = RegExp(r'^/[A-Za-z0-9._/-]+$');

/// Parses `ls -l <by-name dir>` output into (name, target) pairs:
///   boot -> /dev/block/sde17
/// Relative targets resolve against [dir]. Names and paths are
/// whitelisted - ls output off a hostile endpoint must never become a
/// filename or a shell fragment.
List<({String name, String path})> parseByNameLs(String dir, String out) {
  final found = <({String name, String path})>[];
  for (final line in out.split('\n')) {
    final t = line.trim();
    if (t.isEmpty || t.startsWith('total')) continue;
    final arrow = t.lastIndexOf(' -> ');
    if (arrow < 0) continue;
    final target = t.substring(arrow + 4).trim();
    final left = t.substring(0, arrow).trim();
    final name = left.split(RegExp(r'\s+')).last;
    if (!_nameRe.hasMatch(name) || name == '.' || name == '..') {
      continue;
    }
    final path = target.startsWith('/') ? target : '$dir/$target';
    if (!_pathRe.hasMatch(path)) continue;
    found.add((name: name, path: path));
  }
  return found;
}

/// The last path segment, e.g. sde17 out of /dev/block/sde17.
String basenameOf(String path) {
  final i = path.lastIndexOf('/');
  return i < 0 ? path : path.substring(i + 1);
}

/// `cat /sys/class/block/<dev>/size` reports 512-byte sectors.
int? parseSectorCount(String out) {
  final v = int.tryParse(out.trim());
  if (v == null || v < 0) return null;
  return v;
}

int sectorsToBytes(int sectors) => sectors * 512;
