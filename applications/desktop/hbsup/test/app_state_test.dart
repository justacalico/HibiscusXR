import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_hbsup/src/adb_runner.dart';
import 'package:hibiscus_hbsup/src/app_state.dart';
import 'package:hibiscus_hbsup/src/engine.dart';

const _byName = '/dev/block/bootdevice/by-name';
const _ls = '''
total 0
lrwxrwxrwx 1 root root 1970 boot -> /dev/block/sde17
lrwxrwxrwx 1 root root 1970 system -> /dev/block/sde32
''';

/// adb where `shell` answers text commands and `exec` answers dd.
class FakeAdb extends AdbRunner {
  FakeAdb({
    AdbRun? run,
    Map<String, String>? shell,
    Map<String, Stream<Uint8List>>? exec,
    this.onExec,
  })  : _shell = shell ?? const {},
        _exec = exec ?? const {},
        super(
          run: run ?? (_) async => ProcessResult(0, 0, '', ''),
          spawn: (_) => Process.start('echo', const []),
        );

  final Map<String, String> _shell;
  final Map<String, Stream<Uint8List>> _exec;
  final void Function(String command)? onExec;

  @override
  Future<String> shellText(String serial, String command) async =>
      _shell[command] ?? '';

  @override
  Stream<Uint8List> execOut(String serial, String command) {
    onExec?.call(command);
    return _exec[command] ?? const Stream.empty();
  }
}

String ddOf(String path) => 'dd if=$path bs=4M 2>/dev/null';

Map<String, String> get _shell => {
      'ls -l $_byName': _ls,
      'cat /sys/class/block/sde17/size': '2048',
      'cat /sys/class/block/sde32/size': '8388608',
    };

Map<String, Stream<Uint8List>> get _exec => {
      ddOf('/dev/block/sde17'): Stream.value(Uint8List(2048 * 512)),
      ddOf('/dev/block/sde32'): Stream.value(Uint8List(4)),
    };

AppState fakeState({
  FakeAdb? adb,
  String? hostOs,
  int freeBytes = 1 << 40,
  OpenSink? openSink,
  Future<String?> Function()? pickDir,
}) =>
    AppState(
      adb: adb ?? FakeAdb(shell: _shell, exec: _exec),
      hostOs: hostOs ?? 'linux',
      freeSpace: (_) async => freeBytes,
      openSink: openSink,
      pickDir: pickDir,
    );

Future<AppState> connectedState({String? hostOs, FakeAdb? adb}) async {
  final s = fakeState(hostOs: hostOs, adb: adb);
  await s.connect('SER');
  return s;
}

void main() {
  test('host os flags windows as unsupported', () {
    expect(fakeState(hostOs: 'windows').unsupportedHost, isTrue);
    expect(fakeState(hostOs: 'linux').unsupportedHost, isFalse);
    expect(fakeState(hostOs: 'macos').unsupportedHost, isFalse);
  });

  test('dismissWarning flips the flag', () {
    final s = fakeState(hostOs: 'windows');
    expect(s.warningDismissed, isFalse);
    s.dismissWarning();
    expect(s.warningDismissed, isTrue);
  });

  group('scanAdb', () {
    test('lists ready devices', () async {
      final adb = FakeAdb(run: (args) async => ProcessResult(
          0, 0, 'List of devices attached\nSER9\tdevice model:Neo2\n', ''));
      final s = fakeState(adb: adb);
      await s.scanAdb();
      expect(s.devices.single.serial, 'SER9');
      expect(s.scanState, ScanState.idle);
      expect(s.lastError, isNull);
    });

    test('adb errors land in lastError', () async {
      final adb = FakeAdb(
          run: (args) async => throw const ProcessException('adb', []));
      final s = fakeState(adb: adb);
      await s.scanAdb();
      expect(s.devices, isEmpty);
      expect(s.lastError, isNotNull);
    });
  });

  group('connect', () {
    test('reads the partition table', () async {
      final s = await connectedState();
      expect(s.connected, isTrue);
      expect(s.partitions!.length, 2);
      expect(s.partitions![0].name, 'boot');
      expect(s.partitions![0].sizeBytes, 2048 * 512);
      expect(s.partitions![1].sizeBytes, 8388608 * 512);
    });

    test('falls back to the second by-name dir', () async {
      final adb = FakeAdb(shell: {
        'ls -l /dev/block/by-name': _ls,
        'cat /sys/class/block/sde17/size': '2048',
        'cat /sys/class/block/sde32/size': '8388608',
      });
      final s = await connectedState(adb: adb);
      expect(s.partitions!.length, 2);
    });

    test('no table anywhere means empty partitions + error', () async {
      final s = await connectedState(adb: FakeAdb());
      expect(s.partitions, isEmpty);
      expect(s.lastError, 'no partitions found');
    });

    test('an empty table can never become a plan', () async {
      final s = await connectedState(adb: FakeAdb());
      await s.setDestination('/tmp/dest');
      expect(s.partitions, isEmpty);
      expect(s.plan, isNull);
      await s.startBackup();
      expect(s.backupState, BackupState.idle);
    });

    test('reconnect resets a finished run', () async {
      final tmp = Directory.systemTemp.createTempSync('hbsup-state');
      addTearDown(() => tmp.deleteSync(recursive: true));
      final adb = FakeAdb(shell: {
        'ls -l $_byName': '''
total 0
lrwxrwxrwx 1 root root 1970 tiny -> /dev/block/sde1
''',
        'cat /sys/class/block/sde1/size': '2',
      }, exec: {
        ddOf('/dev/block/sde1'): Stream.value(Uint8List(1024)),
      });
      final s = fakeState(adb: adb);
      await s.connect('SER');
      await s.setDestination(tmp.path);
      await s.startBackup();
      expect(s.backupState, BackupState.done);
      await s.connect('SER');
      expect(s.backupState, BackupState.idle);
      expect(s.items, isEmpty);
      expect(s.manifestWritten, isFalse);
    });

    test('disconnect during a run keeps the session clean', () async {
      final tmp = Directory.systemTemp.createTempSync('hbsup-state');
      addTearDown(() => tmp.deleteSync(recursive: true));
      final gate = Completer<void>();
      Stream<Uint8List> stream() async* {
        yield Uint8List(512);
        await gate.future;
        yield Uint8List(512);
      }

      final adb = FakeAdb(shell: {
        'ls -l $_byName': '''
total 0
lrwxrwxrwx 1 root root 1970 tiny -> /dev/block/sde1
''',
        'cat /sys/class/block/sde1/size': '2',
      }, exec: {
        ddOf('/dev/block/sde1'): stream(),
      });
      final s = fakeState(adb: adb);
      await s.connect('SER');
      await s.setDestination(tmp.path);
      final run = s.startBackup();
      await Future<void>.delayed(Duration.zero);
      s.disconnect();
      gate.complete();
      await run;
      expect(s.backupState, BackupState.idle);
      expect(s.connected, isFalse);
      expect(s.lastError, isNull);
    });

    test('shell errors degrade to an empty table', () async {
      final s = fakeState(adb: _ThrowingShellAdb());
      await s.connect('SER');
      expect(s.partitions, isEmpty);
      expect(s.lastError, isNotNull);
    });

    test('unreadable size lands as zero bytes', () async {
      final adb = FakeAdb(shell: {
        'ls -l $_byName': _ls,
        'cat /sys/class/block/sde17/size': 'not-a-number',
        'cat /sys/class/block/sde32/size': '8',
      });
      final s = await connectedState(adb: adb);
      expect(s.partitions![0].sizeBytes, 0);
      expect(s.partitions![1].sizeBytes, 4096);
    });
  });

  group('pickDestination', () {
    test('stores the picked folder', () async {
      final s = fakeState(pickDir: () async => '/tmp/picked');
      await s.pickDestination();
      expect(s.destDir, '/tmp/picked');
      expect(s.destFreeBytes, 1 << 40);
    });

    test('a cancelled pick leaves the folder unset', () async {
      final s = fakeState(pickDir: () async => null);
      await s.pickDestination();
      expect(s.destDir, isNull);
    });

    test('a broken picker is swallowed', () async {
      final s = fakeState(
          pickDir: () async => throw StateError('no plugin'));
      await s.pickDestination();
      expect(s.destDir, isNull);
    });
  });

  test('setDestination probes free space', () async {
    final s = await connectedState();
    expect(s.destDir, isNull);
    await s.setDestination('/tmp/dest');
    expect(s.destDir, '/tmp/dest');
    expect(s.destFreeBytes, 1 << 40);
  });

  test('plan and space stay null until device + folder exist', () async {
    final s = fakeState();
    expect(s.plan, isNull);
    expect(s.space, isNull);
    await s.connect('SER');
    expect(s.plan, isNull);
    await s.setDestination('/tmp/dest');
    expect(s.plan, isNotNull);
    expect(s.space!.fits, isTrue);
  });

  test('startBackup without a plan is a no-op', () async {
    final s = await connectedState();
    await s.startBackup();
    expect(s.backupState, BackupState.idle);
  });

  test('full run ends done with a manifest', () async {
    final tmp = Directory.systemTemp.createTempSync('hbsup-state');
    addTearDown(() => tmp.deleteSync(recursive: true));
    // real files: no injected sink, sizes must match streamed bytes
    final adb = FakeAdb(shell: {
      'ls -l $_byName': '''
total 0
lrwxrwxrwx 1 root root 1970 tiny -> /dev/block/sde1
''',
      'cat /sys/class/block/sde1/size': '2',
    }, exec: {
      ddOf('/dev/block/sde1'): Stream.value(Uint8List(1024)),
    });
    final s = fakeState(adb: adb);
    await s.connect('SER');
    await s.setDestination(tmp.path);
    await s.startBackup();
    expect(s.backupState, BackupState.done);
    expect(s.manifestWritten, isTrue);
    expect(s.writtenTotal, 1024);
    expect(s.items.single.phase, ItemPhase.done);
    expect(File('${tmp.path}/tiny.img').lengthSync(), 1024);
    expect(s.logBuf.join('\n'), contains('backup finished'));
  });

  test('a failed dump lands as BackupState.failed', () async {
    final tmp = Directory.systemTemp.createTempSync('hbsup-state');
    addTearDown(() => tmp.deleteSync(recursive: true));
    final adb = FakeAdb(shell: {
      'ls -l $_byName': '''
total 0
lrwxrwxrwx 1 root root 1970 tiny -> /dev/block/sde1
''',
      'cat /sys/class/block/sde1/size': '2',
    }, exec: {
      ddOf('/dev/block/sde1'): Stream.error(StateError('dd died')),
    });
    final s = fakeState(adb: adb);
    await s.connect('SER');
    await s.setDestination(tmp.path);
    await s.startBackup();
    expect(s.backupState, BackupState.failed);
    expect(s.items.single.error, contains('dd died'));
    expect(s.logBuf.join('\n'), contains('backup failed'));
  });

  test('cancel mid-run reports cancelled', () async {
    final tmp = Directory.systemTemp.createTempSync('hbsup-state');
    addTearDown(() => tmp.deleteSync(recursive: true));
    final c = StreamController<Uint8List>();
    final adb = FakeAdb(shell: {
      'ls -l $_byName': '''
total 0
lrwxrwxrwx 1 root root 1970 tiny -> /dev/block/sde1
''',
      'cat /sys/class/block/sde1/size': '2',
    }, exec: {
      ddOf('/dev/block/sde1'): c.stream,
    });
    final s = fakeState(adb: adb);
    await s.connect('SER');
    await s.setDestination(tmp.path);
    final run = s.startBackup();
    await Future<void>.delayed(Duration.zero);
    c.add(Uint8List(512));
    await Future<void>.delayed(Duration.zero);
    s.cancel();
    c.add(Uint8List(512));
    await c.close();
    await run;
    expect(s.backupState, BackupState.cancelled);
    expect(s.logBuf.join('\n'), contains('backup cancelled'));
  });

  test('disconnect resets the whole session', () async {
    final s = await connectedState();
    await s.setDestination('/tmp/dest');
    s.disconnect();
    expect(s.connected, isFalse);
    expect(s.partitions, isNull);
    expect(s.destDir, isNull);
    expect(s.backupState, BackupState.idle);
  });

  test('the log buffer is capped at 400 lines', () async {
    final tmp = Directory.systemTemp.createTempSync('hbsup-log');
    addTearDown(() => tmp.deleteSync(recursive: true));
    final adb = FakeAdb(shell: {
      'ls -l $_byName': '''
total 0
lrwxrwxrwx 1 root root 1970 t -> /dev/block/sde1
''',
      'cat /sys/class/block/sde1/size': '0',
    }, exec: {
      ddOf('/dev/block/sde1'): const Stream.empty(),
    });
    final s = fakeState(adb: adb);
    await s.connect('S');
    await s.setDestination(tmp.path);
    for (var i = 0; i < 210; i++) {
      await s.startBackup();
    }
    expect(s.logBuf.length, lessThanOrEqualTo(400));
    expect(s.backupState, BackupState.done);
  });
}

class _ThrowingShellAdb extends FakeAdb {
  @override
  Future<String> shellText(String serial, String command) async =>
      throw const ProcessException('adb', []);
}
