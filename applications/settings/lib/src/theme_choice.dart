/// The three OS themes. The wire value is the enum name: this app writes
/// it into the hibiscus_theme Settings.Global key, pn2-themed mirrors that
/// onto persist.hibiscus.theme for the native chrome, and the other panel
/// apps read the same key back through their own channels.
enum ThemeChoice { dark, light, oled }

/// Every option in picker order.
const kThemeChoices = ThemeChoice.values;

/// Parse a stored or wire value. Missing and unrecognized names fall back
/// to dark, matching the pn2-themed daemon's default.
ThemeChoice themeChoiceFromName(String? name) => switch (name) {
  'light' => ThemeChoice.light,
  'oled' => ThemeChoice.oled,
  _ => ThemeChoice.dark,
};
