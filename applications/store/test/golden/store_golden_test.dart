import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_store/main.dart';
import 'package:pn2_store/src/store_controller.dart';

import '../helpers.dart';

Future<StoreController> pumpGolden(
  WidgetTester tester, {
  Size size = const Size(1280, 800),
}) async {
  tester.view.physicalSize = size;
  tester.view.devicePixelRatio = 1.0;
  addTearDown(tester.view.reset);
  final c = await readyController();
  await tester.pumpWidget(StoreApp(controller: c));
  for (var i = 0; i < 10; i++) {
    await tester.pump(const Duration(milliseconds: 50));
  }
  return c;
}

void main() {
  testWidgets('catalog golden', (tester) async {
    await pumpGolden(tester);
    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/store_catalog.png'),
    );
  });

  testWidgets('detail pane golden', (tester) async {
    final c = await pumpGolden(tester);
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
    await pumpGolden(tester, size: const Size(480, 800));
    await tester.tap(find.text('Alpha Player'));
    await tester.pumpAndSettle();
    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/store_detail_compact.png'),
    );
  });

  testWidgets('error golden', (tester) async {
    tester.view.physicalSize = const Size(1280, 800);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    final c = testController(throwError: Exception('offline'));
    await c.start();
    await tester.pumpWidget(StoreApp(controller: c));
    for (var i = 0; i < 10; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/store_error.png'),
    );
  });
}
