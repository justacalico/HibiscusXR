import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/src/adb_runner.dart';
import 'package:hibiscus_cte/src/app_state.dart';
import 'package:hibiscus_cte/src/link.dart';
import 'package:hibiscus_cte/src/models.dart';

class FakeLink extends HeadsetLink {
  FakeLink(this.desc);

  final String desc;
  final ctrlCtl = StreamController<List<CtrlState>>.broadcast();
  final poseCtl = StreamController<PoseSample>.broadcast();
  final frameCtl = StreamController<Uint8List>.broadcast();
  final logCtl = StreamController<String>.broadcast();
  bool disposed = false;

  @override
  String get description => desc;
  @override
  Future<DeviceInfo> fetchInfo() async =>
      const DeviceInfo(model: 'Pico Neo 2', device: 'PICOA7B10');
  @override
  Future<Map<String, String>> fetchProps() async => {'ro.test': '1'};
  @override
  Stream<String> installApk(String path, {Uint8List? bytes}) =>
      Stream.fromIterable(['Success']);
  @override
  Stream<Uint8List> frames() => frameCtl.stream;
  @override
  Stream<String> logLines() => logCtl.stream;
  @override
  Stream<PoseSample> poses() => poseCtl.stream;
  @override
  Stream<List<CtrlState>> controllers() => ctrlCtl.stream;
  @override
  Future<void> dispose() async {
    disposed = true;
  }
}

void main() {
  test('connectAdb attaches feeds and fills state', () async {
    final fake = FakeLink('adb:FAKE');
    final state = AppState(
      adb: AdbRunner(run: (_) async => ProcessResult(0, 0, '', '')),
      adbLinkFactory: (_) => fake,
      socketFactory: (_) async => null,
    );
    await state.connectAdb('FAKE');
    expect(state.connState, ConnState.connected);
    expect(state.connected, isTrue);
    expect(state.connectedLabel, contains('adb:FAKE'));

    fake.ctrlCtl.add(const [
      CtrlState(index: 0, connected: true, battery: 90),
      CtrlState(index: 1),
    ]);
    fake.poseCtl.add(const PoseSample(
        pose: Pose(x: 1, y: 2, z: 3), timestampNs: 5, hasPosition: true));
    fake.frameCtl.add(Uint8List.fromList([1, 2]));
    fake.logCtl.add('line one');
    await pumpEventQueue();

    expect(state.ctrls[0].connected, isTrue);
    expect(state.ctrls[0].battery, 90);
    expect(state.poseBuf.last.pose.x, 1);
    expect(state.poseBuf.last.mode, TrackingMode.dof6);
    expect(state.frame, [1, 2]);
    expect(state.logBuf.last, 'line one');
    await state.disconnect();
    expect(state.connected, isFalse);
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
    final fake = FakeLink('adb:10.0.0.2:5555');
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

  test('wireless prefers cted when it answers', () async {
    final fake = FakeLink('cte://10.0.0.2');
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
    final fake = FakeLink('adb:F');
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
    final fake = FakeLink('adb:F');
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
    fake.frameCtl.add(Uint8List.fromList([7]));
    await pumpEventQueue();
    expect(state.frame, [7]);
  });
}
