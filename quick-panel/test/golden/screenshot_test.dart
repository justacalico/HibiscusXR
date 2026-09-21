import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/src/models.dart';
import 'package:pn2_quicksettings/src/persistence.dart';
import 'package:pn2_quicksettings/src/platform/fake_settings_source.dart';
import 'package:pn2_quicksettings/src/settings_controller.dart';
import 'package:pn2_quicksettings/src/settings_store.dart';
import 'package:pn2_quicksettings/src/ui/quick_settings_page.dart';
import 'package:pn2_quicksettings/src/ui/theme.dart';
import 'package:pn2_quicksettings/l10n/app_localizations.dart';

/// Screenshot generator: same scenes as the canonical goldens but with
/// the real Roboto and MaterialIcons fonts loaded from the Flutter SDK,
/// so the output doubles as README art. On machines without the SDK font
/// cache the test early-outs instead of failing.
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
    fontsReady = true;
  });

  Future<void> pump(WidgetTester tester, SettingsSnapshot initial) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    final c = SettingsController(
      source: FakeSettingsSource(initial: initial),
      persistence: MemoryPersistence(),
      store: SettingsStore(clock: () => DateTime(2023, 8, 16, 15, 52)),
    );
    addTearDown(c.dispose);
    await c.start();
    await tester.pumpWidget(
      MaterialApp(
        localizationsDelegates: AppLocalizations.localizationsDelegates,
        supportedLocales: AppLocalizations.supportedLocales,
        theme: PanelTheme.data().copyWith(
          textTheme: PanelTheme.data().textTheme.apply(fontFamily: 'Roboto'),
        ),
        debugShowCheckedModeBanner: false,
        home: QuickSettingsPage(controller: c),
      ),
    );
    for (var i = 0; i < 20; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
  }

  testWidgets('panel screenshot', (tester) async {
    if (!fontsReady) return;
    await pump(
      tester,
      const SettingsSnapshot(
        batteryLevel: 96,
        wifiSsid: 'neosalsa-5g',
        volume: 0.62,
        brightness: 0.38,
        toggles: {
          ToggleId.wifi: true,
          ToggleId.boundary: true,
          ToggleId.microphone: true,
        },
      ),
    );
    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/screenshots/panel.png'),
    );
  });

  testWidgets('radios off screenshot', (tester) async {
    if (!fontsReady) return;
    await pump(
      tester,
      const SettingsSnapshot(
        batteryLevel: 15,
        volume: 0.1,
        brightness: 0.9,
        toggles: {ToggleId.doNotDisturb: true},
      ),
    );
    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/screenshots/panel_off.png'),
    );
  });
}
