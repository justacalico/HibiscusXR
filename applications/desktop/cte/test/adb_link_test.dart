import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/src/adb_runner.dart';
import 'package:hibiscus_cte/src/debug_dump.dart';
import 'package:hibiscus_cte/src/transport/adb_link.dart';

import 'ctrl_share_test.dart' show makeShareMem;
import 'tar_fixture.dart';

class FakeProcess implements Process {
  FakeProcess(this._stdout, [this._exitCode = 0]);
  final List<int> _stdout;
  final int _exitCode;

  @override
  Future<int> get exitCode async => _exitCode;
  @override
  int get pid => 0;
  @override
  IOSink get stdin => IOSink(StreamController<List<int>>().sink);
  @override
  Stream<List<int>> get stdout => Stream.value(_stdout);
  @override
  Stream<List<int>> get stderr => const Stream.empty();
  @override
  bool kill([ProcessSignal signal = ProcessSignal.sigterm]) => true;
}

AdbRunner fakeAdb({
  String getprop = '',
  String battery = 'level: 80',
  List<int>? shareMem,
  List<int>? screencap,
  int screencapRepeats = 1,
  bool screencapFails = false,
  List<String> logcatLines = const [],
  List<String> installOut = const ['Success'],
  List<int>? dumpBlob,
}) {
  ProcessResult res(String out) =>
      ProcessResult(0, 0, out, '');
  return AdbRunner(
    run: (args) async {
      final cmd = args.join(' ');
      if (cmd.contains('getprop')) return res(getprop);
      if (cmd.contains('dumpsys battery')) return res(battery);
      if (cmd.contains('setprop')) return res('');
      if (cmd.startsWith('devices')) return res('List of devices attached\nABC\tdevice\n');
      if (cmd.startsWith('connect')) return res('connected to host:5555');
      return res('');
    },
    spawn: (args) async {
      final cmd = args.join(' ');
      // the on-device collector: present = tar.gz blob, absent = empty
      if (cmd.contains('exec-out') && cmd.contains('cte-dump')) {
        return FakeProcess(dumpBlob ?? const []);
      }
      if (cmd.contains('exec-out') && cmd.contains('screencap')) {
        if (screencapFails) {
          return FakeProcess(utf8.encode('+ERR screencap failed\n'));
        }
        // the device-side loop speaks the FRAME wire format
        final png = screencap ?? [1, 2, 3];
        return FakeProcess([
          for (var i = 0; i < screencapRepeats; i++) ...[
            ...utf8.encode('FRAME ${png.length}\n'),
            ...png,
          ],
        ]);
      }
      if (cmd.contains('exec-out') && cmd.contains('CtrlShareMem')) {
        return FakeProcess(shareMem ?? List.filled(1024, 0));
      }
      if (cmd.contains('install')) {
        return FakeProcess(utf8.encode(installOut.join('\n')));
      }
      // legacy dump sections land here: any exec-out not claimed above
      if (cmd.contains('exec-out')) {
        return FakeProcess(utf8.encode('sec:$cmd'));
      }
      if (cmd.contains('logcat')) {
        return FakeProcess(utf8.encode(logcatLines.join('\n')));
      }
      return FakeProcess(const []);
    },
  );
}

void main() {
  const getprop = '''
[ro.product.model]: [Pico Neo 2]
[ro.product.device]: [PICOA7B10]
[ro.build.version.release]: [10]
[ro.build.version.sdk]: [29]
[ro.hibiscus.version]: [v2026.09.01-r1]
[persist.pn2.dof]: [6dof]
''';

  test('parseAdbDevices', () {
    final rows = parseAdbDevices(
        'List of devices attached\nABC123\tdevice product:x model:Pico_Neo_2 device:A7B10\n'
        'offline\tdevice\n');
    expect(rows.length, 2);
    expect(rows.first.serial, 'ABC123');
    expect(rows.first.model, 'Pico_Neo_2');
    expect(rows.first.isReady, isTrue);
  });

  test('fetchInfo reads props and battery', () async {
    final link = AdbLink(fakeAdb(getprop: getprop), 'ABC');
    final info = await link.fetchInfo();
    expect(info.headsetName, 'Pico Neo 2');
    expect(info.batteryLevel, 80);
    expect(info.trackingMode.name, 'dof6');
  });

  test('frames yields screencap bytes', () async {
    final link = AdbLink(fakeAdb(screencap: [9, 9, 9]), 'ABC');
    final frame =
        await link.frames().first.timeout(const Duration(seconds: 2));
    expect(frame, [9, 9, 9]);
  });

  test('frames streams back-to-back blobs off one exec-out', () async {
    final link =
        AdbLink(fakeAdb(screencap: [7, 8], screencapRepeats: 3), 'ABC');
    final frames =
        await link.frames().take(3).toList().timeout(const Duration(seconds: 2));
    expect(frames.length, 3);
    expect(frames.last, [7, 8]);
  });

  test('frames surfaces a device-side screencap error', () async {
    final link = AdbLink(fakeAdb(screencapFails: true), 'ABC');
    await expectLater(
        link.frames().first.timeout(const Duration(seconds: 2)),
        throwsA(anything));
  });

  test('controllers decodes a CtrlShareMem snapshot', () async {
    final link =
        AdbLink(fakeAdb(shareMem: makeShareMem(batteryL: 66)), 'ABC');
    final ctrls = await link
        .controllers(interval: const Duration(milliseconds: 1))
        .first
        .timeout(const Duration(seconds: 2));
    expect(ctrls[0].battery, 66);
    // baseline hash only - not yet proven live
    expect(ctrls[0].connected, isFalse);
  });

  test('poses parses pn2pose dump lines from logcat', () async {
    final link = AdbLink(
      fakeAdb(logcatLines: [
        'qvr  q=(0 0 0 1) p=(0 0 0) st=3 q=0.9',
        '     rel q=(0 0 0.1 0.99) p=(0.1 0.2 0.3) w=(0 0 0) ts=12',
      ]),
      'ABC',
    );
    final s = await link.poses().first.timeout(const Duration(seconds: 2));
    expect(s.pose.x, closeTo(0.1, 1e-6));
    expect(s.mode.name, 'dof6');
  });

  test('installApk streams adb output', () async {
    final link = AdbLink(fakeAdb(installOut: ['Performing Streamed Install', 'Success']), 'ABC');
    final out = await link.installApk('/tmp/x.apk').toList();
    expect(out, contains('Success'));
  });

  test('debugDump decodes the cte-dump tar.gz', () async {
    final blob = tarArchive({
      'meta.txt': utf8.encode('model=Pico Neo 2\n'),
      'logcat.txt': utf8.encode('I/x: hi\n'),
    });
    final link = AdbLink(fakeAdb(dumpBlob: blob), 'ABC');
    final sections = await link.debugDump().toList();
    expect(sections.map((s) => s.name), ['meta.txt', 'logcat.txt']);
    expect(utf8.decode(sections.first.bytes), contains('Pico Neo 2'));
  });

  test('debugDump runs per-section commands when no collector exists',
      () async {
    final link = AdbLink(fakeAdb(), 'ABC'); // no dumpBlob = old image
    final sections = await link.debugDump().toList();
    expect(sections.map((s) => s.name).toList(),
        legacyDumpSections.map((s) => s.$1).toList());
    expect(
        utf8.decode(
            sections.firstWhere((s) => s.name == 'getprop.txt').bytes),
        contains('getprop'));
  });
}
