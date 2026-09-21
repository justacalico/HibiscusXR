import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:go_router/go_router.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:pn2_website/main.dart';
import 'package:pn2_website/src/settings.dart';
import 'package:pn2_website/src/ui/hero_shot.dart';

Future<Widget> _app() async {
  SharedPreferences.setMockInitialValues({});
  final prefs = await SharedPreferences.getInstance();
  return SiteApp(settings: AppSettings(prefs)..load());
}

GoRouter _routerOf(WidgetTester tester) => tester
    .widget<MaterialApp>(find.byType(MaterialApp))
    .routerConfig! as GoRouter;

void main() {
  testWidgets('home page renders hero and nav', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    expect(find.text('Neosalsa'), findsWidgets);
    expect(find.text('Status'), findsWidgets);
    expect(find.text('Repos'), findsWidgets);
    expect(find.text('See the status'), findsOneWidget);
    expect(find.text('Read the docs'), findsOneWidget);
  });

  testWidgets('home hero draws the project mark', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    expect(find.byType(HeroShot), findsOneWidget);
    expect(find.text('All (21)'), findsWidgets);
    expect(find.text('Calendar'), findsWidgets);
    expect(
      find.byWidgetPredicate(
          (w) => w is CustomPaint && w.painter is LibraryMarkPainter),
      findsWidgets,
    );
  });

  testWidgets('navigates to download page', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    await tester.tap(find.text('Download').first);
    await tester.pumpAndSettle();

    expect(find.text('Flashing risk'), findsOneWidget);
    expect(find.text('Alpha software'), findsOneWidget);
  });

  testWidgets('download page unlocks steps after backup confirm',
      (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/download');
    await tester.pumpAndSettle();

    expect(find.text('Back up first'), findsOneWidget);
    expect(
        find.text('Confirm your backup above to reveal'), findsNWidgets(2));
    expect(find.byType(ImageFiltered), findsNWidgets(2));

    final confirm = find.text('I created a full backup of my headset');
    await Scrollable.ensureVisible(tester.element(confirm),
        alignment: 0.5);
    await tester.pumpAndSettle();
    await tester.tap(confirm);
    await tester.pumpAndSettle();

    expect(find.text('Confirm your backup above to reveal'), findsNothing);
    expect(find.byType(ImageFiltered), findsNothing);
  });

  testWidgets('repositories page lists all groups', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/repositories');
    await tester.pumpAndSettle();

    expect(find.text('Software'), findsOneWidget);
    expect(find.text('Port source'), findsOneWidget);
    expect(find.text('Dumps & staging'), findsOneWidget);
    expect(find.text('vrhome'), findsOneWidget);
  });

  testWidgets('status page shows both lists', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/status');
    await tester.pumpAndSettle();

    expect(find.text('Working'), findsOneWidget);
    expect(find.text('Not yet'), findsOneWidget);
  });

  testWidgets('unknown route shows the not-found page', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/nope');
    await tester.pumpAndSettle();

    expect(find.text('Page not found'), findsOneWidget);
    expect(find.text('Back to Neosalsa'), findsOneWidget);
  });

  testWidgets('zh locale renders translated chrome', (tester) async {
    SharedPreferences.setMockInitialValues({'locale': 'zh'});
    final prefs = await SharedPreferences.getInstance();
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(SiteApp(settings: AppSettings(prefs)..load()));
    await tester.pumpAndSettle();

    expect(find.text('Neosalsa'), findsWidgets);
    expect(find.text('现状'), findsWidgets);
    expect(find.text('查看现状'), findsOneWidget);
  });

  testWidgets('faq rows expand on tap', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/faq');
    await tester.pumpAndSettle();

    expect(find.text('FAQ'), findsWidgets);
    final answer = find.textContaining('VR display shows a real picture');
    expect(answer, findsNothing);

    await tester.tap(find.text('Does the port actually work?'));
    await tester.pumpAndSettle();
    expect(answer, findsOneWidget);
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
