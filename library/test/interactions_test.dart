import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_library/main.dart';
import 'package:pn2_library/src/library_controller.dart';
import 'package:pn2_library/src/library_store.dart';
import 'package:pn2_library/src/models.dart';
import 'package:pn2_library/src/persistence.dart';
import 'package:pn2_library/src/platform/fake_app_source.dart';

List<AppEntry> demoApps() => [
  const AppEntry(packageName: 'a', label: 'Alpha'),
  const AppEntry(packageName: 'b', label: 'Beta'),
  const AppEntry(packageName: 'c', label: 'Gamma'),
  const AppEntry(packageName: 's', label: 'Sys', isSystem: true),
];

Future<LibraryController> pumpApp(
  WidgetTester tester,
  FakeAppSource src,
) async {
  final c = LibraryController(source: src, persistence: MemoryPersistence());
  addTearDown(c.dispose);
  await tester.pumpWidget(LibraryApp(controller: c));
  for (var i = 0; i < 10; i++) {
    await tester.pump(const Duration(milliseconds: 50));
  }
  return c;
}

void main() {
  testWidgets('tapping a tile launches the app', (tester) async {
    final src = FakeAppSource(apps: demoApps());
    addTearDown(src.dispose);
    await pumpApp(tester, src);
    await tester.tap(find.text('Alpha'));
    expect(src.launched, ['a']);
  });

  testWidgets('search field filters the grid', (tester) async {
    final src = FakeAppSource(apps: demoApps());
    addTearDown(src.dispose);
    await pumpApp(tester, src);
    await tester.enterText(find.byType(TextField), 'bet');
    await tester.pump();
    expect(find.text('Beta'), findsOneWidget);
    expect(find.text('Alpha'), findsNothing);
  });

  testWidgets('kebab opens the menu, uninstall hidden for system apps', (
    tester,
  ) async {
    final src = FakeAppSource(apps: demoApps());
    addTearDown(src.dispose);
    await pumpApp(tester, src);
    // system tile kebab
    final kebabs = find.byIcon(Icons.more_vert);
    await tester.tap(kebabs.last);
    await tester.pumpAndSettle();
    expect(find.text('Open'), findsOneWidget);
    expect(find.text('Uninstall'), findsNothing);
    await tester.tapAt(const Offset(10, 10));
    await tester.pumpAndSettle();
    // user tile kebab -> uninstall present
    await tester.tap(kebabs.first);
    await tester.pumpAndSettle();
    expect(find.text('Uninstall'), findsOneWidget);
  });

  testWidgets('pin via menu moves the tile to the front', (tester) async {
    final src = FakeAppSource(apps: demoApps());
    addTearDown(src.dispose);
    final c = await pumpApp(tester, src);
    final kebabOfGamma = find.byIcon(Icons.more_vert).at(2);
    await tester.tap(kebabOfGamma);
    await tester.pumpAndSettle();
    await tester.tap(find.text('Pin'));
    await tester.pump();
    expect(c.store.isPinned('c'), isTrue);
    expect(c.store.visible.first.packageName, 'c');
  });

  testWidgets('d-pad enter launches the focused tile', (tester) async {
    final src = FakeAppSource(apps: demoApps());
    addTearDown(src.dispose);
    await pumpApp(tester, src);
    await tester.sendKeyEvent(LogicalKeyboardKey.select);
    expect(src.launched, ['a']); // first tile is autofocused
  });

  testWidgets('arrow right moves focus then enter launches', (tester) async {
    final src = FakeAppSource(apps: demoApps());
    addTearDown(src.dispose);
    await pumpApp(tester, src);
    await tester.sendKeyEvent(LogicalKeyboardKey.arrowRight);
    await tester.pump();
    await tester.sendKeyEvent(LogicalKeyboardKey.enter);
    expect(src.launched, ['b']);
  });

  testWidgets('collection menu filters to user apps', (tester) async {
    final src = FakeAppSource(apps: demoApps());
    addTearDown(src.dispose);
    final c = await pumpApp(tester, src);
    await tester.tap(find.text('All (4)'));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Apps (3)'));
    await tester.pump();
    expect(c.store.filter, isA<FilterUserApps>());
    expect(find.text('Sys'), findsNothing);
  });
}
