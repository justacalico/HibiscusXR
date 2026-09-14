import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:pn2_website/main.dart';
import 'package:pn2_website/src/settings.dart';

Future<Widget> _app() async {
  SharedPreferences.setMockInitialValues({});
  final prefs = await SharedPreferences.getInstance();
  return SiteApp(settings: AppSettings(prefs)..load());
}

void main() {
  testWidgets('home page renders hero and nav', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    expect(find.text('Library'), findsWidgets);
    expect(find.text('Features'), findsWidgets);
    expect(find.text('Screenshots'), findsWidgets);
    expect(find.text('See features'), findsOneWidget);
    expect(find.text('Read the docs'), findsOneWidget);
  });

  testWidgets('navigates to download page', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    await tester.tap(find.text('Download').first);
    await tester.pumpAndSettle();

    expect(find.text('Nothing to install yet.'), findsOneWidget);
    expect(find.text('Not shipping yet'), findsOneWidget);
  });

  testWidgets('dark theme applies dark scaffold', (tester) async {
    SharedPreferences.setMockInitialValues({'theme_mode': 'dark'});
    final prefs = await SharedPreferences.getInstance();
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(SiteApp(settings: AppSettings(prefs)..load()));
    await tester.pumpAndSettle();

    final app = tester.widget<MaterialApp>(find.byType(MaterialApp));
    expect(app.themeMode, ThemeMode.dark);
  });
}
