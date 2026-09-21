import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/main.dart';
import 'package:pn2_quicksettings/src/models.dart';
import 'package:pn2_quicksettings/src/persistence.dart';
import 'package:pn2_quicksettings/src/platform/fake_settings_source.dart';
import 'package:pn2_quicksettings/src/settings_controller.dart';
import 'package:pn2_quicksettings/src/settings_store.dart';

void main() {
  Future<SettingsController> pump(
    WidgetTester tester,
    SettingsSnapshot initial,
  ) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    final c = SettingsController(
      source: FakeSettingsSource(initial: initial),
      persistence: MemoryPersistence(),
      store: SettingsStore(clock: () => DateTime(2023, 8, 16, 15, 52)),
    );
    addTearDown(c.dispose);
    await c.start();
    await tester.pumpWidget(QuickSettingsApp(controller: c));
    for (var i = 0; i < 20; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    return c;
  }

  testWidgets('quick settings panel golden', (tester) async {
    await pump(
      tester,
      const SettingsSnapshot(
        batteryLevel: 96,
        wifiSsid: 'neosalsa-5g',
        volume: 0.62,
        brightness: 0.38,
        toggles: {
          ToggleId.wifi: true,
          ToggleId.boundary: true,
          ToggleId.microphone: true,
        },
      ),
    );

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/quick_settings.png'),
    );
  });

  testWidgets('radios off golden', (tester) async {
    await pump(
      tester,
      const SettingsSnapshot(
        batteryLevel: 15,
        volume: 0.1,
        brightness: 0.9,
        toggles: {ToggleId.doNotDisturb: true},
      ),
    );

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/quick_settings_off.png'),
    );
  });
}
