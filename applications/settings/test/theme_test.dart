import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:panel_theme/panel_theme.dart';
import 'package:pn2_settings/src/ui/theme.dart';

void main() {
  test('themeChoiceFromName parses the wire values, dark by default', () {
    expect(themeChoiceFromName('dark'), ThemeChoice.dark);
    expect(themeChoiceFromName('light'), ThemeChoice.light);
    expect(themeChoiceFromName('oled'), ThemeChoice.oled);
    expect(themeChoiceFromName(null), ThemeChoice.dark);
    expect(themeChoiceFromName(''), ThemeChoice.dark);
    expect(themeChoiceFromName('sepia'), ThemeChoice.dark);
  });

  test('every choice has a palette with matching brightness', () {
    for (final t in kThemeChoices) {
      final p = paletteFor(t);
      expect(
        p.brightness,
        t == ThemeChoice.light ? Brightness.light : Brightness.dark,
        reason: '$t',
      );
    }
    expect(paletteFor(ThemeChoice.dark), same(kDarkPalette));
    expect(paletteFor(ThemeChoice.light), same(kLightPalette));
    expect(paletteFor(ThemeChoice.oled), same(kOledPalette));
  });

  test('oled keeps the dark hues on a true-black ramp', () {
    expect(kOledPalette.background, const Color(0xFF000000));
    expect(kOledPalette.accent, kDarkPalette.accent);
    expect(kOledPalette.textPrimary, kDarkPalette.textPrimary);
    // the ramp still climbs: panel over background, surface over panel
    expect(
      kOledPalette.panel.computeLuminance(),
      greaterThan(kOledPalette.background.computeLuminance()),
    );
    expect(
      kOledPalette.surfaceHigh.computeLuminance(),
      greaterThan(kOledPalette.surface.computeLuminance()),
    );
  });

  test('PanelTheme getters follow the swapped palette', () {
    PanelTheme.palette = kLightPalette;
    addTearDown(() => PanelTheme.palette = kDarkPalette);

    expect(PanelTheme.background, kLightPalette.background);
    expect(PanelTheme.accent, kLightPalette.accent);

    final data = PanelTheme.data();
    expect(data.brightness, Brightness.light);
    expect(data.scaffoldBackgroundColor, kLightPalette.background);
    expect(data.colorScheme.primary, kLightPalette.accent);

    PanelTheme.palette = kOledPalette;
    expect(PanelTheme.background, const Color(0xFF000000));
    expect(PanelTheme.data().brightness, Brightness.dark);
  });
}
