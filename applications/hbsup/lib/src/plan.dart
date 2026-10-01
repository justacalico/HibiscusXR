import 'dart:io';

import 'models.dart';

/// `df -B1`-style probes are injectable so the space check runs on the
/// host in tests without touching real disks.
typedef RunProc = Future<ProcessResult> Function(
    String exe, List<String> args);
typedef FreeSpaceProbe = Future<int?> Function(String dir);

/// Where by-name partition tables live on the headsets we support.
const byNameDirs = [
  '/dev/block/bootdevice/by-name',
  '/dev/block/by-name',
];

/// A full-disk backup: every partition plus where the dump lands.
class BackupPlan {
  const BackupPlan({required this.destDir, required this.partitions});

  final String destDir;
  final List<Partition> partitions;

  int get totalBytes =>
      partitions.fold(0, (sum, p) => sum + p.sizeBytes);
}

/// Result of comparing a plan against free space at the destination.
class SpaceCheck {
  const SpaceCheck({required this.needed, required this.free});

  final int needed;

  /// Free bytes at the destination, null when the probe could not tell.
  final int? free;

  /// True when we know the dump fits, or when free space is unknown -
  /// unknown never blocks a backup, it just warns less.
  bool get fits => free == null || free! >= needed;

  int get shortfall => free == null || free! >= needed ? 0 : needed - free!;
}

SpaceCheck checkSpace(BackupPlan plan, int? freeBytes) =>
    SpaceCheck(needed: plan.totalBytes, free: freeBytes);

/// Human-readable byte counts: 512 B, 4.0 MB, 3.6 GB.
String formatBytes(int bytes) {
  if (bytes < 1024) return '$bytes B';
  const units = ['KB', 'MB', 'GB', 'TB'];
  var v = bytes / 1024.0;
  var i = 0;
  while (v >= 1024 && i < units.length - 1) {
    v /= 1024;
    i++;
  }
  return '${v.toStringAsFixed(1)} ${units[i]}';
}

/// manifest.txt written next to the dumps - the restore reference.
String manifestFor(String serial, List<Partition> parts) {
  final b = StringBuffer()
    ..writeln('HBSUP full-disk backup')
    ..writeln('serial: $serial')
    ..writeln('partitions: ${parts.length}');
  for (final p in parts) {
    b.writeln('${p.name}\t${p.path}\t${p.sizeBytes}\t${p.fileName}');
  }
  return b.toString();
}

/// Free bytes of the filesystem holding [dir]. `df -B1` on Linux/macOS,
/// fsutil on Windows. Returns null when nothing could be parsed.
/// [isWindows] exists so the Windows branch is testable off-Windows.
Future<int?> freeSpaceOf(String dir,
    {RunProc? proc, bool? isWindows}) async {
  final run = proc ?? Process.run;
  if (isWindows ?? Platform.isWindows) {
    return _fsutilFree(dir, run);
  }
  return _dfFree(dir, run);
}

/// `df -B1 <dir>` second row, column 4 (available 1-byte blocks).
Future<int?> _dfFree(String dir, RunProc run) async {
  try {
    final r = await run('df', ['-B1', dir]);
    final lines = (r.stdout as String? ?? '').trim().split('\n');
    if (lines.length < 2) return null;
    final cols = lines[1].split(RegExp(r'\s+'));
    if (cols.length < 4) return null;
    return int.tryParse(cols[3]);
  } catch (_) {
    return null;
  }
}

/// `fsutil volume diskfree <drive>` - "Total # of free bytes" line.
Future<int?> _fsutilFree(String dir, RunProc run) async {
  try {
    final drive = dir.length >= 2 && dir[1] == ':' ? dir.substring(0, 2) : dir;
    final r = await run('fsutil', ['volume', 'diskfree', drive]);
    for (final line in (r.stdout as String? ?? '').split('\n')) {
      final m = RegExp(r'(\d[\d,]*)').allMatches(line).toList();
      if (line.toLowerCase().contains('free') && m.isNotEmpty) {
        return int.tryParse(m.first.group(1)!.replaceAll(',', ''));
      }
    }
    return null;
  } catch (_) {
    return null;
  }
}
