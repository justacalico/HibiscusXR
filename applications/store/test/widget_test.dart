import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_store/main.dart' as app;
import 'package:pn2_store/src/installer.dart';
import 'package:pn2_store/src/platform/fakes.dart';
import 'package:pn2_store/src/store_controller.dart';
import 'package:pn2_store/src/persistence.dart';
import 'package:pn2_store/src/store_state.dart';
import 'package:pn2_store/src/ui/theme.dart';
import 'package:pn2_store/src/ui/widgets.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'helpers.dart';

Future<(StoreController,)> pumpApp(
  WidgetTester tester, {
  StoreController? controller,
  Size size = const Size(1280, 800),
}) async {
  tester.view.physicalSize = size;
  tester.view.devicePixelRatio = 1.0;
  addTearDown(tester.view.reset);
  final c = controller ?? await readyController();
  await tester.pumpWidget(app.StoreApp(controller: c));
  for (var i = 0; i < 10; i++) {
    await tester.pump(const Duration(milliseconds: 50));
  }
  return (c,);
}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('main boots the app with an injected controller', (tester) async {
    tester.view.physicalSize = const Size(1280, 800);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    final c = await readyController();
    await app.main(controller: c);
    for (var i = 0; i < 10; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    expect(find.text('Store'), findsOneWidget);
    expect(find.text('Alpha Player'), findsOneWidget);
  });

  testWidgets('buildStoreController assembles the real wiring', (tester) async {
    SharedPreferences.setMockInitialValues({});
    final c = await app.buildStoreController();
    expect(c.store.repoUrl, StoreController.kDefaultRepo);
  });

  testWidgets('main without a controller builds the real wiring',
      (tester) async {
    SharedPreferences.setMockInitialValues({});
    // runAsync because the default repo fetch is a real socket call;
    // no pump afterwards - the loading spinner's ticker would mix real
    // and fake clocks and trip elapsedInSeconds
    await tester.runAsync(() => app.main());
  });

  testWidgets('app cards fall back to the repo url without an index',
      (tester) async {
    final c = testController(); // never started: index is still null
    final app0 = testIndex().apps.first;
    await tester.pumpWidget(
      MaterialApp(
        theme: StoreTheme.data(),
        home: Scaffold(
          body: AppCard(
            app: app0,
            controller: c,
            selected: false,
            onTap: () {},
          ),
        ),
      ),
    );
    await tester.pump();
    expect(find.text('Alpha Player'), findsOneWidget);
  });

  testWidgets('catalog lists apps and shows count', (tester) async {
    await pumpApp(tester);
    expect(find.text('3 apps'), findsOneWidget);
    expect(find.text('Alpha Player'), findsOneWidget);
    expect(find.text('Beta Tool'), findsOneWidget);
    expect(find.text('Gamma Notes'), findsOneWidget);
    // sorted by recency: alpha first
    expect(find.text('Plays things'), findsOneWidget);
  });

  testWidgets('search narrows the list', (tester) async {
    await pumpApp(tester);
    await tester.enterText(find.byType(TextField).first, 'notes');
    await tester.pumpAndSettle();
    expect(find.text('Gamma Notes'), findsOneWidget);
    expect(find.text('Alpha Player'), findsNothing);
  });

  testWidgets('category chips filter the list', (tester) async {
    await pumpApp(tester);
    await tester.tap(find.text('System'));
    await tester.pumpAndSettle();
    expect(find.text('Beta Tool'), findsOneWidget);
    expect(find.text('Alpha Player'), findsNothing);
    await tester.tap(find.text('All'));
    await tester.pumpAndSettle();
    expect(find.text('Alpha Player'), findsOneWidget);
  });

  testWidgets('sort menu reorders alphabetically', (tester) async {
    await pumpApp(tester);
    await tester.tap(find.byIcon(Icons.sort));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Name A-Z'));
    await tester.pumpAndSettle();
    final betaDy = tester.getTopLeft(find.text('Beta Tool')).dy;
    final gammaDy = tester.getTopLeft(find.text('Gamma Notes')).dy;
    expect(betaDy, lessThan(gammaDy));
  });

  testWidgets('selecting an app fills the detail pane', (tester) async {
    final (c,) = await pumpApp(tester);
    await tester.tap(find.text('Alpha Player'));
    await tester.pumpAndSettle();
    expect(c.store.selectedPackage, 'com.example.alpha');
    expect(find.text('by Example Inc'), findsOneWidget);
    expect(find.text('GPL-3.0-only'), findsOneWidget);
    expect(find.text('1.2'), findsOneWidget);
    expect(find.text('API 24'), findsOneWidget);
    expect(find.text('Screenshots'), findsOneWidget);
    expect(find.text('Versions'), findsOneWidget);
    expect(find.text('1.0 (10)'), findsOneWidget);
  });

  testWidgets('install flow ends on an open button', (tester) async {
    final (c,) = await pumpApp(tester);
    await tester.tap(find.text('Alpha Player'));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Install'));
    // let the mocked download + install finish
    for (var i = 0; i < 20; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    expect(find.text('Open'), findsOneWidget);
    await tester.tap(find.text('Open'));
    await tester.pumpAndSettle();
    expect(
      (c.installer as FakeInstaller).openedPackages,
      ['com.example.alpha'],
    );
  });

  testWidgets('install failure shows retry', (tester) async {
    final installer = FakeInstaller(outcome: InstallOutcome.failed);
    final (c,) = await pumpApp(
      tester,
      controller: await readyController(installer: installer),
    );
    await tester.tap(find.text('Beta Tool'));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Install'));
    for (var i = 0; i < 20; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    expect(find.text('Retry'), findsOneWidget);
    expect(find.textContaining('Install failed'), findsOneWidget);
    // tapping retry kicks the flow off again
    installer.outcome = InstallOutcome.installed;
    await tester.tap(find.text('Retry'));
    for (var i = 0; i < 20; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    expect(installer.installedPaths, hasLength(2));
    expect(
      c.store.progressOf('com.example.beta').phase,
      InstallPhase.installed,
    );
  });

  testWidgets('installing and downloading phases render', (tester) async {
    final (c,) = await pumpApp(tester);
    await tester.tap(find.text('Beta Tool'));
    await tester.pumpAndSettle();
    c.store.setInstall(
      'com.example.beta',
      const InstallProgress(InstallPhase.downloading, progress: 0.4),
    );
    await tester.pump();
    expect(find.text('Downloading'), findsOneWidget);
    expect(find.byType(LinearProgressIndicator), findsOneWidget);
    c.store.setInstall(
      'com.example.beta',
      const InstallProgress(InstallPhase.installing),
    );
    await tester.pump();
    expect(find.text('Installing'), findsOneWidget);
  });

  testWidgets('compact layout pushes detail as a route', (tester) async {
    await pumpApp(tester, size: const Size(480, 800));
    // no two-pane: the detail placeholder isn't on screen
    expect(find.text('Pick an app to see details'), findsNothing);
    await tester.tap(find.text('Alpha Player'));
    await tester.pumpAndSettle();
    expect(find.text('by Example Inc'), findsOneWidget);
    // back pops the route
    await tester.tap(find.byIcon(Icons.arrow_back));
    await tester.pumpAndSettle();
    expect(find.text('3 apps'), findsOneWidget);
  });

  testWidgets('repo sheet validates and saves', (tester) async {
    final persistence = MemoryPersistence();
    final (c,) = await pumpApp(
      tester,
      controller: await readyController(persistence: persistence),
    );
    await tester.tap(find.byIcon(Icons.dns_outlined));
    await tester.pumpAndSettle();
    expect(find.text('Repository'), findsWidgets);
    // invalid input shows the error and does not save
    await tester.enterText(find.byType(TextField).last, 'ftp://x');
    await tester.tap(find.text('Save'));
    await tester.pumpAndSettle();
    expect(find.text('Enter an http or https URL'), findsOneWidget);
    expect(c.store.repoUrl, StoreController.kDefaultRepo);
    // a good url persists and reloads
    await tester.enterText(
      find.byType(TextField).last,
      'https://home.example/repo',
    );
    await tester.tap(find.text('Save'));
    await tester.pumpAndSettle();
    expect(c.store.repoUrl, Uri.parse('https://home.example/repo'));
    expect(persistence.stored!['repoUrl'], 'https://home.example/repo');
  });

  testWidgets('repo sheet submits from the keyboard', (tester) async {
    final persistence = MemoryPersistence();
    final (c,) = await pumpApp(
      tester,
      controller: await readyController(persistence: persistence),
    );
    await tester.tap(find.byIcon(Icons.dns_outlined));
    await tester.pumpAndSettle();
    await tester.enterText(
      find.byType(TextField).last,
      'https://kbd.example/repo',
    );
    await tester.testTextInput.receiveAction(TextInputAction.done);
    await tester.pumpAndSettle();
    expect(c.store.repoUrl, Uri.parse('https://kbd.example/repo'));
  });

  testWidgets('repo sheet reset and cancel', (tester) async {
    final (c,) = await pumpApp(tester);
    await tester.tap(find.byIcon(Icons.dns_outlined));
    await tester.pumpAndSettle();
    await tester.enterText(find.byType(TextField).last, 'https://other/');
    await tester.tap(find.text('Reset to default'));
    await tester.pumpAndSettle();
    expect(
      (tester.widget(find.byType(TextField).last) as TextField)
          .controller!
          .text,
      StoreController.kDefaultRepo.toString(),
    );
    await tester.tap(find.text('Cancel'));
    await tester.pumpAndSettle();
    expect(find.text('Repository'), findsNothing);
    expect(c.store.repoUrl, StoreController.kDefaultRepo);
  });

  testWidgets('load error pane retries and recovers', (tester) async {
    tester.view.physicalSize = const Size(1280, 800);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    final client = FakeRepoClient(
      throwError: const _FakeRepoError('down'),
      index: testIndex(),
    );
    final base = testController();
    final failing = StoreController(
      client: client,
      downloader: base.downloader,
      installer: base.installer,
      persistence: MemoryPersistence(),
    );
    await failing.start();
    await tester.pumpWidget(app.StoreApp(controller: failing));
    for (var i = 0; i < 10; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    expect(find.textContaining("Couldn't load the catalog"), findsOneWidget);
    // recover and retry
    client.throwError = null;
    await tester.tap(find.text('Retry'));
    for (var i = 0; i < 10; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    expect(find.text('3 apps'), findsOneWidget);
  });

  testWidgets('empty search result shows the empty pane', (tester) async {
    await pumpApp(tester);
    await tester.enterText(find.byType(TextField).first, 'zzzz');
    await tester.pumpAndSettle();
    expect(find.text('No apps match your search'), findsOneWidget);
  });

  testWidgets('detail route with no selection shows the placeholder',
      (tester) async {
    final (c,) = await pumpApp(tester, size: const Size(480, 800));
    await tester.tap(find.text('Alpha Player'));
    await tester.pumpAndSettle();
    // the index refresh (or state reset) dropped the selection - the
    // route shows the placeholder instead of a blank page
    c.store.select(null);
    await tester.pumpAndSettle();
    expect(find.text('Pick an app to see details'), findsOneWidget);
  });

  testWidgets('broken screenshot bytes collapse to nothing', (tester) async {
    tester.view.physicalSize = const Size(1280, 800);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    final c = await readyController();
    // screenshots resolve to undecodable bytes; icons still work
    c.imageResolver = (url) => url.contains('phone/')
        ? MemoryImage(Uint8List.fromList(const [1, 2, 3]))
        : testImageResolver(url);
    await tester.pumpWidget(app.StoreApp(controller: c));
    await tester.tap(find.text('Alpha Player'));
    await tester.pumpAndSettle();
    await tester.pump();
    expect(find.text('Screenshots'), findsOneWidget);
  });
}

class _FakeRepoError implements Exception {
  const _FakeRepoError(this.message);
  final String message;
}
