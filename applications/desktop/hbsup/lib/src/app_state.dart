import 'dart:async';
import 'dart:collection';
import 'dart:io';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/foundation.dart';

import 'adb_runner.dart';
import 'engine.dart';
import 'models.dart';
import 'plan.dart';

enum ScanState { idle, scanning }

enum BackupState { idle, running, done, failed, cancelled }

/// Everything the UI reads lives here; created once above MaterialApp so
/// window resizes never touch it.
class AppState extends ChangeNotifier {
  AppState({
    AdbRunner? adb,
    String? hostOs,
    FreeSpaceProbe? freeSpace,
    this._openSink,
    Future<String?> Function()? pickDir,
  })  : _adb = adb ?? AdbRunner(),
        hostOs = hostOs ?? Platform.operatingSystem,
        _freeSpace = freeSpace ?? freeSpaceOf,
        _pickDir = pickDir ?? FilePicker.getDirectoryPath;

  final AdbRunner _adb;
  final FreeSpaceProbe _freeSpace;
  final OpenSink? _openSink;
  final Future<String?> Function() _pickDir;

  /// Host OS the app runs on. Windows builds exist but are not a
  /// supported host - the banner says so on open.
  final String hostOs;
  bool get unsupportedHost => hostOs == 'windows';
  bool warningDismissed = false;

  ScanState scanState = ScanState.idle;
  List<AdbDeviceRow> devices = const [];
  String? lastError;

  String? activeSerial;
  List<Partition>? partitions;
  bool loadingPartitions = false;

  String? destDir;
  int? destFreeBytes;

  BackupState backupState = BackupState.idle;
  List<BackupItem> items = const [];
  int currentItem = -1;
  bool manifestWritten = false;
  final Queue<String> logBuf = ListQueue(400);

  CancelToken? _cancel;

  bool get connected => activeSerial != null;
  bool get running => backupState == BackupState.running;

  BackupPlan? get plan =>
      destDir == null || partitions == null || partitions!.isEmpty
          ? null
          : BackupPlan(destDir: destDir!, partitions: partitions!);

  SpaceCheck? get space {
    final p = plan;
    return p == null ? null : checkSpace(p, destFreeBytes);
  }

  int get writtenTotal =>
      items.fold(0, (s, i) => s + i.written);

  void dismissWarning() {
    warningDismissed = true;
    notifyListeners();
  }

  void _log(String line) {
    logBuf.addLast(line);
    if (logBuf.length > 400) logBuf.removeFirst();
  }

  Future<void> scanAdb() async {
    scanState = ScanState.scanning;
    lastError = null;
    notifyListeners();
    try {
      devices = await _adb.devices();
    } catch (e) {
      devices = const [];
      lastError = e.toString();
    }
    scanState = ScanState.idle;
    notifyListeners();
  }

  /// Picks a headset and pulls its partition table. Resets any state
  /// left over from a previous session.
  Future<void> connect(String serial) async {
    activeSerial = serial;
    partitions = null;
    loadingPartitions = true;
    lastError = null;
    backupState = BackupState.idle;
    items = const [];
    currentItem = -1;
    manifestWritten = false;
    notifyListeners();
    try {
      partitions = await _readPartitions(serial) ?? const [];
      if (partitions!.isEmpty) {
        lastError = 'no partitions found';
      }
    } catch (e) {
      lastError = e.toString();
      partitions = const [];
    }
    loadingPartitions = false;
    notifyListeners();
  }

  /// Walks the by-name dirs until one answers, then sizes each partition
  /// off /sys/class/block. Returns null when no table was found.
  Future<List<Partition>?> _readPartitions(String serial) async {
    for (final dir in byNameDirs) {
      final listing = await _adb.shellText(serial, 'ls -l $dir');
      final entries = parseByNameLs(dir, listing);
      if (entries.isEmpty) continue;
      final parts = <Partition>[];
      for (final e in entries) {
        final dev = basenameOf(e.path);
        final sizeOut =
            await _adb.shellText(serial, 'cat /sys/class/block/$dev/size');
        final sectors = parseSectorCount(sizeOut);
        parts.add(Partition(e.name, e.path,
            sectors == null ? 0 : sectorsToBytes(sectors)));
      }
      return parts;
    }
    return null;
  }

  /// Asks the user for a folder, then probes its free space. Picker
  /// failures (no portal, no plugin) just leave the folder unset.
  Future<void> pickDestination() async {
    try {
      final dir = await _pickDir();
      if (dir != null) await setDestination(dir);
    } catch (_) {}
  }

  Future<void> setDestination(String dir) async {
    destDir = dir;
    destFreeBytes = null;
    notifyListeners();
    destFreeBytes = await _freeSpace(dir);
    notifyListeners();
  }

  void disconnect() {
    _cancel?.cancelled = true;
    activeSerial = null;
    partitions = null;
    destDir = null;
    destFreeBytes = null;
    lastError = null;
    items = const [];
    currentItem = -1;
    manifestWritten = false;
    backupState = BackupState.idle;
    logBuf.clear();
    notifyListeners();
  }

  Future<void> startBackup() async {
    final p = plan;
    final serial = activeSerial;
    if (p == null || serial == null || running) return;
    _cancel = CancelToken();
    backupState = BackupState.running;
    _log('dumping ${p.partitions.length} partitions to ${p.destDir}');
    manifestWritten = false;
    notifyListeners();
    final engine = BackupEngine(adb: _adb, openSink: _openSink);
    final logged = <String>{};
    var cancelled = false;
    await for (final s in engine.run(serial, p, token: _cancel)) {
      items = s.items;
      currentItem = s.current;
      manifestWritten = s.manifestWritten;
      cancelled = s.cancelled;
      for (final item in s.items) {
        if (item.phase == ItemPhase.failed &&
            item.error != null &&
            logged.add(item.partition.name)) {
          _log('${item.partition.name}: ${item.error}');
        }
      }
      notifyListeners();
    }
    // a disconnect mid-run already reset everything - don't clobber it
    if (activeSerial != serial) return;
    if (cancelled) {
      backupState = BackupState.cancelled;
      _log('backup cancelled');
    } else if (items.any((i) => i.phase == ItemPhase.failed)) {
      backupState = BackupState.failed;
      _log('backup failed');
    } else {
      backupState = BackupState.done;
      _log('backup finished');
    }
    notifyListeners();
  }

  void cancel() => _cancel?.cancelled = true;
}
