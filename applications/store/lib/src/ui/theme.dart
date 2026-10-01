import 'package:flutter/material.dart';
import 'package:panel_theme/panel_theme.dart';

/// Store chrome colors. The dark table the other panel apps share - the
/// store has no light mode yet, and these stay consts because the
/// widgets read them inside const styles.
class StoreTheme {
  static const background = kPanelDarkBackground;
  static const panel = kPanelDarkPanel;
  static const surface = kPanelDarkSurface;
  static const surfaceHigh = kPanelDarkSurfaceHigh;
  static const accent = kPanelDarkAccent;
  static const danger = kPanelDarkDanger;
  static const warn = kPanelDarkWarn;
  static const good = kPanelDarkGood;
  static const textPrimary = kPanelDarkTextPrimary;
  static const textSecondary = kPanelDarkTextSecondary;

  static const cardRadius = 18.0;

  static ThemeData data() {
    final base = ThemeData.dark(useMaterial3: true);
    return base.copyWith(
      scaffoldBackgroundColor: background,
      colorScheme: ColorScheme.dark().copyWith(
        surface: panel,
        primary: accent,
        error: danger,
        onSurface: textPrimary,
      ),
      textTheme: base.textTheme.apply(
        bodyColor: textPrimary,
        displayColor: textPrimary,
      ),
      dividerColor: surfaceHigh,
    );
  }
}
