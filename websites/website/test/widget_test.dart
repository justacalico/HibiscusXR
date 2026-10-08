import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:go_router/go_router.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:hibiscusxr_website/main.dart';
import 'package:hibiscusxr_website/src/settings.dart';
import 'package:hibiscusxr_website/src/ui/hero_shot.dart';
import 'package:hibiscusxr_website/src/ui/widgets.dart';

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

    expect(find.text('HibiscusXR'), findsWidgets);
    expect(find.text('Repos'), findsWidgets);
    expect(find.text('Guide'), findsWidgets);
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

  testWidgets('cte page renders features and links', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/cte');
    await tester.pumpAndSettle();

    expect(find.text('HCTE'), findsWidgets);
    expect(find.text('What it is'), findsOneWidget);
    expect(find.text('Screen mirror'), findsOneWidget);
    expect(find.text('Live 6DoF/3DoF tracking'), findsOneWidget);
    expect(find.text('How it connects'), findsOneWidget);
    expect(find.text('CTE releases'), findsOneWidget);
    expect(find.text('Source'), findsWidgets);
    expect(find.text('What it looks like'), findsOneWidget);
    expect(find.byType(ShotCard), findsNWidgets(6));
  });

  testWidgets('home page shows the supported devices grid', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    expect(find.text('Oculus Quest 1'), findsOneWidget);
    expect(find.text('Pico Neo 3'), findsOneWidget);
    expect(find.text('Hibiscus VMD'), findsOneWidget);
    expect(find.text('Supported, in development'), findsOneWidget);
    expect(find.text('Planned'), findsNWidgets(2));
    expect(find.text('Virtual device'), findsOneWidget);
  });

  testWidgets('nav guide button opens the flashing docs', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    await tester.tap(find.text('Guide').first);
    await tester.pumpAndSettle();

    expect(find.text('Which headset do you have?'), findsOneWidget);
  });

  testWidgets('the old download path redirects to the flashing docs',
      (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/download');
    await tester.pumpAndSettle();

    expect(find.text('Which headset do you have?'), findsOneWidget);
  });

  testWidgets('app download pages render and show source links',
      (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/download/hbsup');
    await tester.pumpAndSettle();
    expect(find.text('HBSUP downloads'), findsWidgets);
    expect(find.text('Windows is unsupported'), findsOneWidget);
    expect(find.text('What it looks like'), findsOneWidget);
    expect(find.byType(ShotCard), findsNWidgets(5));

    _routerOf(tester).go('/download/cte');
    await tester.pumpAndSettle();
    expect(find.text('HCTE downloads'), findsWidgets);
    expect(find.text('Windows is unsupported'), findsNothing);
    expect(find.byType(ShotCard), findsNothing);
  });

  testWidgets('backup section links to the HBSUP downloads',
      (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/flashdocs/pico-neo-2/linux');
    await tester.pumpAndSettle();

    final cta = find.text('Get HBSUP');
    await Scrollable.ensureVisible(tester.element(cta),
        alignment: 0.5);
    await tester.pumpAndSettle();
    await tester.tap(cta);
    await tester.pumpAndSettle();

    expect(find.text('HBSUP downloads'), findsWidgets);
  });

  testWidgets('flashdocs home asks for the headset', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/flashdocs');
    await tester.pumpAndSettle();

    expect(find.text('Which headset do you have?'), findsOneWidget);
    expect(find.text('Pico Neo 2'), findsWidgets);
  });

  testWidgets('flashdocs walks device then OS to the linux guide',
      (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/flashdocs/pico-neo-2');
    await tester.pumpAndSettle();

    expect(find.text('Which OS is your computer running?'),
        findsOneWidget);
    expect(find.text('Linux'), findsWidgets);

    _routerOf(tester).go('/flashdocs/pico-neo-2/linux');
    await tester.pumpAndSettle();

    expect(find.text('Flash Hibiscus on the Pico Neo 2'), findsOneWidget);
    expect(find.text('Read this first'), findsOneWidget);
    expect(find.text('1. Root the headset'), findsOneWidget);
    expect(find.text('fastboot oem pico unlock'), findsWidgets);
    expect(
        find.text('fastboot flash boot magisk_patched_pico_neo_2_boot.img'),
        findsOneWidget);
    expect(find.text('Download the patched boot image'), findsOneWidget);
    expect(find.text('Available builds'), findsOneWidget);
  });

  testWidgets('flashdocs vmd route shows the VM guide', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/flashdocs/vmd');
    await tester.pumpAndSettle();
    expect(find.text('Which OS is your computer running?'),
        findsOneWidget);

    _routerOf(tester).go('/flashdocs/vmd/linux');
    await tester.pumpAndSettle();

    expect(find.text('Run Hibiscus in a VM'), findsOneWidget);
    expect(find.text('adb connect localhost:15555'), findsOneWidget);
    expect(find.text('Available builds'), findsOneWidget);
  });

  testWidgets('flashdocs rejects unknown device and OS slugs',
      (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/flashdocs/nope');
    await tester.pumpAndSettle();
    expect(find.text('Page not found'), findsOneWidget);

    _routerOf(tester).go('/flashdocs/pico-neo-2/windows');
    await tester.pumpAndSettle();
    expect(find.text('Page not found'), findsOneWidget);
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

  testWidgets('old status route shows the not-found page', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(await _app());
    await tester.pumpAndSettle();

    _routerOf(tester).go('/status');
    await tester.pumpAndSettle();

    expect(find.text('Page not found'), findsOneWidget);
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
    expect(find.text('Back to HibiscusXR'), findsOneWidget);
  });

  testWidgets('zh locale renders translated chrome', (tester) async {
    SharedPreferences.setMockInitialValues({'locale': 'zh'});
    final prefs = await SharedPreferences.getInstance();
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(SiteApp(settings: AppSettings(prefs)..load()));
    await tester.pumpAndSettle();

    expect(find.text('HibiscusXR'), findsWidgets);
    expect(find.text('刷机指南'), findsWidgets);
    expect(find.text('阅读文档'), findsOneWidget);
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

    await tester.tap(find.text('Does it actually work?'));
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
