/// The three OS themes. The wire value is the enum name: the settings
/// app writes it into the hibiscus_theme Settings.Global key, pn2-themed
/// mirrors that onto persist.hibiscus.theme for the native chrome, and
/// this panel reads the same key back through its own channel.
enum ThemeChoice { dark, light, oled }

/// Parse a stored or wire value. Missing and unrecognized names fall
/// back to dark, matching the pn2-themed daemon's default.
ThemeChoice themeChoiceFromName(String? name) => switch (name) {
  'light' => ThemeChoice.light,
  'oled' => ThemeChoice.oled,
  _ => ThemeChoice.dark,
};
