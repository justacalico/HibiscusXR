import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:hibiscus_cte/src/adb_runner.dart';
import 'package:hibiscus_cte/src/app_state.dart';
import 'package:hibiscus_cte/src/link.dart';
import 'package:hibiscus_cte/src/models.dart';

/// The one adb row the fake `adb devices -l` reports.
const fakeDevicesOut =
    'List of devices attached\nSER9\tdevice model:Pico_Neo_2\n';

/// Fixed device identity so every test sees the same headset.
const fakeInfo = DeviceInfo(
    model: 'Pico Neo 2',
    device: 'PICOA7B10',
    androidRelease: '10',
    sdkInt: 29,
    buildId: 'PN2A7B10-userdebug-10-QP1A.190711',
    hibiscusVersion: 'v2026.09.01-r1',
    trackingMode: TrackingMode.dof6,
    serial: 'SER9',
    batteryLevel: 88);

const fakeProps = {
  'ro.product.model': 'Pico Neo 2',
  'ro.build.display.id': 'PN2A7B10-userdebug-10-QP1A.190711',
  'persist.hibiscus.cted': '1',
  'sys.hibiscus.tracking.mode': '6dof',
};

/// HeadsetLink that replays canned data. Feeds are StreamController-backed
/// so a test can push frames/poses/log lines after connect lands.
class FakeLink extends HeadsetLink {
  FakeLink({
    this.description = 'adb:TEST',
    this.info = fakeInfo,
    this.props = fakeProps,
    this.install,
  });

  @override
  final String description;
  final DeviceInfo info;
  final Map<String, String> props;
  final Stream<String> Function(String path)? install;
  var disposed = false;

  // broadcast so setMirror's cancel/re-listen works; sync delivery keeps
  // state changes inside the pump that caused them - no stray event-loop
  // turns between add() and the assertion
  final ctrlFeed = StreamController<List<CtrlState>>.broadcast(sync: true);
  final poseFeed = StreamController<PoseSample>.broadcast(sync: true);
  final logFeed = StreamController<String>.broadcast(sync: true);
  final frameFeed = StreamController<Uint8List>.broadcast(sync: true);

  @override
  Future<DeviceInfo> fetchInfo() async => info;
  @override
  Future<Map<String, String>> fetchProps() async => props;
  @override
  Stream<String> installApk(String path, {Uint8List? bytes}) =>
      install?.call(path) ?? const Stream.empty();
  @override
  Stream<Uint8List> frames() => frameFeed.stream;
  @override
  Stream<String> logLines() => logFeed.stream;
  @override
  Stream<PoseSample> poses() => poseFeed.stream;
  @override
  Stream<List<CtrlState>> controllers() => ctrlFeed.stream;
  @override
  Future<void> dispose() async {
    disposed = true;
  }
}

AppState fakeState({FakeLink? link, AdbRunner? adb}) => AppState(
      adb: adb ??
          AdbRunner(
              run: (_) async => ProcessResult(0, 0, fakeDevicesOut, '')),
      adbLinkFactory: (_) => link ?? FakeLink(),
      socketFactory: (_) async => null,
    );
