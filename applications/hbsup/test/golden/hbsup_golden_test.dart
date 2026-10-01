import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_hbsup/src/app_state.dart';
import 'package:hibiscus_hbsup/src/theme.dart';
import 'package:hibiscus_hbsup/ui/app.dart';

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
    if (!File('$fontDir/Roboto-Regular.ttf').existsSync()) return;
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
    // the log card asks for `monospace` - alias it to Roboto so the
    // golden stays legible instead of painting tofu boxes
    await load('monospace', 'Roboto-Regular.ttf');
    fontsReady = true;
  });

  ThemeData themed() => HbsupTheme.dark().copyWith(
        textTheme:
            HbsupTheme.dark().textTheme.apply(fontFamily: 'Roboto'),
      );

  Future<AppState> pumpApp(
    WidgetTester tester,
    AppState s, {
    Size size = const Size(1280, 800),
  }) async {
    tester.view.physicalSize = size;
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(HbsupApp(state: s, theme: themed()));
    // pumpAndSettle never settles while the icon decodes async; give the
    // real event loop a beat for the asset read, then pump a fixed
    // number of frames so the golden stays deterministic.
    await tester.runAsync(
        () => Future<void>.delayed(const Duration(milliseconds: 200)));
    for (var i = 0; i < 30; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    return s;
  }

  Future<void> frames(WidgetTester tester, [int count = 10]) async {
    for (var i = 0; i < count; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
  }

  testWidgets('connect page lists a headset', (tester) async {
    if (!fontsReady) return;
    await pumpApp(tester, fakeState());

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/hbsup_connect.png'),
    );
  });

  testWidgets('windows shows the unsupported banner', (tester) async {
    if (!fontsReady) return;
    await pumpApp(tester, fakeState(hostOs: 'windows'));

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/hbsup_connect_unsupported.png'),
    );
  });

  testWidgets('backup page ready to run', (tester) async {
    if (!fontsReady) return;
    final s = await pumpApp(
        tester, fakeState(),
        size: const Size(1280, 1000));
    await s.connect('SER9');
    await s.setDestination('/tmp/hbsup-golden');
    await frames(tester);

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/hbsup_backup_ready.png'),
    );
  });

  testWidgets('backup page blocks when space is short', (tester) async {
    if (!fontsReady) return;
    final s = AppState(
      adb: fakeAdb(),
      hostOs: 'linux',
      freeSpace: (_) async => 8,
    );
    await pumpApp(tester, s, size: const Size(1280, 1000));
    await s.connect('SER9');
    await s.setDestination('/tmp/hbsup-golden');
    await frames(tester);

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/hbsup_backup_blocked.png'),
    );
  });

  testWidgets('backup page after a finished run', (tester) async {
    if (!fontsReady) return;
    // only the boot partition, so the fake dump size matches
    final adb = FakeAdb(
      run: (args) async => ProcessResult(0, 0,
          'List of devices attached\nSER9\tdevice model:Pico_Neo_2\n',
          ''),
      shell: {
        'ls -l $byNameDir': '''
total 0
lrwxrwxrwx 1 root root 1970 boot -> /dev/block/sde17
''',
        'cat /sys/class/block/sde17/size': '2048',
      },
      exec: {
        'dd if=/dev/block/sde17 bs=4M 2>/dev/null':
            Stream.value(Uint8List(2048 * 512)),
      },
    );
    final s = await pumpApp(tester, fakeState(adb: adb),
        size: const Size(1280, 1000));
    await s.connect('SER9');
    await s.setDestination('/tmp/hbsup-golden');
    await s.startBackup();
    await frames(tester);
    expect(s.backupState, BackupState.done);

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/hbsup_backup_done.png'),
    );
  });
}
