import 'package:flutter/material.dart';

import '../theme_choice.dart';

/// One palette per OS theme choice. These values mirror the tables in
/// applications/vrhome/src/common/palette.cpp - the HUD chrome blends to
/// the same code values a panel pixel shows - and the copies in the
/// other panel apps' lib/src/ui/theme.dart.
class PanelPalette {
  const PanelPalette({
    required this.brightness,
    required this.background,
    required this.panel,
    required this.surface,
    required this.surfaceHigh,
    required this.accent,
    required this.danger,
    required this.warn,
    required this.good,
    required this.textPrimary,
    required this.textSecondary,
  });

  final Brightness brightness;
  final Color background;
  final Color panel;
  final Color surface;
  final Color surfaceHigh;
  final Color accent;
  final Color danger;
  final Color warn;
  final Color good;
  final Color textPrimary;
  final Color textSecondary;
}

/// The long-standing panel look: deep blue-grey background, soft pill
/// controls, one cool accent.
const kDarkPalette = PanelPalette(
  brightness: Brightness.dark,
  background: Color(0xFF141A21),
  panel: Color(0xFF1B232D),
  surface: Color(0xFF232D38),
  surfaceHigh: Color(0xFF2E3A47),
  accent: Color(0xFF4E9CFF),
  danger: Color(0xFFFF5E5E),
  warn: Color(0xFFF5C542),
  good: Color(0xFF3DD68C),
  textPrimary: Color(0xFFF2F5F8),
  textSecondary: Color(0xFF9AA7B4),
);

/// The dark ramp shifted onto true black for OLED panels.
const kOledPalette = PanelPalette(
  brightness: Brightness.dark,
  background: Color(0xFF000000),
  panel: Color(0xFF0B0F13),
  surface: Color(0xFF151A20),
  surfaceHigh: Color(0xFF212A33),
  accent: Color(0xFF4E9CFF),
  danger: Color(0xFFFF5E5E),
  warn: Color(0xFFF5C542),
  good: Color(0xFF3DD68C),
  textPrimary: Color(0xFFF2F5F8),
  textSecondary: Color(0xFF9AA7B4),
);

/// Light theme: the ramp inverts, so the void sits a shade under the
/// furniture and cards go white. Accent and status hues darken to keep
/// their contrast on white.
const kLightPalette = PanelPalette(
  brightness: Brightness.light,
  background: Color(0xFFDDE3EA),
  panel: Color(0xFFE9EEF4),
  surface: Color(0xFFFFFFFF),
  surfaceHigh: Color(0xFFC7D0DB),
  accent: Color(0xFF1C6DD9),
  danger: Color(0xFFD93636),
  warn: Color(0xFF8F6400),
  good: Color(0xFF1E9E5A),
  textPrimary: Color(0xFF141A21),
  textSecondary: Color(0xFF4E5A66),
);

/// The palette for one picker option.
PanelPalette paletteFor(ThemeChoice choice) => switch (choice) {
  ThemeChoice.light => kLightPalette,
  ThemeChoice.oled => kOledPalette,
  ThemeChoice.dark => kDarkPalette,
};

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
