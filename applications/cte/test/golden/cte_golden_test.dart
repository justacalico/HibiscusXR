import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/src/app_state.dart';
import 'package:hibiscus_cte/src/models.dart';
import 'package:hibiscus_cte/src/theme.dart';
import 'package:hibiscus_cte/ui/app.dart';

import '../fakes.dart';

/// Canonical goldens render real Roboto and MaterialIcons, loaded from
/// the Flutter SDK font cache - without them every glyph paints as a
/// box. Hosts without the font cache skip the assertions instead of
/// failing.
void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  final fontDir = '${Platform.environment['FLUTTER_ROOT'] ?? ''}'
      '/bin/cache/artifacts/material_fonts';
  var fontsReady = false;

  setUpAll(() async {
    const needed = [
      'Roboto-Regular.ttf',
      'Roboto-Medium.ttf',
      'Roboto-Bold.ttf',
      'MaterialIcons-Regular.otf',
    ];
    if (needed.any((f) => !File('$fontDir/$f').existsSync())) {
      // ignore: avoid_print
      print('golden tests skipped - no font cache at $fontDir');
      return;
    }
    Future<void> load(String family, String file) async {
      final bytes = await File('$fontDir/$file').readAsBytes();
      await (FontLoader(family)
            ..addFont(Future.value(ByteData.sublistView(bytes))))
          .load();
    }

    await load('Roboto', 'Roboto-Regular.ttf');
    await load('Roboto', 'Roboto-Medium.ttf');
    await load('Roboto', 'Roboto-Bold.ttf');
    await load('MaterialIcons', 'MaterialIcons-Regular.otf');
    // install log, prop table and pose log ask for `monospace` - alias it
    // to Roboto so the goldens stay legible instead of painting tofu
    await load('monospace', 'Roboto-Regular.ttf');
    fontsReady = true;
  });

  ThemeData themed() => CteTheme.dark().copyWith(
        textTheme: CteTheme.dark().textTheme.apply(fontFamily: 'Roboto'),
      );

  Future<void> pumpApp(
    WidgetTester tester,
    AppState s, {
    Size size = const Size(1280, 800),
  }) async {
    tester.view.physicalSize = size;
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    addTearDown(s.dispose);
    await tester.pumpWidget(CteApp(state: s, theme: themed()));
    // give the real event loop a beat for async work (image decode,
    // scanAdb's post-frame callback), then pump fixed frames so the
    // golden stays deterministic
    await tester.runAsync(
        () => Future<void>.delayed(const Duration(milliseconds: 200)));
    for (var i = 0; i < 30; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
  }

  Future<void> frames(WidgetTester tester, [int count = 10]) async {
    for (var i = 0; i < count; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
  }

  testWidgets('connect page lists the scanned headset', (tester) async {
    if (!fontsReady) return;
    await pumpApp(tester, fakeState());

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/cte_connect.png'),
    );
  });

  testWidgets('overview page shows headset, controllers and link',
      (tester) async {
    if (!fontsReady) return;
    final link = FakeLink();
    final s = fakeState(link: link);
    await pumpApp(tester, s);
    await s.connectAdb('SER9');
    link.ctrlFeed.add(const [
      CtrlState(index: 0, connected: true, battery: 74, tracked: true),
      CtrlState(index: 1, connected: true, battery: 81, tracked: true),
    ]);
    await frames(tester);

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/cte_overview.png'),
    );
  });

  testWidgets('display page renders a mirrored frame', (tester) async {
    if (!fontsReady) return;
    final link = FakeLink();
    final s = fakeState(link: link);
    await pumpApp(tester, s);
    await s.connectAdb('SER9');
    await frames(tester);
    await tester.tap(find.text('Display'));
    await frames(tester);
    // file reads and the frame decode need the real event loop
    final icon = await tester
        .runAsync(() => File('assets/icon.png').readAsBytes())
        .then((b) => b!);
    link.frameFeed.add(icon);
    // build the Image widget so the codec starts, then let it finish
    await tester.pump();
    await tester.runAsync(
        () => Future<void>.delayed(const Duration(milliseconds: 200)));
    await frames(tester);

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/cte_display.png'),
    );
  });

  testWidgets('install page after a finished push', (tester) async {
    if (!fontsReady) return;
    final link = FakeLink(
      install: (_) => Stream.fromIterable(const [
        'Performing Streamed Install',
        'Success',
      ]),
    );
    final s = fakeState(link: link);
    await pumpApp(tester, s);
    await s.connectAdb('SER9');
    await frames(tester);
    await tester.tap(find.text('Install'));
    await frames(tester);
    await s.install('/tmp/hbsup.apk');
    expect(s.installing, isFalse);
    expect(s.installLog, contains('Success'));
    await frames(tester);

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/cte_install.png'),
    );
  });

  testWidgets('tracking page plots the pose trail', (tester) async {
    if (!fontsReady) return;
    final link = FakeLink();
    final s = fakeState(link: link);
    await pumpApp(tester, s);
    await s.connectAdb('SER9');
    link.ctrlFeed.add(const [
      CtrlState(
          index: 0,
          connected: true,
          battery: 74,
          tracked: true,
          pose: Pose(x: -0.21, y: 1.12, z: -0.34)),
      CtrlState(
          index: 1,
          connected: true,
          battery: 81,
          tracked: true,
          pose: Pose(x: 0.24, y: 1.09, z: -0.31)),
    ]);
    // all five samples land synchronously inside the 2s rate window, so
    // the rate readout stays a deterministic "0.0 samples/s"
    const trail = [
      (0.00, 1.60, -0.05),
      (0.04, 1.61, -0.08),
      (0.09, 1.60, -0.10),
      (0.15, 1.59, -0.09),
      (0.18, 1.61, -0.04),
    ];
    for (var i = 0; i < trail.length; i++) {
      final p = trail[i];
      link.poseFeed.add(PoseSample(
          pose: Pose(x: p.$1, y: p.$2, z: p.$3),
          timestampNs: 1746400000000 + i * 11111111,
          trackingState: 3,
          hasPosition: true));
    }
    await frames(tester);
    // the overview card has a 'Tracking' row too - hit the rail label
    await tester.tap(find.descendant(
        of: find.byType(NavigationRail), matching: find.text('Tracking')));
    await frames(tester);

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/cte_tracking.png'),
    );
  });

  testWidgets('debug page lists props and the logcat tail',
      (tester) async {
    if (!fontsReady) return;
    final link = FakeLink();
    final s = fakeState(link: link);
    await pumpApp(tester, s);
    await s.connectAdb('SER9');
    for (final line in const [
      'I/vr_runtime: tracking session started mode=6dof',
      'I/cted: client connected addr=10.0.0.5',
      'W/PowerManager: screen on battery=88',
    ]) {
      link.logFeed.add(line);
    }
    await frames(tester);
    await tester.tap(find.text('Debug'));
    await frames(tester);

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/cte_debug.png'),
    );
  });
}
