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
  bool uiOnlyMode = false,
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
  await tester.pumpWidget(
    SettingsApp(controller: c, uiOnlyMode: uiOnlyMode),
  );
  await tester.pump();
  return (c, source);
}

void main() {
  testWidgets('ui-only mode pops a notice that dismisses',
      (tester) async {
    await pumpApp(tester, uiOnlyMode: true);
    await tester.pump();
    expect(find.text('UI-only mode'), findsOneWidget);
    await tester.tap(find.text('OK'));
    await tester.pump();
    await tester.pump(const Duration(milliseconds: 300));
    expect(find.text('UI-only mode'), findsNothing);
  });

  testWidgets('ui-only notice stays off by default', (tester) async {
    await pumpApp(tester);
    await tester.pump();
    expect(find.text('UI-only mode'), findsNothing);
  });

  testWidgets('sidebar lists sections and selects', (tester) async {
    final (c, _) = await pumpApp(tester);
    expect(find.text('Wi-Fi'), findsWidgets);
    expect(find.text('Display'), findsOneWidget);
    expect(find.text('Developer'), findsOneWidget);
    await tester.tap(find.text('About'));
    await tester.pump();
    expect(c.store.section, SectionId.about);
    expect(find.text('HibiscusXR'), findsOneWidget);
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
    await tester.tap(find.text('Display'));
    await tester.pump();
    // night mode is the only stub row: it renders dimmed and its
    // switch forwards nothing to the platform
    expect(
      find.byWidgetPredicate((w) => w is Opacity && w.opacity < 1),
      findsOneWidget,
    );
    await tester.tap(find.byType(Switch));
    await tester.pump();
    expect(source.togglesRequested, isEmpty);
  });

  testWidgets('implemented controls stay enabled', (tester) async {
    final (_, source) = await pumpApp(tester);
    await tester.tap(find.text('Display'));
    await tester.pump();
    // night mode is a stub: its switch takes no taps
    await tester.tap(find.byType(Switch));
    await tester.pump();
    expect(source.togglesRequested, isEmpty);
    // wifi is real: its switch forwards as before
    await tester.tap(find.text('Wi-Fi'));
    await tester.pump();
    await tester.tap(find.byType(Switch).first);
    await tester.pump();
    expect(source.togglesRequested, [(ItemId.wifiToggle, true)]);
  });

  testWidgets('developer toggles render as inert stubs',
      (tester) async {
    final (_, source) = await pumpApp(tester);
    await tester.tap(find.text('Developer'));
    await tester.pump();
    expect(find.text('USB debugging'), findsOneWidget);
    expect(find.text('Stay awake'), findsOneWidget);
    expect(find.text('Show touches'), findsOneWidget);
    expect(
      find.byWidgetPredicate((w) => w is Opacity && w.opacity < 1),
      findsNWidgets(3),
    );
    await tester.tap(find.byType(Switch).first);
    await tester.pump();
    expect(source.togglesRequested, isEmpty);
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
    // each name/state shows twice: once on the scan card chips and
    // once on the dedicated rows
    expect(find.text('Left controller'), findsNWidgets(2));
    expect(find.text('Right controller'), findsNWidgets(2));
    expect(find.text('Connected'), findsNWidgets(2));
    expect(find.text('Disconnected'), findsNWidgets(2));
  });

  testWidgets('scan card and unpair forward to the source',
      (tester) async {
    final (_, source) = await pumpApp(tester);
    await tester.tap(find.text('Controllers'));
    await tester.pump();
    await tester.tap(find.text('Scan for controllers'));
    await tester.pump();
    await tester.tap(find.byIcon(Icons.chevron_right).first);
    await tester.pump();
    expect(source.actionsPerformed,
        [ItemId.controllerPair, ItemId.controllerUnbind]);
  });

  testWidgets('scanning state shows on the card', (tester) async {
    await pumpApp(
      tester,
      initial: const SettingsSnapshot(
        toggles: {ItemId.controllerPair: true},
        controllers: {
          ItemId.controllerLeft:
              ControllerInfo(link: ControllerLink.pairing),
        },
      ),
    );
    await tester.tap(find.text('Controllers'));
    await tester.pump();
    expect(find.byType(CircularProgressIndicator), findsOneWidget);
    expect(find.text('Scanning for controllers…'), findsOneWidget);
    expect(find.text('Pairing…'), findsNWidgets(2));
  });

  testWidgets('idle scan card shows its status', (tester) async {
    await pumpApp(tester);
    await tester.tap(find.text('Controllers'));
    await tester.pump();
    expect(find.text('Not scanning'), findsOneWidget);
    expect(find.byType(CircularProgressIndicator), findsNothing);
  });

  testWidgets('scan card chips fit a narrow window', (tester) async {
    await pumpApp(
      tester,
      initial: const SettingsSnapshot(
        controllers: {
          ItemId.controllerLeft:
              ControllerInfo(link: ControllerLink.disconnected),
          ItemId.controllerRight:
              ControllerInfo(link: ControllerLink.disconnected),
        },
      ),
    );
    tester.view.physicalSize = const Size(640, 800);
    await tester.pump();
    await tester.tap(find.text('Controllers'));
    await tester.pump();
    expect(tester.takeException(), isNull);
    expect(find.text('Left controller'), findsWidgets);
    expect(find.text('Right controller'), findsWidgets);
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
    expect(find.text('Disconnected'), findsNWidgets(2));

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
    expect(find.text('Connected'), findsNWidgets(2));
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
