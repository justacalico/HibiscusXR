import 'package:flutter/material.dart';

/// Dark-first, restrained. Same family as HCTE, colder accent - this app
/// moves bytes, not pixels.
abstract final class HbsupTheme {
  static const accent = Color(0xFF4EC9A6);
  static const accentSoft = Color(0x334EC9A6);
  static const bad = Color(0xFFE05A6E);
  static const warn = Color(0xFFE0B45A);

  static ThemeData dark() {
    final scheme = ColorScheme.fromSeed(
      seedColor: accent,
      brightness: Brightness.dark,
      surface: const Color(0xFF161A18),
    );
    return ThemeData(
      useMaterial3: true,
      colorScheme: scheme,
      scaffoldBackgroundColor: const Color(0xFF101412),
      cardTheme: CardThemeData(
        color: const Color(0xFF1B201E),
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(14),
          side: const BorderSide(color: Color(0xFF2A3330), width: 0.5),
        ),
      ),
      snackBarTheme:
          const SnackBarThemeData(behavior: SnackBarBehavior.floating),
    );
  }
}
