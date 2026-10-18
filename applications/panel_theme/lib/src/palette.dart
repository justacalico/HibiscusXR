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
const kPanelDarkBackground = Color(0xFF131318);
const kPanelDarkPanel = Color(0xFF38383F);
const kPanelDarkSurface = Color(0xFF4A4A52);
const kPanelDarkSurfaceHigh = Color(0xFF5D5D66);
const kPanelDarkAccent = Color(0xFF0A84FF);
const kPanelDarkDanger = Color(0xFFFF453A);
const kPanelDarkWarn = Color(0xFFFF9F0A);
const kPanelDarkGood = Color(0xFF30D158);
const kPanelDarkTextPrimary = Color(0xFFF5F5F7);
const kPanelDarkTextSecondary = Color(0xFF9898A0);

/// The panel look: neutral graphite surfaces under one system blue.
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
  panel: Color(0xFF1E1E22),
  surface: Color(0xFF2B2B31),
  surfaceHigh: Color(0xFF3D3D44),
  accent: Color(0xFF0A84FF),
  danger: Color(0xFFFF453A),
  warn: Color(0xFFFF9F0A),
  good: Color(0xFF30D158),
  textPrimary: Color(0xFFF5F5F7),
  textSecondary: Color(0xFF9898A0),
);

/// Light theme: the ramp inverts, so the void sits a shade under the
/// furniture and cards go white. Accent and status hues darken to keep
/// their contrast on white.
const kLightPalette = PanelPalette(
  brightness: Brightness.light,
  background: Color(0xFFD8D8DE),
  panel: Color(0xFFF4F4F6),
  surface: Color(0xFFFFFFFF),
  surfaceHigh: Color(0xFFC9C9CF),
  accent: Color(0xFF007AFF),
  danger: Color(0xFFFF3B30),
  warn: Color(0xFFFF9500),
  good: Color(0xFF34C759),
  textPrimary: Color(0xFF1C1C1E),
  textSecondary: Color(0xFF6E6E73),
);

/// The palette for one picker option.
PanelPalette paletteFor(ThemeChoice choice) => switch (choice) {
  ThemeChoice.light => kLightPalette,
  ThemeChoice.oled => kOledPalette,
  ThemeChoice.dark => kDarkPalette,
};
