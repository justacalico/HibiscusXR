import 'package:flutter/material.dart';

/// Dark theme tuned to read like a Quest-style panel: deep blue-grey
/// background, soft pill controls, one cool accent.
class LibraryTheme {
  static const background = Color(0xFF141A21);
  static const panel = Color(0xFF1B232D);
  static const surface = Color(0xFF232D38);
  static const surfaceHigh = Color(0xFF2E3A47);
  static const accent = Color(0xFF4E9CFF);
  static const danger = Color(0xFFFF5E5E);
  static const textPrimary = Color(0xFFF2F5F8);
  static const textSecondary = Color(0xFF9AA7B4);

  static const tileRadius = 18.0;
  static const pillRadius = 24.0;

  static ThemeData data() {
    final base = ThemeData.dark(useMaterial3: true);
    return base.copyWith(
      scaffoldBackgroundColor: background,
      colorScheme: const ColorScheme.dark(
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
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(14),
        ),
        labelTextStyle: WidgetStateProperty.all(
          const TextStyle(color: textPrimary, fontSize: 14),
        ),
      ),
      dialogTheme: DialogThemeData(
        backgroundColor: surface,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(20),
        ),
        titleTextStyle: const TextStyle(
          color: textPrimary,
          fontSize: 18,
          fontWeight: FontWeight.w600,
        ),
      ),
      snackBarTheme: const SnackBarThemeData(
        backgroundColor: surfaceHigh,
        contentTextStyle: TextStyle(color: textPrimary),
        behavior: SnackBarBehavior.floating,
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: surface,
        hintStyle: const TextStyle(color: textSecondary),
        contentPadding:
            const EdgeInsets.symmetric(horizontal: 20, vertical: 14),
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(pillRadius),
          borderSide: BorderSide.none,
        ),
      ),
    );
  }
}
