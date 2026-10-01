import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/main.dart';
import 'package:pn2_settings/src/envs/env_info.dart';
import 'package:pn2_settings/src/envs/env_source.dart';
import 'package:pn2_settings/src/models.dart';
import 'package:pn2_settings/src/persistence.dart';
import 'package:pn2_settings/src/platform/fake_settings_source.dart';
import 'package:pn2_settings/src/settings_controller.dart';
import 'package:pn2_settings/src/settings_store.dart';

class _StubEnvs implements EnvSource {
  const _StubEnvs(this.options);

  final List<EnvOption> options;

  @override
  Future<List<EnvOption>> list() async => options;

  @override
  Future<bool> remove(String id) async => false;
}

void main() {
  Future<SettingsController> pump(
    WidgetTester tester,
    SettingsSnapshot initial, {
    MemoryPersistence? persistence,
    EnvSource envs = const EmptyEnvSource(),
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
      envs: envs,
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
        wifi: [
          WifiNetwork(
            ssid: 'neosalsa-5g',
            capabilities: '[WPA2-PSK-CCMP][ESS]',
            level: 4,
            connected: true,
            savedId: 2,
          ),
          WifiNetwork(
            ssid: 'lab-guest',
            capabilities: '[WPA2-PSK-CCMP]',
            level: 2,
            savedId: 5,
          ),
          WifiNetwork(ssid: 'cafe-open', capabilities: '[ESS]', level: 1),
        ],
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

  testWidgets('display section golden', (tester) async {
    await pump(
      tester,
      const SettingsSnapshot(
        sliders: {ItemId.brightness: 0.4, ItemId.ipd: 0.4},
      ),
      persistence: MemoryPersistence({'section': 'display'}),
    );

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/settings_display.png'),
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

  testWidgets('environment section golden', (tester) async {
    final c = await pump(
      tester,
      const SettingsSnapshot(
        texts: {ItemId.homeEnv: 'skyloft'},
      ),
      persistence: MemoryPersistence({'section': 'environment'}),
      envs: const _StubEnvs([
        EnvOption(
          id: 'skyloft',
          name: 'Sky Loft',
          version: '1.4',
          license: 'CC0',
        ),
        EnvOption(
          id: 'cabin',
          name: 'Forest Cabin',
          version: '0.9',
          license: 'MIT',
          homepage: 'https://example.com/cabin',
        ),
        EnvOption(id: 'broken', hasMap: false),
      ]),
    );
    // the section-entry rescan lands async; let it settle before the shot
    for (var i = 0; i < 20; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    expect(c.store.envOptions, hasLength(3));

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/settings_environment.png'),
    );
  });
}
