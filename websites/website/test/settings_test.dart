import 'package:flutter/material.dart' show ThemeMode;
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:pn2_website/src/settings.dart';

Future<AppSettings> _settings([Map<String, Object> values = const {}]) async {
  SharedPreferences.setMockInitialValues(values);
  final prefs = await SharedPreferences.getInstance();
  return AppSettings(prefs)..load();
}

void main() {
  test('defaults to system theme and system locale', () async {
    final s = await _settings();
    expect(s.themeMode, ThemeMode.system);
    expect(s.locale, isNull);
  });

  test('restores persisted theme and locale', () async {
    final s = await _settings({'theme_mode': 'dark', 'locale': 'zh'});
    expect(s.themeMode, ThemeMode.dark);
    expect(s.locale, const Locale('zh'));
  });

  test('persists theme changes', () async {
    SharedPreferences.setMockInitialValues({});
    final prefs = await SharedPreferences.getInstance();
    final s = AppSettings(prefs)..load();
    s.setThemeMode(ThemeMode.light);
    expect(prefs.getString('theme_mode'), 'light');
    expect(AppSettings(prefs)..load(), isNotNull);
  });

  test('persists and clears locale', () async {
    SharedPreferences.setMockInitialValues({});
    final prefs = await SharedPreferences.getInstance();
    final s = AppSettings(prefs)..load();
    s.setLocale(const Locale('zh'));
    expect(prefs.getString('locale'), 'zh');
    s.setLocale(null);
    expect(prefs.getString('locale'), isNull);
  });

  test('notifies listeners on change, not on no-op', () async {
    final s = await _settings();
    var count = 0;
    s.addListener(() => count++);
    s.setThemeMode(ThemeMode.system);
    expect(count, 0);
    s.setThemeMode(ThemeMode.dark);
    expect(count, 1);
  });

  test('unknown theme names fall back to system', () {
    expect(AppSettings.themeModeFromName('neon'), ThemeMode.system);
    expect(AppSettings.themeModeFromName(null), ThemeMode.system);
    expect(AppSettings.themeModeFromName('light'), ThemeMode.light);
    expect(AppSettings.themeModeFromName('dark'), ThemeMode.dark);
  });
}
