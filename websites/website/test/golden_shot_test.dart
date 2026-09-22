import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';
import 'package:hibiscusxr_website/src/ui/hero_shot.dart';

void main() {
  testWidgets('hero shot render', (tester) async {
    await tester.pumpWidget(
      MaterialApp(
        localizationsDelegates: AppLocalizations.localizationsDelegates,
        supportedLocales: AppLocalizations.supportedLocales,
        home: const Scaffold(
          backgroundColor: Colors.black,
          body: Center(
            child: SizedBox(width: 880, child: HeroShot()),
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();
    await expectLater(
      find.byType(HeroShot),
      matchesGoldenFile('goldens/hero_shot.png'),
    );
  });
}
