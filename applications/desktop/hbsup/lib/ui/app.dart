import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';

import '../l10n/app_localizations.dart';
import '../src/app_state.dart';
import '../src/theme.dart';
import 'backup_page.dart';
import 'connect_page.dart';

class HbsupApp extends StatelessWidget {
  const HbsupApp({super.key, required this.state, this.locale, this.theme});

  final AppState state;

  /// Locale override for tests - null follows the system.
  final Locale? locale;

  /// Theme override for tests - null uses the dark theme.
  final ThemeData? theme;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'HBSUP',
      locale: locale,
      debugShowCheckedModeBanner: false,
      theme: theme ?? HbsupTheme.dark(),
      localizationsDelegates: const [
        AppLocalizations.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
      ],
      supportedLocales: AppLocalizations.supportedLocales,
      home: ListenableBuilder(
        listenable: state,
        builder: (context, _) => Column(
          children: [
            if (state.unsupportedHost && !state.warningDismissed)
              _UnsupportedBanner(state: state),
            Expanded(
              child: AnimatedSwitcher(
                duration: const Duration(milliseconds: 200),
                child: state.connected
                    ? BackupPage(state: state, key: const ValueKey('backup'))
                    : ConnectPage(state: state, key: const ValueKey('connect')),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

/// Persistent top banner on Windows - builds ship, but the host OS is
/// not supported, so say so on open instead of pretending.
class _UnsupportedBanner extends StatelessWidget {
  const _UnsupportedBanner({required this.state});

  final AppState state;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Material(
      color: HbsupTheme.warn.withValues(alpha: 0.14),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 10),
        child: Row(
          children: [
            const Icon(Icons.warning_amber_rounded,
                size: 18, color: HbsupTheme.warn),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(l10n.unsupportedHostTitle,
                      style: const TextStyle(
                          fontWeight: FontWeight.w600, fontSize: 13)),
                  Text(l10n.unsupportedHostBody,
                      style: const TextStyle(fontSize: 12, height: 1.35)),
                ],
              ),
            ),
            TextButton(
              onPressed: state.dismissWarning,
              child: Text(l10n.unsupportedHostDismiss),
            ),
          ],
        ),
      ),
    );
  }
}
