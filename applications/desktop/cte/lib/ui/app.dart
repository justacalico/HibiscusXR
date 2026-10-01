import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';

import '../l10n/app_localizations.dart';
import '../src/app_state.dart';
import '../src/theme.dart';
import 'connect_page.dart';
import 'home_shell.dart';

class CteApp extends StatelessWidget {
  const CteApp({super.key, required this.state, this.theme});

  final AppState state;

  /// Override point for golden tests - real fonts need a loaded family.
  final ThemeData? theme;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'HCTE',
      debugShowCheckedModeBanner: false,
      theme: theme ?? CteTheme.dark(),
      localizationsDelegates: const [
        AppLocalizations.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
      ],
      supportedLocales: AppLocalizations.supportedLocales,
      home: ListenableBuilder(
        listenable: state,
        builder: (context, _) => AnimatedSwitcher(
          duration: const Duration(milliseconds: 200),
          child: state.connected
              ? HomeShell(state: state, key: const ValueKey('shell'))
              : ConnectPage(state: state, key: const ValueKey('connect')),
        ),
      ),
    );
  }
}
