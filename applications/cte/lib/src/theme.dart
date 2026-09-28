import 'package:flutter/material.dart';

/// Dark-first, restrained. One accent hue pulled from the hibiscus mark.
abstract final class CteTheme {
  static const accent = Color(0xFFE05A6E);
  static const accentSoft = Color(0x33E05A6E);

  static ThemeData dark() {
    final scheme = ColorScheme.fromSeed(
      seedColor: accent,
      brightness: Brightness.dark,
      surface: const Color(0xFF16161A),
    );
    return ThemeData(
      useMaterial3: true,
      colorScheme: scheme,
      scaffoldBackgroundColor: const Color(0xFF101014),
      cardTheme: CardThemeData(
        color: const Color(0xFF1B1B21),
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(14),
          side: const BorderSide(color: Color(0xFF2A2A33), width: 0.5),
        ),
      ),
      navigationRailTheme: NavigationRailThemeData(
        backgroundColor: const Color(0xFF141418),
        indicatorColor: accentSoft,
        selectedIconTheme: const IconThemeData(color: accent),
        unselectedIconTheme:
            const IconThemeData(color: Color(0xFF8A8A96)),
      ),
      snackBarTheme: const SnackBarThemeData(behavior: SnackBarBehavior.floating),
    );
  }
}
