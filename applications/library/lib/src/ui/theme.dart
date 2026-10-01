import 'package:flutter/material.dart';

import 'package:panel_theme/panel_theme.dart';

/// Panel chrome colors. Every widget reads through these getters, which
/// resolve against [palette] at build time: when the theme setting moves
/// the app shell swaps the palette and rebuilds, the same way the native
/// chrome repoints its kPal* tables in syncPalette().
class LibraryTheme {
  static PanelPalette palette = kDarkPalette;

  static Color get background => palette.background;
  static Color get panel => palette.panel;
  static Color get surface => palette.surface;
  static Color get surfaceHigh => palette.surfaceHigh;
  static Color get accent => palette.accent;
  static Color get danger => palette.danger;
  static Color get warn => palette.warn;
  static Color get good => palette.good;
  static Color get textPrimary => palette.textPrimary;
  static Color get textSecondary => palette.textSecondary;

  static const tileRadius = 18.0;
  static const pillRadius = 24.0;

  static ThemeData data() {
    final dark = palette.brightness == Brightness.dark;
    final base = dark
        ? ThemeData.dark(useMaterial3: true)
        : ThemeData.light(useMaterial3: true);
    return base.copyWith(
      scaffoldBackgroundColor: background,
      colorScheme:
          (dark ? const ColorScheme.dark() : const ColorScheme.light())
              .copyWith(
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
      popupMenuTheme: PopupMenuThemeData(
        color: surface,
        elevation: 12,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
        labelTextStyle: WidgetStateProperty.all(
          TextStyle(color: textPrimary, fontSize: 14),
        ),
      ),
      dialogTheme: DialogThemeData(
        backgroundColor: surface,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
        titleTextStyle: TextStyle(
          color: textPrimary,
          fontSize: 18,
          fontWeight: FontWeight.w600,
        ),
      ),
      snackBarTheme: SnackBarThemeData(
        backgroundColor: surfaceHigh,
        contentTextStyle: TextStyle(color: textPrimary),
        behavior: SnackBarBehavior.floating,
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: surface,
        hintStyle: TextStyle(color: textSecondary),
        contentPadding: const EdgeInsets.symmetric(
          horizontal: 20,
          vertical: 14,
        ),
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(pillRadius),
          borderSide: BorderSide.none,
        ),
      ),
    );
  }
}
