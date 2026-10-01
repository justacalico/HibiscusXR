import 'package:flutter/material.dart';

/// Store chrome colors. Same palette the other panel apps share - the
/// values mirror applications/vrhome/src/common/palette.cpp so a panel
/// pixel and the native chrome stay in the same family.
class StoreTheme {
  static const background = Color(0xFF141A21);
  static const panel = Color(0xFF1B232D);
  static const surface = Color(0xFF232D38);
  static const surfaceHigh = Color(0xFF2E3A47);
  static const accent = Color(0xFF4E9CFF);
  static const danger = Color(0xFFFF5E5E);
  static const warn = Color(0xFFF5C542);
  static const good = Color(0xFF3DD68C);
  static const textPrimary = Color(0xFFF2F5F8);
  static const textSecondary = Color(0xFF9AA7B4);

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
