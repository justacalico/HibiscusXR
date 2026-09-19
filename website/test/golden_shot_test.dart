import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:pn2_website/l10n/app_localizations.dart';
import 'package:pn2_website/src/ui/library_shot.dart';

void main() {
  testWidgets('library shot render', (tester) async {
    await tester.pumpWidget(
      MaterialApp(
        localizationsDelegates: AppLocalizations.localizationsDelegates,
        supportedLocales: AppLocalizations.supportedLocales,
        home: const Scaffold(
          backgroundColor: Colors.black,
          body: Center(
            child: SizedBox(width: 880, child: LibraryShot()),
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();
    await expectLater(
      find.byType(LibraryShot),
      matchesGoldenFile('goldens/library_shot.png'),
    );
  });
}
