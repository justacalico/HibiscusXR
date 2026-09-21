import 'package:flutter/material.dart' show ThemeMode;
import 'package:flutter/widgets.dart';
import 'package:shared_preferences/shared_preferences.dart';

/// User-selectable site settings: theme mode and locale override.
///
/// Pure enough to test on the host - inject a [SharedPreferences] instance
/// (use SharedPreferences.setMockInitialValues in tests) and every method
/// runs without a binding.
class AppSettings extends ChangeNotifier {
  AppSettings(this._prefs);

  final SharedPreferences _prefs;

  static const _themeKey = 'theme_mode';
  static const _localeKey = 'locale';

  ThemeMode _themeMode = ThemeMode.system;
  Locale? _locale;

  ThemeMode get themeMode => _themeMode;

  /// Null means follow the browser locale.
  Locale? get locale => _locale;

  void load() {
    _themeMode = themeModeFromName(_prefs.getString(_themeKey));
    final code = _prefs.getString(_localeKey);
    _locale = code == null ? null : Locale(code);
    notifyListeners();
  }

  void setThemeMode(ThemeMode mode) {
    if (mode == _themeMode) return;
    _themeMode = mode;
    _prefs.setString(_themeKey, mode.name);
    notifyListeners();
  }

  void setLocale(Locale? locale) {
    if (locale == _locale) return;
    _locale = locale;
    if (locale == null) {
      _prefs.remove(_localeKey);
    } else {
      _prefs.setString(_localeKey, locale.languageCode);
    }
    notifyListeners();
  }

  static ThemeMode themeModeFromName(String? name) => switch (name) {
        'light' => ThemeMode.light,
        'dark' => ThemeMode.dark,
        _ => ThemeMode.system,
      };
}

/// Exposes [AppSettings] to the widget tree.
class AppSettingsScope extends InheritedNotifier<AppSettings> {
  const AppSettingsScope({
    super.key,
    required AppSettings settings,
    required super.child,
  }) : super(notifier: settings);

  static AppSettings of(BuildContext context) {
    final scope =
        context.dependOnInheritedWidgetOfExactType<AppSettingsScope>();
    assert(scope != null, 'AppSettingsScope missing from the tree');
    return scope!.notifier!;
  }
}
