import 'dart:ui' show Locale;

import 'package:flutter_test/flutter_test.dart';

import 'package:pn2_website/l10n/app_localizations.dart';
import 'package:pn2_website/src/repos.dart';

void main() {
  test('every repo name is unique and under the group', () {
    final names = repositories.map((r) => r.name).toList();
    expect(names.toSet().length, names.length);
    for (final entry in repositories) {
      expect(entry.url, 'https://gitlab.com/neosalsa/${entry.name}');
      expect(entry.label, entry.name.split('/').last);
    }
  });

  test('every group has at least one repo', () {
    for (final group in RepoGroup.values) {
      expect(reposIn(group), isNotEmpty, reason: group.name);
    }
  });

  test('every repo description resolves in both locales', () async {
    final en = await AppLocalizations.delegate.load(const Locale('en'));
    final zh = await AppLocalizations.delegate.load(const Locale('zh'));
    for (final entry in repositories) {
      expect(entry.describe(en), isNotEmpty, reason: entry.name);
      expect(entry.describe(zh), isNotEmpty, reason: entry.name);
    }
  });
}
