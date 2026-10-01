import 'package:flutter/material.dart';

import 'theme_choice.dart';

/// One palette per OS theme choice. These values mirror the tables in
/// applications/vrhome/src/common/palette.cpp - the HUD chrome blends to
/// the same code values a panel pixel shows.
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

/// The dark ramp's raw values. The palette composes them; code in const
/// contexts (const TextStyle and friends) reads them directly.
const kPanelDarkBackground = Color(0xFF141A21);
const kPanelDarkPanel = Color(0xFF1B232D);
const kPanelDarkSurface = Color(0xFF232D38);
const kPanelDarkSurfaceHigh = Color(0xFF2E3A47);
const kPanelDarkAccent = Color(0xFF4E9CFF);
const kPanelDarkDanger = Color(0xFFFF5E5E);
const kPanelDarkWarn = Color(0xFFF5C542);
const kPanelDarkGood = Color(0xFF3DD68C);
const kPanelDarkTextPrimary = Color(0xFFF2F5F8);
const kPanelDarkTextSecondary = Color(0xFF9AA7B4);

/// The long-standing panel look: deep blue-grey surfaces, one cool
/// accent.
const kDarkPalette = PanelPalette(
  brightness: Brightness.dark,
  background: kPanelDarkBackground,
  panel: kPanelDarkPanel,
  surface: kPanelDarkSurface,
  surfaceHigh: kPanelDarkSurfaceHigh,
  accent: kPanelDarkAccent,
  danger: kPanelDarkDanger,
  warn: kPanelDarkWarn,
  good: kPanelDarkGood,
  textPrimary: kPanelDarkTextPrimary,
  textSecondary: kPanelDarkTextSecondary,
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
