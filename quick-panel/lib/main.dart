import 'package:flutter/material.dart';

import 'l10n/app_localizations.dart';
import 'src/platform/android_settings_source.dart';
import 'src/platform/prefs_persistence.dart';
import 'src/settings_controller.dart';
import 'src/ui/quick_settings_page.dart';
import 'src/ui/theme.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final persistence = await PrefsPersistence.open();
  final controller = SettingsController(
    source: AndroidSettingsSource(),
    persistence: persistence,
  );
  // Fire and forget: the panel renders with defaults and fills in as
  // the platform snapshot arrives.
  controller.start();
  runApp(QuickSettingsApp(controller: controller));
}

class QuickSettingsApp extends StatelessWidget {
  const QuickSettingsApp({super.key, required this.controller});

  final SettingsController controller;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      onGenerateTitle: (context) => AppLocalizations.of(context).appTitle,
      localizationsDelegates: AppLocalizations.localizationsDelegates,
      supportedLocales: AppLocalizations.supportedLocales,
      theme: PanelTheme.data(),
      debugShowCheckedModeBanner: false,
      home: QuickSettingsPage(controller: controller),
    );
  }
}
