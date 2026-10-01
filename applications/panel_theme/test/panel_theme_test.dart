import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:panel_theme/panel_theme.dart';

void main() {
  test('themeChoiceFromName parses the wire values, dark by default', () {
    expect(themeChoiceFromName('dark'), ThemeChoice.dark);
    expect(themeChoiceFromName('light'), ThemeChoice.light);
    expect(themeChoiceFromName('oled'), ThemeChoice.oled);
    expect(themeChoiceFromName(null), ThemeChoice.dark);
    expect(themeChoiceFromName(''), ThemeChoice.dark);
    expect(themeChoiceFromName('sepia'), ThemeChoice.dark);
  });

  test('kThemeChoices lists every option in picker order', () {
    expect(kThemeChoices,
        [ThemeChoice.dark, ThemeChoice.light, ThemeChoice.oled]);
  });

  test('every choice has a palette with matching brightness', () {
    for (final t in kThemeChoices) {
      final p = paletteFor(t);
      expect(
        p.brightness,
        t == ThemeChoice.light ? Brightness.light : Brightness.dark,
      );
    }
    expect(paletteFor(ThemeChoice.dark), same(kDarkPalette));
    expect(paletteFor(ThemeChoice.light), same(kLightPalette));
    expect(paletteFor(ThemeChoice.oled), same(kOledPalette));
  });
}
