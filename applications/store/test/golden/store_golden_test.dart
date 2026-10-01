import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_store/l10n/app_localizations.dart';
import 'package:pn2_store/src/store_controller.dart';
import 'package:pn2_store/src/ui/store_page.dart';
import 'package:pn2_store/src/ui/theme.dart';

import '../helpers.dart';

/// Canonical goldens render real Roboto and MaterialIcons, loaded from
/// the Flutter SDK font cache - the pngs double as README art. Hosts
/// without the font cache skip the assertions instead of failing.
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

  ThemeData themed() => StoreTheme.data().copyWith(
        textTheme:
            StoreTheme.data().textTheme.apply(fontFamily: 'Roboto'),
      );

  Future<StoreController> pump(
    WidgetTester tester, {
    Size size = const Size(1280, 800),
  }) async {
    tester.view.physicalSize = size;
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    final c = await readyController();
    await tester.pumpWidget(
      MaterialApp(
        localizationsDelegates: AppLocalizations.localizationsDelegates,
        supportedLocales: AppLocalizations.supportedLocales,
        theme: themed(),
        debugShowCheckedModeBanner: false,
        home: StorePage(controller: c),
      ),
    );
    for (var i = 0; i < 10; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    return c;
  }

  testWidgets('catalog golden', (tester) async {
    if (!fontsReady) return;
    await pump(tester);
    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/store_catalog.png'),
    );
  });

  testWidgets('detail pane golden', (tester) async {
    if (!fontsReady) return;
    final c = await pump(tester);
    c.store.select('com.example.alpha');
    for (var i = 0; i < 10; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/store_detail.png'),
    );
  });

  testWidgets('compact detail golden', (tester) async {
    if (!fontsReady) return;
    await pump(tester, size: const Size(480, 800));
    await tester.tap(find.text('Alpha Player'));
    await tester.pumpAndSettle();
    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/store_detail_compact.png'),
    );
  });

  testWidgets('error golden', (tester) async {
    if (!fontsReady) return;
    tester.view.physicalSize = const Size(1280, 800);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    final c = testController(throwError: Exception('offline'));
    await c.start();
    await tester.pumpWidget(
      MaterialApp(
        localizationsDelegates: AppLocalizations.localizationsDelegates,
        supportedLocales: AppLocalizations.supportedLocales,
        theme: themed(),
        debugShowCheckedModeBanner: false,
        home: StorePage(controller: c),
      ),
    );
    for (var i = 0; i < 10; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/store_error.png'),
    );
  });
}
