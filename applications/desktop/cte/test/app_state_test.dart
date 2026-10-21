import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:common/common.dart';
import 'package:hibiscus_cte/src/app_state.dart';
import 'package:hibiscus_cte/src/debug_dump.dart';
import 'package:hibiscus_cte/src/models.dart';

import 'fakes.dart';

void main() {
  test('connectAdb attaches feeds and fills state', () async {
    final fake = FakeLink(description: 'adb:FAKE');
    final state = AppState(
      adb: AdbRunner(run: (_) async => ProcessResult(0, 0, '', '')),
      adbLinkFactory: (_) => fake,
      socketFactory: (_) async => null,
    );
    await state.connectAdb('FAKE');
    expect(state.connState, ConnState.connected);
    expect(state.connected, isTrue);
    expect(state.connectedLabel, contains('adb:FAKE'));

    fake.ctrlFeed.add(const [
      CtrlState(index: 0, connected: true, battery: 90),
      CtrlState(index: 1),
    ]);
    fake.poseFeed.add(const PoseSample(
        pose: Pose(x: 1, y: 2, z: 3), timestampNs: 5, hasPosition: true));
    fake.frameFeed.add(Uint8List.fromList([1, 2]));
    fake.logFeed.add('line one');
    await pumpEventQueue();

    expect(state.ctrls[0].connected, isTrue);
    expect(state.ctrls[0].battery, 90);
    expect(state.poseBuf.last.pose.x, 1);
    expect(state.poseBuf.last.mode, TrackingMode.dof6);
    expect(state.frame, [1, 2]);
    expect(state.logBuf.last, 'line one');
    await state.disconnect();
    expect(state.connected, isFalse);
    expect(fake.disposed, isTrue);
  });

  test('connect failure surfaces the error', () async {
    final state = AppState(
      adb: AdbRunner(run: (_) async => ProcessResult(0, 0, '', '')),
      adbLinkFactory: (_) => throw Exception('boom'),
      socketFactory: (_) async => null,
    );
    await state.connectAdb('X');
    expect(state.connState, ConnState.failed);
    expect(state.lastError, contains('boom'));
  });

  test('wireless falls back to adb connect when no cted', () async {
    final fake = FakeLink(description: 'adb:10.0.0.2:5555');
    final state = AppState(
      adb: AdbRunner(
        run: (args) async => args.first == 'connect'
            ? ProcessResult(0, 0, 'connected to 10.0.0.2:5555', '')
            : ProcessResult(0, 0, '', ''),
      ),
      socketFactory: (_) async => null,
      adbLinkFactory: (_) => fake,
    );
    await state.connectWireless('10.0.0.2');
    expect(state.connState, ConnState.connected);
    expect(state.linkKind, LinkKind.adbWireless);
  });

  test('wireless splits a host:port target before adb connect', () async {
    final fake = FakeLink(description: 'adb:10.0.0.2:5555');
    String? probedHost;
    String? serial;
    List<String>? connectArgs;
    final state = AppState(
      adb: AdbRunner(run: (args) async {
        if (args.first == 'connect') connectArgs = args;
        return args.first == 'connect'
            ? ProcessResult(0, 0, 'connected to 10.0.0.2:5555', '')
            : ProcessResult(0, 0, '', '');
      }),
      socketFactory: (h) async {
        probedHost = h;
        return null;
      },
      adbLinkFactory: (s) {
        serial = s;
        return fake;
      },
    );
    await state.connectWireless('10.0.0.2:5555');
    expect(probedHost, '10.0.0.2');
    expect(connectArgs, ['connect', '10.0.0.2:5555']);
    expect(serial, '10.0.0.2:5555');
    expect(state.linkKind, LinkKind.adbWireless);
  });

  test('wireless prefers cted when it answers', () async {
    final fake = FakeLink(description: 'cte://10.0.0.2');
    final state = AppState(
      adb: AdbRunner(run: (_) async => ProcessResult(0, 0, '', '')),
      socketFactory: (_) async => fake,
      adbLinkFactory: (_) => throw Exception('should not reach adb'),
    );
    await state.connectWireless('10.0.0.2');
    expect(state.linkKind, LinkKind.cteSocket);
  });

  test('wireless failure when adb connect refuses', () async {
    final state = AppState(
      adb: AdbRunner(
        run: (args) async =>
            ProcessResult(0, 0, 'failed to connect to 10.0.0.2:5555', ''),
      ),
      socketFactory: (_) async => null,
      adbLinkFactory: (_) => throw Exception('nope'),
    );
    await state.connectWireless('10.0.0.2');
    expect(state.connState, ConnState.failed);
    expect(state.lastError, contains('failed to connect'));
  });

  test('drops controller updates with out-of-range indexes', () async {
    final fake = FakeLink(description: 'adb:F');
    final state = fakeState(link: fake);
    await state.connectAdb('F');
    fake.ctrlFeed.add(const [
      CtrlState(index: -1, connected: true, battery: 99),
      CtrlState(index: 9, connected: true, battery: 99),
      CtrlState(index: 0, connected: true, battery: 42),
    ]);
    await pumpEventQueue();
    expect(state.ctrls[0].battery, 42);
    expect(state.ctrls[1].connected, isFalse);
  });

  test('a late fetchProps reply is dropped after disconnect', () async {
    final gate = Completer<Map<String, String>>();
    final fake = FakeLink(
        description: 'adb:F', propsLoader: () => gate.future);
    final state = fakeState(link: fake);
    await state.connectAdb('F');
    await state.disconnect();
    gate.complete(const {'ro.stale': '1'});
    await pumpEventQueue();
    expect(state.props, isNull);
  });

  test('a late fetchProps reply cannot clobber the new link', () async {
    final gate = Completer<Map<String, String>>();
    final stale = FakeLink(
        description: 'adb:A', propsLoader: () => gate.future);
    final fresh = FakeLink(
        description: 'adb:B', props: const {'ro.fresh': '1'});
    var next = stale;
    final state = AppState(
      adb: AdbRunner(run: (_) async => ProcessResult(0, 0, '', '')),
      adbLinkFactory: (_) => next,
      socketFactory: (_) async => null,
    );
    await state.connectAdb('A');
    await state.disconnect();
    next = fresh;
    await state.connectAdb('B');
    await pumpEventQueue();
    expect(state.props, fresh.props);
    gate.complete(const {'ro.stale': '1'});
    await pumpEventQueue();
    expect(state.props, fresh.props);
  });

  test('scanAdb lists devices', () async {
    final state = AppState(
      adb: AdbRunner(
        run: (_) async => ProcessResult(0, 0,
            'List of devices attached\nABC\tdevice model:X\n', ''),
      ),
    );
    await state.scanAdb();
    expect(state.adbDevices.single.serial, 'ABC');
    expect(state.connState, ConnState.idle);
  });

  test('install logs output lines', () async {
    final fake = FakeLink(
        description: 'adb:F',
        install: (_) => Stream.fromIterable(const ['Success']));
    final state = AppState(
      adb: AdbRunner(run: (_) async => ProcessResult(0, 0, '', '')),
      adbLinkFactory: (_) => fake,
      socketFactory: (_) async => null,
    );
    await state.connectAdb('F');
    await state.install('/tmp/x.apk');
    expect(state.installLog, contains('Success'));
    expect(state.installing, isFalse);
  });

  test('setMirror toggles the frame feed', () async {
    final fake = FakeLink(description: 'adb:F');
    final state = AppState(
      adb: AdbRunner(run: (_) async => ProcessResult(0, 0, '', '')),
      adbLinkFactory: (_) => fake,
      socketFactory: (_) async => null,
    );
    await state.connectAdb('F');
    await state.setMirror(false);
    expect(state.mirrorOn, isFalse);
    expect(state.frame, isNull);
    await state.setMirror(true);
    fake.frameFeed.add(Uint8List.fromList([7]));
    await pumpEventQueue();
    expect(state.frame, [7]);
  });

  test('dumpDebug collects sections and returns a zip', () async {
    final fake = FakeLink(
      description: 'adb:F',
      dump: () => Stream.fromIterable([
        DumpSection('meta.txt',
            Uint8List.fromList('model=Pico Neo 2\n'.codeUnits)),
        DumpSection('logcat.txt', Uint8List.fromList('I/x: hi\n'.codeUnits)),
      ]),
    );
    final state = AppState(
      adb: AdbRunner(run: (_) async => ProcessResult(0, 0, '', '')),
      adbLinkFactory: (_) => fake,
      socketFactory: (_) async => null,
    );
    await state.connectAdb('F');
    final zip = await state.dumpDebug();
    expect(state.dumping, isFalse);
    expect(state.dumpCount, 2);
    expect(state.dumpError, isNull);
    expect(state.dumpSuggestedName, startsWith('hibiscus-dump-SER9-'));
    expect(zip, isNotNull);
    expect(zip!.sublist(0, 4), [0x50, 0x4B, 0x03, 0x04]);
  });

  test('dumpDebug surfaces an empty bundle as an error', () async {
    final fake = FakeLink(description: 'adb:F');
    final state = AppState(
      adb: AdbRunner(run: (_) async => ProcessResult(0, 0, '', '')),
      adbLinkFactory: (_) => fake,
      socketFactory: (_) async => null,
    );
    await state.connectAdb('F');
    final zip = await state.dumpDebug();
    expect(zip, isNull);
    expect(state.dumpError, isNotNull);
    expect(state.dumping, isFalse);
  });
}
