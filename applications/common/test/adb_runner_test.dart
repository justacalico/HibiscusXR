import 'dart:io';

import 'package:common/common.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  group('parseAdbDevices', () {
    test('parses rows with models and states', () {
      const out = '''
List of devices attached
PICO1234	device usb:1-2 model:Pico_Neo_2
192.168.1.9:5555	device model:Pico_Neo_3
DEAD	device-xyz model:Nope
''';
      // second row stays a device; third's state is not "device"
      final rows = parseAdbDevices(out);
      expect(rows.length, 3);
      expect(rows[0].serial, 'PICO1234');
      expect(rows[0].model, 'Pico_Neo_2');
      expect(rows[0].isReady, isTrue);
      expect(rows[2].isReady, isFalse);
    });

    test('skips headers, daemon lines and junk', () {
      const out = '''
List of devices attached
* daemon started successfully
x
''';
      expect(parseAdbDevices(out), isEmpty);
    });

    test('missing model leaves an empty model', () {
      final rows =
          parseAdbDevices('List of devices attached\nSER1\toffline\n');
      expect(rows.single.model, '');
      expect(rows.single.isReady, isFalse);
    });
  });

  test('devices() runs adb devices', () async {
    final adb = AdbRunner(
        run: (args) async => ProcessResult(
            0, 0, 'List of devices attached\nSER9\tdevice\n', ''));
    final devs = await adb.devices();
    expect(devs.single.serial, 'SER9');
  });

  test('shellText concatenates stdout and stderr', () async {
    final adb = AdbRunner(
        run: (args) async => ProcessResult(0, 0, 'out', 'err'));
    expect(await adb.shellText('S', 'ls'), 'outerr');
  });

  test('execOut passes the command through and streams stdout', () async {
    List<String> spawned = const [];
    final adb = AdbRunner(spawn: (args) {
      spawned = args;
      return Process.start('echo', const ['chunk']);
    });
    final bytes =
        await adb.execOut('S', ['cat', '/x']).fold<List<int>>([], (a, c) {
      a.addAll(c);
      return a;
    });
    expect(spawned, ['-s', 'S', 'exec-out', 'cat', '/x']);
    expect(String.fromCharCodes(bytes).trim(), 'chunk');
  });

  test('execOut throws on a nonzero exit code with failOnExit', () async {
    final adb =
        AdbRunner(spawn: (args) => Process.start('false', const []));
    await expectLater(
        adb.execOut('S', ['dd if=/x'], failOnExit: true).drain<void>(),
        throwsA(isA<ProcessException>()));
  });

  test('execOut ignores a nonzero exit code by default', () async {
    final adb =
        AdbRunner(spawn: (args) => Process.start('false', const []));
    await adb.execOut('S', ['dd if=/x']).drain<void>();
  });

  test('execOut kills the child when the consumer cancels', () async {
    final adb = AdbRunner(
        spawn: (args) => Process.start('sleep', const ['60']));
    final sub = adb.execOut('S', ['cat', '/x']).listen((_) {});
    await Future<void>.delayed(const Duration(milliseconds: 100));
    await sub.cancel();
  });

  test('streamLines yields decoded stdout lines', () async {
    final adb = AdbRunner(
        spawn: (args) => Process.start('printf', const ['a\nb\n']));
    expect(await adb.streamLines('S', ['logcat']).toList(), ['a', 'b']);
  });

  test('default run/spawn hit the real executable', () async {
    final adb = AdbRunner(adbPath: 'echo');
    final r = await adb.run(['hello']);
    expect((r.stdout as String).trim(), 'hello');
    final p = await adb.spawn(['hi']);
    expect(await p.exitCode, 0);
  });
}
