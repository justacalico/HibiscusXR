import 'dart:io' show Platform;

import 'package:flutter/material.dart';

import 'l10n/app_localizations.dart';
import 'src/models.dart';
import 'src/platform/android_settings_source.dart';
import 'src/platform/fake_settings_source.dart';
import 'src/platform/prefs_persistence.dart';
import 'src/settings_controller.dart';
import 'src/ui/settings_page.dart';
import 'src/ui/theme.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final persistence = await PrefsPersistence.open();
  final controller = SettingsController(
    // Desktop builds are for UI development only: no Android channel
    // exists there, so the preview source stands in with seeded state.
    source: Platform.isAndroid
        ? AndroidSettingsSource()
        : FakeSettingsSource(
            initial: const SettingsSnapshot(
              toggles: {
                ItemId.wifiToggle: true,
                ItemId.bluetoothToggle: true,
              },
              sliders: {ItemId.volume: 0.6, ItemId.brightness: 0.8},
              texts: {ItemId.wifiSsid: 'dev-preview'},
            ),
          ),
    persistence: persistence,
  );
  // Fire and forget: the page renders with defaults and fills in as
  // the platform snapshot arrives.
  controller.start();
  runApp(
    SettingsApp(controller: controller, uiOnlyMode: !Platform.isAndroid),
  );
}

class SettingsApp extends StatelessWidget {
  const SettingsApp({
    super.key,
    required this.controller,
    this.uiOnlyMode = false,
  });

  final SettingsController controller;
  final bool uiOnlyMode;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      onGenerateTitle: (context) => AppLocalizations.of(context).appTitle,
      localizationsDelegates: AppLocalizations.localizationsDelegates,
      supportedLocales: AppLocalizations.supportedLocales,
      theme: PanelTheme.data(),
      debugShowCheckedModeBanner: false,
      home: SettingsPage(controller: controller, uiOnlyMode: uiOnlyMode),
    );
  }
}
