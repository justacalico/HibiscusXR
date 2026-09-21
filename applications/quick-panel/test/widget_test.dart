import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/main.dart';
import 'package:pn2_quicksettings/src/models.dart';
import 'package:pn2_quicksettings/src/persistence.dart';
import 'package:pn2_quicksettings/src/platform/fake_settings_source.dart';
import 'package:pn2_quicksettings/src/settings_controller.dart';
import 'package:pn2_quicksettings/src/settings_store.dart';

SettingsController makeController({
  SettingsSnapshot initial = const SettingsSnapshot(),
  DateTime Function()? clock,
}) {
  final controller = SettingsController(
    source: FakeSettingsSource(initial: initial),
    persistence: MemoryPersistence(),
    store: SettingsStore(clock: clock ?? () => DateTime(2023, 8, 16, 15, 52)),
  );
  addTearDown(controller.dispose);
  return controller;
}

void main() {
  testWidgets('panel renders status bar, sliders and tiles', (tester) async {
    tester.view.physicalSize = const Size(1200, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    final c = makeController(
      initial: const SettingsSnapshot(
        batteryLevel: 96,
        wifiSsid: 'neosalsa-5g',
        toggles: {ToggleId.wifi: true},
      ),
    );
    await c.start();
    await tester.pumpWidget(QuickSettingsApp(controller: c));
    await tester.pumpAndSettle();

    expect(find.text('96%'), findsOneWidget);
    expect(find.text('Wed, Aug 16, 2023'), findsOneWidget);
    expect(find.text('Wi-Fi'), findsOneWidget);
    expect(find.text('neosalsa-5g'), findsOneWidget);
    expect(find.text('Bluetooth'), findsOneWidget);
    expect(find.text('Reset view'), findsOneWidget);
  });
}
