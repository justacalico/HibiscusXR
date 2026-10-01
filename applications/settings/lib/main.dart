import 'dart:io' show Platform;

import 'package:flutter/material.dart';

import 'l10n/app_localizations.dart';
import 'src/envs/env_source.dart';
import 'src/models.dart';
import 'src/platform/android_settings_source.dart';
import 'src/platform/fake_settings_source.dart';
import 'src/platform/prefs_persistence.dart';
import 'src/settings_controller.dart';
import 'package:panel_theme/panel_theme.dart';
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
              wifi: [
                WifiNetwork(
                  ssid: 'dev-preview',
                  capabilities: '[WPA2-PSK-CCMP]',
                  level: 3,
                  connected: true,
                  savedId: 1,
                ),
                WifiNetwork(ssid: 'cafe-open', capabilities: '[ESS]'),
              ],
              bt: [
                BtDevice(
                  name: 'dev buds',
                  address: 'DE:AD:00:00:00:01',
                  bonded: true,
                ),
              ],
              imes: [
                ImeOption(
                  id: 'com.android.inputmethod.latin/.LatinIME',
                  label: 'Android Keyboard',
                  active: true,
                ),
              ],
            ),
          ),
    persistence: persistence,
    // environment zips live on shared storage - nothing to scan off-device
    envs: Platform.isAndroid ? DirEnvSource() : const EmptyEnvSource(),
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
    // The theme picker lands in the store as a plain text row; rebuild
    // the whole app on any store change and swap the palette up front
    // so every PanelTheme getter resolves against it.
    return AnimatedBuilder(
      animation: controller.store,
      builder: (context, _) {
        PanelTheme.palette = paletteFor(
          themeChoiceFromName(controller.store.textOf(ItemId.themeMode)),
        );
        return MaterialApp(
          onGenerateTitle: (context) =>
              AppLocalizations.of(context).appTitle,
          localizationsDelegates: AppLocalizations.localizationsDelegates,
          supportedLocales: AppLocalizations.supportedLocales,
          theme: PanelTheme.data(),
          debugShowCheckedModeBanner: false,
          home: SettingsPage(
            controller: controller,
            uiOnlyMode: uiOnlyMode,
          ),
        );
      },
    );
  }
}
