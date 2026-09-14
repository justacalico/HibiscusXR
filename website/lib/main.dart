import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'l10n/app_localizations.dart';
import 'src/router.dart';
import 'src/settings.dart';
import 'src/theme.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final prefs = await SharedPreferences.getInstance();
  runApp(SiteApp(settings: AppSettings(prefs)..load()));
}

class SiteApp extends StatefulWidget {
  const SiteApp({super.key, required this.settings});

  final AppSettings settings;

  @override
  State<SiteApp> createState() => _SiteAppState();
}

class _SiteAppState extends State<SiteApp> {
  late final _router = buildRouter();

  @override
  Widget build(BuildContext context) {
    return AppSettingsScope(
      settings: widget.settings,
      child: ListenableBuilder(
        listenable: widget.settings,
        builder: (context, _) => MaterialApp.router(
          title: 'Library',
          onGenerateTitle: (context) => AppLocalizations.of(context).appTitle,
          debugShowCheckedModeBanner: false,
          theme: lightTheme(),
          darkTheme: darkTheme(),
          themeMode: widget.settings.themeMode,
          locale: widget.settings.locale,
          localizationsDelegates: AppLocalizations.localizationsDelegates,
          supportedLocales: AppLocalizations.supportedLocales,
          routerConfig: _router,
        ),
      ),
    );
  }
}
