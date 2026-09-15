import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/main.dart';
import 'package:pn2_quicksettings/src/models.dart';
import 'package:pn2_quicksettings/src/persistence.dart';
import 'package:pn2_quicksettings/src/platform/fake_settings_source.dart';
import 'package:pn2_quicksettings/src/settings_controller.dart';
import 'package:pn2_quicksettings/src/ui/panel_slider.dart';

void main() {
  testWidgets('tapping a toggle tile flips it and notifies the source',
      (tester) async {
    tester.view.physicalSize = const Size(1200, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    final src = FakeSettingsSource();
    final c = SettingsController(
      source: src,
      persistence: MemoryPersistence(),
    );
    addTearDown(c.dispose);
    await c.start();
    await tester.pumpWidget(QuickSettingsApp(controller: c));
    await tester.pumpAndSettle();

    await tester.tap(find.text('Wi-Fi'));
    await tester.pump();
    expect(c.store.isOn(ToggleId.wifi), isTrue);
    expect(src.togglesRequested, [(ToggleId.wifi, true)]);
  });

  testWidgets('action tiles dispatch instead of toggling', (tester) async {
    tester.view.physicalSize = const Size(1200, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    final src = FakeSettingsSource();
    final c = SettingsController(
      source: src,
      persistence: MemoryPersistence(),
    );
    addTearDown(c.dispose);
    await c.start();
    await tester.pumpWidget(QuickSettingsApp(controller: c));
    await tester.pumpAndSettle();

    await tester.tap(find.text('Reset view'));
    await tester.pump();
    expect(src.actionsPerformed, [ActionId.resetView]);
  });

  testWidgets('gear and footer buttons dispatch actions', (tester) async {
    tester.view.physicalSize = const Size(1200, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    final src = FakeSettingsSource();
    final c = SettingsController(
      source: src,
      persistence: MemoryPersistence(),
    );
    addTearDown(c.dispose);
    await c.start();
    await tester.pumpWidget(QuickSettingsApp(controller: c));
    await tester.pumpAndSettle();

    await tester.tap(find.byIcon(Icons.settings));
    await tester.tap(find.byIcon(Icons.close));
    await tester.tap(find.byIcon(Icons.remove));
    await tester.pump();
    expect(src.actionsPerformed, [
      ActionId.openSettings,
      ActionId.close,
      ActionId.minimize,
    ]);
  });

  testWidgets('dragging the volume slider updates the source',
      (tester) async {
    tester.view.physicalSize = const Size(1200, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    final src = FakeSettingsSource();
    final c = SettingsController(
      source: src,
      persistence: MemoryPersistence(),
    );
    addTearDown(c.dispose);
    await c.start();
    await tester.pumpWidget(QuickSettingsApp(controller: c));
    await tester.pumpAndSettle();

    final slider = find.byType(PanelSlider).first;
    final rect = tester.getRect(slider);
    await tester.tapAt(Offset(rect.left + rect.width * 0.9, rect.center.dy));
    await tester.pump();
    expect(c.store.volume, greaterThan(0.8));
    expect(src.volumesSet, isNotEmpty);
  });

  testWidgets('d-pad arrows move tile focus and enter activates',
      (tester) async {
    tester.view.physicalSize = const Size(1200, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    final src = FakeSettingsSource();
    final c = SettingsController(
      source: src,
      persistence: MemoryPersistence(),
    );
    addTearDown(c.dispose);
    await c.start();
    await tester.pumpWidget(QuickSettingsApp(controller: c));
    await tester.pumpAndSettle();

    // first large tile is autofocused; move right and activate
    await tester.sendKeyEvent(LogicalKeyboardKey.arrowRight);
    await tester.sendKeyEvent(LogicalKeyboardKey.enter);
    await tester.pump();
    expect(src.togglesRequested, [(ToggleId.boundary, true)]);
  });
}
