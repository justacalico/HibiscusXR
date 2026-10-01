import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'adb_runner.dart';
import 'models.dart';
import 'plan.dart';

/// Opens the dump file a partition writes to. Injectable so tests never
/// touch the filesystem.
typedef OpenSink = IOSink Function(String path);

enum ItemPhase { pending, running, done, failed }

/// Live state of one partition's dump.
class BackupItem {
  BackupItem(this.partition);

  final Partition partition;
  ItemPhase phase = ItemPhase.pending;
  int written = 0;
  String? error;

  double get fraction => partition.sizeBytes <= 0
      ? (phase == ItemPhase.done ? 1 : 0)
      : (written / partition.sizeBytes).clamp(0.0, 1.0);
}

/// Snapshot the engine emits on every state change.
class BackupSnapshot {
  const BackupSnapshot({
    required this.items,
    required this.current,
    required this.finished,
    required this.cancelled,
    required this.manifestWritten,
  });

  final List<BackupItem> items;

  /// Index of the partition currently dumping, -1 when idle.
  final int current;
  final bool finished;
  final bool cancelled;

  /// manifest.txt landed next to the dumps.
  final bool manifestWritten;

  int get doneCount =>
      items.where((i) => i.phase == ItemPhase.done).length;
  bool get anyFailed => items.any((i) => i.phase == ItemPhase.failed);
  int get writtenTotal => items.fold(0, (s, i) => s + i.written);
}

/// Cooperative cancel flag - the UI flips it, the engine checks between
/// chunks and partitions.
class CancelToken {
  bool cancelled = false;
}

/// Runs a full-disk backup: dd every by-name partition out of the
/// headset into the destination folder, then write manifest.txt.
class BackupEngine {
  BackupEngine({
    required this._adb,
    OpenSink? openSink,
  }) : _openSink = openSink ?? ((p) => File(p).openWrite());

  final AdbRunner _adb;
  final OpenSink _openSink;

  /// Dumps every partition in [plan]. Emits a snapshot on start, on every
  /// chunk, and on each phase change. Set [token].cancelled to stop - the
  /// current file is left in place, marked failed.
  Stream<BackupSnapshot> run(String serial, BackupPlan plan,
      {CancelToken? token}) async* {
    final items = [for (final p in plan.partitions) BackupItem(p)];
    var current = -1;
    var cancelled = false;
    var manifestWritten = false;

    BackupSnapshot snap() => BackupSnapshot(
          items: items,
          current: current,
          finished: false,
          cancelled: cancelled,
          manifestWritten: manifestWritten,
        );

    yield snap();
    for (var i = 0; i < items.length; i++) {
      if (token?.cancelled ?? false) {
        cancelled = true;
        break;
      }
      final item = items[i];
      item.phase = ItemPhase.running;
      current = i;
      yield snap();
      IOSink? sink;
      try {
        sink = _openSink('${plan.destDir}/${item.partition.fileName}');
        await for (final chunk in _adb.execOut(
            serial, 'dd if=${item.partition.path} bs=4M 2>/dev/null')) {
          if (token?.cancelled ?? false) {
            cancelled = true;
            break;
          }
          sink.add(chunk);
          item.written += chunk.length;
          yield snap();
        }
        await sink.flush();
        await sink.close();
      } catch (e) {
        item.error = e.toString();
        try {
          await sink?.close();
        } catch (_) {}
      }
      if (cancelled || (token?.cancelled ?? false)) {
        cancelled = true;
        item.phase = ItemPhase.failed;
        item.error ??= 'cancelled';
        yield snap();
        break;
      }
      if (item.error == null &&
          item.written != item.partition.sizeBytes &&
          item.partition.sizeBytes > 0) {
        item.error = 'size mismatch';
      }
      item.phase =
          item.error == null ? ItemPhase.done : ItemPhase.failed;
      yield snap();
      if (item.phase == ItemPhase.failed) break;
    }
    current = -1;
    if (!cancelled && !items.any((i) => i.phase == ItemPhase.failed)) {
      try {
        final sink = _openSink('${plan.destDir}/manifest.txt');
        sink.add(utf8.encode(manifestFor(serial, plan.partitions)));
        await sink.flush();
        await sink.close();
        manifestWritten = true;
      } catch (_) {
        manifestWritten = false;
      }
    }
    yield BackupSnapshot(
      items: items,
      current: current,
      finished: !cancelled,
      cancelled: cancelled,
      manifestWritten: manifestWritten,
    );
  }
}
