import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/main.dart';
import 'package:pn2_settings/src/models.dart';
import 'package:pn2_settings/src/persistence.dart';
import 'package:pn2_settings/src/platform/fake_settings_source.dart';
import 'package:pn2_settings/src/settings_controller.dart';
import 'package:pn2_settings/src/settings_store.dart';

void main() {
  Future<SettingsController> pump(
    WidgetTester tester,
    SettingsSnapshot initial, {
    MemoryPersistence? persistence,
  }) async {
    tester.view.physicalSize = const Size(1280, 800);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    final source = FakeSettingsSource(initial: initial);
    addTearDown(source.dispose);
    final c = SettingsController(
      source: source,
      persistence: persistence ?? MemoryPersistence(),
      store: SettingsStore(),
    );
    addTearDown(c.dispose);
    await c.start();
    await tester.pumpWidget(SettingsApp(controller: c));
    for (var i = 0; i < 10; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    return c;
  }

  testWidgets('settings wifi section golden', (tester) async {
    await pump(
      tester,
      const SettingsSnapshot(
        toggles: {ItemId.wifiToggle: true},
        sliders: {ItemId.volume: 0.6, ItemId.brightness: 0.4},
        texts: {ItemId.wifiSsid: 'neosalsa-5g'},
      ),
    );

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/settings_wifi.png'),
    );
  });

  testWidgets('about section golden', (tester) async {
    await pump(
      tester,
      const SettingsSnapshot(
        texts: {
          ItemId.modelName: 'A7B10',
          ItemId.androidVersion: '10',
          ItemId.hibiscusVersion: 'alpha-v2026.09.22-r7',
        },
      ),
      persistence: MemoryPersistence({'section': 'about'}),
    );

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/settings_about.png'),
    );
  });

  testWidgets('controllers section golden', (tester) async {
    await pump(
      tester,
      const SettingsSnapshot(
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
      persistence: MemoryPersistence({'section': 'controllers'}),
    );

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/settings_controllers.png'),
    );
  });
}
