import 'package:flutter_test/flutter_test.dart';

import 'package:pn2_website/l10n/app_localizations_en.dart';
import 'package:pn2_website/l10n/app_localizations_zh.dart';
import 'package:pn2_website/src/library_mock.dart';

void main() {
  test('grid shows nine unique tiles, only the first pinned', () {
    final tiles = libraryTiles(AppLocalizationsEn());
    expect(tiles, hasLength(9));
    expect(tiles.map((t) => t.mark).toSet(), hasLength(9));
    expect(tiles.where((t) => t.pinned), hasLength(1));
    expect(tiles.first.pinned, isTrue);
  });

  test('tile names resolve in both locales', () {
    final en = libraryTiles(AppLocalizationsEn());
    final zh = libraryTiles(AppLocalizationsZh());
    for (var i = 0; i < en.length; i++) {
      expect(en[i].name, isNotEmpty);
      expect(zh[i].name, isNotEmpty);
    }
    expect(en.first.name, 'Calendar');
    expect(zh.first.name, '日历');
  });

  test('filter count is larger than the shown grid', () {
    expect(libraryTotal,
        greaterThan(libraryTiles(AppLocalizationsEn()).length));
  });
}
