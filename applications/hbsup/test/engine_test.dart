import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_hbsup/src/adb_runner.dart';
import 'package:hibiscus_hbsup/src/engine.dart';
import 'package:hibiscus_hbsup/src/models.dart';
import 'package:hibiscus_hbsup/src/plan.dart';

/// AdbRunner whose dd streams come straight out of a map.
class FakeAdb extends AdbRunner {
  FakeAdb({Map<String, Stream<Uint8List>>? exec})
      : _exec = exec ?? const {},
        super(
          run: (_) async => ProcessResult(0, 0, '', ''),
          spawn: (_) => Process.start('echo', const []),
        );

  final Map<String, Stream<Uint8List>> _exec;

  @override
  Stream<Uint8List> execOut(String serial, String command) =>
      _exec[command] ?? const Stream.empty();
}

String dd(Partition p) => 'dd if=${p.path} bs=4M 2>/dev/null';

const boot = Partition('boot', '/dev/block/sde17', 8);
const system = Partition('system', '/dev/block/sde32', 4);
const zero = Partition('zero', '/dev/block/sde1', 0);

BackupPlan planIn(String dir, List<Partition> parts) =>
    BackupPlan(destDir: dir, partitions: parts);

void main() {
  late Directory tmp;

  setUp(() => tmp = Directory.systemTemp.createTempSync('hbsup-test'));
  tearDown(() => tmp.deleteSync(recursive: true));

  test('dumps every partition and writes the manifest', () async {
    final adb = FakeAdb(exec: {
      dd(boot): Stream.value(Uint8List(8)),
      dd(system): Stream.value(Uint8List(4)),
    });
    final snaps = await BackupEngine(adb: adb)
        .run('S', planIn(tmp.path, [boot, system]))
        .toList();
    final last = snaps.last;
    expect(last.finished, isTrue);
    expect(last.cancelled, isFalse);
    expect(last.doneCount, 2);
    expect(last.anyFailed, isFalse);
    expect(last.writtenTotal, 12);
    expect(last.manifestWritten, isTrue);
    expect(File('${tmp.path}/boot.img').lengthSync(), 8);
    expect(File('${tmp.path}/manifest.txt').readAsStringSync(),
        contains('boot\t/dev/block/sde17'));
  });

  test('emits snapshots as chunks land', () async {
    final c = StreamController<Uint8List>();
    final adb = FakeAdb(exec: {dd(boot): c.stream});
    final snaps = <BackupSnapshot>[];
    final done = BackupEngine(adb: adb)
        .run('S', planIn(tmp.path, [boot]))
        .listen(snaps.add)
        .asFuture();
    await Future<void>.delayed(Duration.zero);
    c.add(Uint8List(4));
    await Future<void>.delayed(Duration.zero);
    c.add(Uint8List(4));
    await c.close();
    await done;
    // start + running + two chunks + done + final; items mutate in
    // place, so assert on the immutable current-index column
    expect(snaps.length, greaterThanOrEqualTo(5));
    expect(snaps[0].current, -1);
    expect(snaps[1].current, 0);
    expect(snaps.where((s) => s.current == 0).length,
        greaterThanOrEqualTo(2));
    expect(snaps.last.current, -1);
    expect(snaps.last.items[0].fraction, 1);
  });

  test('short dump is a size-mismatch failure and stops the run',
      () async {
    final adb = FakeAdb(exec: {
      dd(boot): Stream.value(Uint8List(3)),
      dd(system): Stream.value(Uint8List(4)),
    });
    final last = await BackupEngine(adb: adb)
        .run('S', planIn(tmp.path, [boot, system]))
        .last;
    expect(last.items[0].phase, ItemPhase.failed);
    expect(last.items[0].error, 'size mismatch');
    expect(last.items[1].phase, ItemPhase.pending);
    expect(last.anyFailed, isTrue);
    expect(last.manifestWritten, isFalse);
  });

  test('exec stream errors mark the item failed', () async {
    final adb = FakeAdb(exec: {
      dd(boot): Stream.error(StateError('adb died')),
    });
    final last = await BackupEngine(adb: adb)
        .run('S', planIn(tmp.path, [boot]))
        .last;
    expect(last.items[0].phase, ItemPhase.failed);
    expect(last.items[0].error, contains('adb died'));
  });

  test('unwritable destination fails the item', () async {
    final adb = FakeAdb(exec: {dd(boot): Stream.value(Uint8List(8))});
    final last = await BackupEngine(adb: adb)
        .run('S', planIn('${tmp.path}/nope/deeper', [boot]))
        .last;
    expect(last.items[0].phase, ItemPhase.failed);
    expect(last.items[0].error, isNotNull);
  });

  test('zero-size partition accepts an empty dump', () async {
    final adb = FakeAdb(exec: const {});
    final last = await BackupEngine(adb: adb)
        .run('S', planIn(tmp.path, [zero]))
        .last;
    expect(last.items[0].phase, ItemPhase.done);
    expect(last.items[0].fraction, 1);
  });

  test('cancel mid-partition stops after the current chunk', () async {
    final token = CancelToken();
    final c = StreamController<Uint8List>();
    final adb = FakeAdb(exec: {dd(boot): c.stream, dd(system): Stream.value(Uint8List(4))});
    final done = BackupEngine(adb: adb)
        .run('S', planIn(tmp.path, [boot, system]), token: token)
        .last;
    await Future<void>.delayed(Duration.zero);
    c.add(Uint8List(4));
    await Future<void>.delayed(Duration.zero);
    token.cancelled = true;
    c.add(Uint8List(4));
    await c.close();
    final last = await done;
    expect(last.cancelled, isTrue);
    expect(last.finished, isFalse);
    expect(last.items[0].phase, ItemPhase.failed);
    expect(last.items[0].error, 'cancelled');
    expect(last.items[0].written, 4);
    expect(last.items[1].phase, ItemPhase.pending);
    expect(last.manifestWritten, isFalse);
  });

  test('manifest write failure still finishes the run', () async {
    final adb = FakeAdb(exec: {dd(boot): Stream.value(Uint8List(8))});
    var calls = 0;
    final last = await BackupEngine(
      adb: adb,
      openSink: (p) {
        calls++;
        if (p.endsWith('manifest.txt')) {
          throw const FileSystemException('rofs');
        }
        return File(p).openWrite();
      },
    ).run('S', planIn(tmp.path, [boot])).last;
    expect(calls, 2);
    expect(last.finished, isTrue);
    expect(last.manifestWritten, isFalse);
  });
}
