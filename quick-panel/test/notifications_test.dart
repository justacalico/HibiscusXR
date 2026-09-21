import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/main.dart';
import 'package:pn2_quicksettings/src/models.dart';
import 'package:pn2_quicksettings/src/persistence.dart';
import 'package:pn2_quicksettings/src/platform/fake_settings_source.dart';
import 'package:pn2_quicksettings/src/settings_controller.dart';
import 'package:pn2_quicksettings/src/settings_store.dart';

const _a = NotificationItem(
  key: 'k1',
  app: 'Library',
  title: 'Download finished',
  text: 'vrhome.apk',
  postMs: 1,
  clearable: true,
);
const _b = NotificationItem(
  key: 'k2',
  app: 'System UI',
  title: 'Charging this device',
  text: 'Tap for more options.',
  postMs: 2,
  clearable: false,
);

void main() {
  Future<(SettingsController, FakeSettingsSource)> pump(
    WidgetTester tester,
    SettingsSnapshot initial,
  ) async {
    tester.view.physicalSize = const Size(1200, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);
    final src = FakeSettingsSource(initial: initial);
    final c = SettingsController(
      source: src,
      persistence: MemoryPersistence(),
      store: SettingsStore(clock: () => DateTime(2023, 8, 16, 15, 52)),
    );
    addTearDown(c.dispose);
    await c.start();
    await tester.pumpWidget(QuickSettingsApp(controller: c));
    await tester.pumpAndSettle();
    return (c, src);
  }

  testWidgets('empty shade shows the placeholder', (tester) async {
    await pump(tester, const SettingsSnapshot());
    expect(find.text('Notifications'), findsOneWidget);
    expect(find.text('No notifications'), findsOneWidget);
    expect(find.text('Clear all'), findsNothing);
  });

  testWidgets('rows render and dismiss forwards the key', (tester) async {
    final (c, src) = await pump(
      tester,
      const SettingsSnapshot(notifications: [_a, _b]),
    );
    expect(find.text('Download finished'), findsOneWidget);
    expect(find.text('Charging this device'), findsOneWidget);
    // only the clearable row gets a dismiss button
    expect(find.byIcon(Icons.close), findsOneWidget);
    expect(find.text('Clear all'), findsOneWidget);

    await tester.tap(find.byIcon(Icons.close));
    await tester.pumpAndSettle();
    expect(find.text('Download finished'), findsNothing);
    expect(src.notificationsDismissed, ['k1']);
    expect(c.store.notifications.map((n) => n.key), ['k2']);
  });

  testWidgets('clear all drops clearable rows', (tester) async {
    final (c, src) = await pump(
      tester,
      const SettingsSnapshot(notifications: [_a, _b]),
    );
    await tester.tap(find.text('Clear all'));
    await tester.pumpAndSettle();
    expect(find.text('Download finished'), findsNothing);
    expect(find.text('Charging this device'), findsOneWidget);
    expect(src.dismissAllCount, 1);
    expect(c.store.notifications.map((n) => n.key), ['k2']);
  });
}
