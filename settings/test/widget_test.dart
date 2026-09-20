import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/main.dart';
import 'package:pn2_settings/src/models.dart';
import 'package:pn2_settings/src/persistence.dart';
import 'package:pn2_settings/src/platform/fake_settings_source.dart';
import 'package:pn2_settings/src/settings_controller.dart';
import 'package:pn2_settings/src/settings_store.dart';

Future<(SettingsController, FakeSettingsSource)> pumpApp(
  WidgetTester tester, {
  SettingsSnapshot initial = const SettingsSnapshot(),
}) async {
  tester.view.physicalSize = const Size(1280, 800);
  tester.view.devicePixelRatio = 1.0;
  addTearDown(tester.view.reset);

  final source = FakeSettingsSource(initial: initial);
  final c = SettingsController(
    source: source,
    persistence: MemoryPersistence(),
    store: SettingsStore(),
  );
  addTearDown(c.dispose);
  addTearDown(source.dispose);
  await c.start();
  await tester.pumpWidget(SettingsApp(controller: c));
  await tester.pump();
  return (c, source);
}

void main() {
  testWidgets('sidebar lists sections and selects', (tester) async {
    final (c, _) = await pumpApp(tester);
    expect(find.text('Wi-Fi'), findsWidgets);
    expect(find.text('Headset Tracking'), findsOneWidget);
    expect(find.text('Software Update'), findsOneWidget);
    await tester.tap(find.text('About'));
    await tester.pump();
    expect(c.store.section, SectionId.about);
    expect(find.text('Model'), findsOneWidget);
    expect(find.text('Android version'), findsOneWidget);
  });

  testWidgets('toggle row forwards to the source', (tester) async {
    final (_, source) = await pumpApp(tester);
    await tester.tap(find.byType(Switch).first);
    await tester.pump();
    expect(source.togglesRequested, [(ItemId.wifiToggle, true)]);
  });

  testWidgets('action row forwards to the source', (tester) async {
    final (_, source) = await pumpApp(tester);
    await tester.tap(find.byIcon(Icons.chevron_right).first);
    await tester.pump();
    expect(source.actionsPerformed, contains(ItemId.wifiSettings));
  });

  testWidgets('unimplemented rows grey out and ignore input',
      (tester) async {
    final (_, source) = await pumpApp(tester);
    await tester.tap(find.text('Headset Tracking'));
    await tester.pump();
    // every row in the section is a stub: all render dimmed and none of
    // the controls forward anything to the platform
    expect(find.byType(Opacity), findsWidgets);
    await tester.tap(find.byType(Switch).first);
    await tester.pump();
    expect(source.togglesRequested, isEmpty);
    await tester.tap(find.text('Auto'));
    await tester.pump();
    expect(find.text('60 Hz'), findsNothing);
    await tester.tap(find.byIcon(Icons.chevron_right).last);
    await tester.pump();
    expect(source.actionsPerformed, isEmpty);
  });

  testWidgets('implemented controls stay enabled', (tester) async {
    final (_, source) = await pumpApp(tester);
    await tester.tap(find.text('Camera'));
    await tester.pump();
    // seethrough is a stub: its switch takes no taps
    await tester.tap(find.byType(Switch).first);
    await tester.pump();
    expect(source.togglesRequested, isEmpty);
    // wifi is real: its switch forwards as before
    await tester.tap(find.text('Wi-Fi'));
    await tester.pump();
    await tester.tap(find.byType(Switch).first);
    await tester.pump();
    expect(source.togglesRequested, [(ItemId.wifiToggle, true)]);
  });

  testWidgets('slider row forwards drag', (tester) async {
    final (_, source) = await pumpApp(tester);
    await tester.tap(find.text('Display'));
    await tester.pump();
    await tester.drag(find.byType(Slider).first, const Offset(120, 0));
    await tester.pump();
    expect(source.slidersSet, isNotEmpty);
    expect(source.slidersSet.first.$1, ItemId.brightness);
  });

  testWidgets('info rows show platform text and empty fallback',
      (tester) async {
    await pumpApp(
      tester,
      initial: const SettingsSnapshot(
        texts: {
          ItemId.modelName: 'A7B10',
          ItemId.androidVersion: '10',
          ItemId.buildNumber: 'pn2-full-42',
        },
      ),
    );
    await tester.tap(find.text('About'));
    await tester.pump();
    expect(find.text('A7B10'), findsOneWidget);
    expect(find.text('10'), findsOneWidget);

    await tester.tap(find.text('Wi-Fi'));
    await tester.pump();
    expect(find.text('Not connected'), findsOneWidget);
  });

  testWidgets('controllers section shows state and battery', (tester) async {
    await pumpApp(
      tester,
      initial: const SettingsSnapshot(
        controllers: {
          ItemId.controllerLeft: ControllerInfo(
            link: ControllerLink.connected,
            battery: 4,
          ),
          ItemId.controllerRight: ControllerInfo(
            link: ControllerLink.disconnected,
            battery: -1,
          ),
        },
      ),
    );
    await tester.tap(find.text('Controllers'));
    await tester.pump();
    expect(find.text('Left controller'), findsOneWidget);
    expect(find.text('Right controller'), findsOneWidget);
    expect(find.text('Connected'), findsOneWidget);
    expect(find.text('Disconnected'), findsOneWidget);
  });

  testWidgets('pair and unpair actions forward to the source',
      (tester) async {
    final (_, source) = await pumpApp(tester);
    await tester.tap(find.text('Controllers'));
    await tester.pump();
    await tester.tap(find.byIcon(Icons.chevron_right).first);
    await tester.pump();
    await tester.tap(find.byIcon(Icons.chevron_right).last);
    await tester.pump();
    expect(source.actionsPerformed,
        [ItemId.controllerPair, ItemId.controllerUnbind]);
  });

  testWidgets('controller events refresh the row', (tester) async {
    final (_, source) = await pumpApp(
      tester,
      initial: const SettingsSnapshot(
        controllers: {
          ItemId.controllerRight:
              ControllerInfo(link: ControllerLink.disconnected),
        },
      ),
    );
    await tester.tap(find.text('Controllers'));
    await tester.pump();
    expect(find.text('Disconnected'), findsOneWidget);

    source.emit(const SettingsSnapshot(
      controllers: {
        ItemId.controllerRight: ControllerInfo(
          link: ControllerLink.connected,
          battery: 5,
          charging: true,
        ),
      },
    ));
    await tester.pump();
    await tester.pump();
    expect(find.text('Connected'), findsOneWidget);
  });

  testWidgets('live events update the ui', (tester) async {
    final (_, source) = await pumpApp(tester);
    source.emit(const SettingsSnapshot(
      texts: {ItemId.wifiSsid: 'fresh-net'},
    ));
    await tester.pump();
    await tester.pump();
    expect(find.text('fresh-net'), findsOneWidget);
  });
}
