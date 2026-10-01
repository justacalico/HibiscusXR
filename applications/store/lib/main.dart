import 'dart:io' show Directory;

import 'package:flutter/material.dart';

import 'l10n/app_localizations.dart';
import 'src/downloader.dart';
import 'src/installer.dart';
import 'src/platform/prefs_persistence.dart';
import 'src/repo_client.dart';
import 'src/store_controller.dart';
import 'src/ui/store_page.dart';
import 'src/ui/theme.dart';

/// The production wiring, split out so tests can drive the real
/// constructors on the host.
Future<StoreController> buildStoreController() async => StoreController(
      client: HttpRepoClient(),
      // APKs land in the app cache - they are staging files, the
      // installer consumes and the OS reclaims them.
      downloader: ApkDownloader(directory: Directory.systemTemp),
      installer: const ChannelInstaller(),
      persistence: await PrefsPersistence.open(),
    );

Future<void> main({StoreController? controller}) async {
  WidgetsFlutterBinding.ensureInitialized();
  final c = controller ?? await buildStoreController();
  // Fire and forget: the page renders the loading state and fills in
  // when the index arrives.
  c.start();
  runApp(StoreApp(controller: c));
}

class StoreApp extends StatelessWidget {
  const StoreApp({super.key, required this.controller});

  final StoreController controller;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      onGenerateTitle: (context) => AppLocalizations.of(context).appTitle,
      localizationsDelegates: AppLocalizations.localizationsDelegates,
      supportedLocales: AppLocalizations.supportedLocales,
      theme: StoreTheme.data(),
      debugShowCheckedModeBanner: false,
      home: StorePage(controller: controller),
    );
  }
}
