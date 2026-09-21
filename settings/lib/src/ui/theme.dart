import 'package:flutter/material.dart';

/// Dark theme matching the project's panel apps: deep blue-grey
/// surfaces, one cool accent, flat rows separated by thin dividers.
class PanelTheme {
  static const background = Color(0xFF141A21);
  static const panel = Color(0xFF1B232D);
  static const surface = Color(0xFF232D38);
  static const surfaceHigh = Color(0xFF2E3A47);
  static const accent = Color(0xFF4E9CFF);
  static const textPrimary = Color(0xFFF2F5F8);
  static const textSecondary = Color(0xFF9AA7B4);

  static const panelRadius = 28.0;

  static ThemeData data() {
    final base = ThemeData.dark(useMaterial3: true);
    return base.copyWith(
      scaffoldBackgroundColor: background,
      colorScheme: const ColorScheme.dark(
        surface: panel,
        primary: accent,
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
