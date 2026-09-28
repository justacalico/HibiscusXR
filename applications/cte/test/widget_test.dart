import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/src/adb_runner.dart';
import 'package:hibiscus_cte/src/app_state.dart';
import 'package:hibiscus_cte/src/link.dart';
import 'package:hibiscus_cte/src/models.dart';
import 'package:hibiscus_cte/ui/app.dart';

class FakeLink extends HeadsetLink {
  @override
  String get description => 'adb:TEST';
  @override
  Future<DeviceInfo> fetchInfo() async => const DeviceInfo(
      model: 'Pico Neo 2',
      device: 'PICOA7B10',
      androidRelease: '10',
      sdkInt: 29,
      hibiscusVersion: 'v2026.09.01-r1',
      trackingMode: TrackingMode.dof6,
      batteryLevel: 88);
  @override
  Future<Map<String, String>> fetchProps() async =>
      {'ro.product.model': 'Pico Neo 2'};
  @override
  Stream<String> installApk(String path, {Uint8List? bytes}) =>
      const Stream.empty();
  @override
  Stream<Uint8List> frames() => const Stream.empty();
  @override
  Stream<String> logLines() => const Stream.empty();
  @override
  Stream<PoseSample> poses() => const Stream.empty();
  @override
  Stream<List<CtrlState>> controllers() => const Stream.empty();
  @override
  Future<void> dispose() async {}
}

AppState fakeState() => AppState(
      adb: AdbRunner(
          run: (_) async =>
              ProcessResult(0, 0, 'List of devices attached\n', '')),
      adbLinkFactory: (_) => FakeLink(),
      socketFactory: (_) async => null,
    );

void main() {
  testWidgets('connect page renders brand and sections', (tester) async {
    await tester.pumpWidget(CteApp(state: fakeState()));
    await tester.pumpAndSettle();
    expect(find.text('HCTE'), findsOneWidget);
    expect(find.text('Connect to a headset'), findsOneWidget);
    expect(find.text('Wireless'), findsOneWidget);
  });

  testWidgets('connected state shows the nav shell and overview',
      (tester) async {
    final state = fakeState();
    await tester.pumpWidget(CteApp(state: state));
    await tester.pumpAndSettle();
    await state.connectAdb('TEST');
    await tester.pumpAndSettle();
    expect(find.byType(NavigationRail), findsOneWidget);
    expect(find.text('Headset'), findsOneWidget);
    expect(find.text('Pico Neo 2'), findsWidgets);
    expect(find.text('Controllers'), findsOneWidget);
    // hop to the debug page through the rail
    await tester.tap(find.text('Debug'));
    await tester.pumpAndSettle();
    expect(find.text('Properties'), findsOneWidget);
  });
}
