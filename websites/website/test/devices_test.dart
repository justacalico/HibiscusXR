import 'dart:ui' show Locale;

import 'package:flutter_test/flutter_test.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';
import 'package:hibiscusxr_website/src/devices.dart';

void main() {
  test('the supported device leads the list', () {
    expect(devices, isNotEmpty);
    expect(devices.first.status, DeviceStatus.supported);
    expect(
      devices.where((d) => d.status == DeviceStatus.supported),
      hasLength(1),
    );
  });

  test('every device resolves in both locales', () async {
    final en = await AppLocalizations.delegate.load(const Locale('en'));
    final zh = await AppLocalizations.delegate.load(const Locale('zh'));
    for (final d in devices) {
      for (final l10n in [en, zh]) {
        expect(d.name(l10n), isNotEmpty);
        expect(d.specs(l10n), isNotEmpty);
        expect(d.describe(l10n), isNotEmpty);
        expect(d.statusLabel(l10n), isNotEmpty);
      }
    }
  });
}
