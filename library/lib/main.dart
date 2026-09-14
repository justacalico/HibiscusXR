import 'package:flutter/material.dart';

import 'l10n/app_localizations.dart';
import 'src/library_controller.dart';
import 'src/platform/android_app_source.dart';
import 'src/platform/prefs_persistence.dart';
import 'src/ui/library_page.dart';
import 'src/ui/theme.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final persistence = await PrefsPersistence.open();
  runApp(LibraryApp(
    controller: LibraryController(
      source: AndroidAppSource(),
      persistence: persistence,
    ),
  ));
}

class LibraryApp extends StatelessWidget {
  const LibraryApp({super.key, required this.controller});

  final LibraryController controller;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      onGenerateTitle: (context) => AppLocalizations.of(context).appTitle,
      localizationsDelegates: AppLocalizations.localizationsDelegates,
      supportedLocales: AppLocalizations.supportedLocales,
      theme: LibraryTheme.data(),
      debugShowCheckedModeBanner: false,
      home: LibraryPage(controller: controller),
    );
  }
}
