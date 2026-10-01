import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

/// Injectable seams so tests never touch a real adb binary.
typedef AdbRun = Future<ProcessResult> Function(List<String> args);
typedef AdbSpawn = Future<Process> Function(List<String> args);

/// Thin wrapper over the `adb` executable.
class AdbRunner {
  AdbRunner({this.adbPath = 'adb', AdbRun? run, AdbSpawn? spawn})
      : _run = run ?? ((args) => Process.run(adbPath, args)),
        _spawn = spawn ?? ((args) => Process.start(adbPath, args));

  final String adbPath;
  final AdbRun _run;
  final AdbSpawn _spawn;

  Future<ProcessResult> run(List<String> args) => _run(args);

  Future<Process> spawn(List<String> args) => _spawn(args);

  Future<String> shellText(String serial, String command) async {
    final r = await run(['-s', serial, 'shell', command]);
    return (r.stdout as String? ?? '') + (r.stderr as String? ?? '');
  }

  /// `adb exec-out` byte stream - the dd data channel.
  Stream<Uint8List> execOut(String serial, String command) async* {
    final p = await spawn(
        ['-s', serial, 'exec-out', command]);
    yield* p.stdout.map((c) => c is Uint8List ? c : Uint8List.fromList(c));
    await p.exitCode;
  }

  /// `adb devices -l` rows.
  Future<List<AdbDeviceRow>> devices() async {
    final r = await run(['devices', '-l']);
    return parseAdbDevices(r.stdout as String? ?? '');
  }
}

class AdbDeviceRow {
  const AdbDeviceRow(this.serial, this.state, this.model);
  final String serial, state, model;
  bool get isReady => state == 'device';
}

List<AdbDeviceRow> parseAdbDevices(String out) {
  final rows = <AdbDeviceRow>[];
  for (final line in out.split('\n')) {
    final t = line.trim();
    if (t.isEmpty || t.startsWith('List of devices') || t.startsWith('*')) {
      continue;
    }
    final parts = t.split(RegExp(r'\s+'));
    if (parts.length < 2) continue;
    var model = '';
    for (final kv in parts.skip(2)) {
      if (kv.startsWith('model:')) model = kv.substring(6);
    }
    rows.add(AdbDeviceRow(parts[0], parts[1], model));
  }
  return rows;
}
