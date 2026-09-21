import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/l10n/app_localizations_en.dart';
import 'package:pn2_quicksettings/src/catalog.dart';
import 'package:pn2_quicksettings/src/labels.dart';
import 'package:pn2_quicksettings/src/models.dart';

void main() {
  final l10n = AppLocalizationsEn();

  test('every tile resolves a non-empty label', () {
    for (final spec in panelTiles) {
      expect(tileLabel(spec, l10n), isNotEmpty, reason: '${spec.toggleId}');
    }
  });

  test('subtitle kinds map to strings', () {
    expect(
      subtitleText(const TileSubtitle(SubtitleKind.on), l10n),
      'On',
    );
    expect(
      subtitleText(const TileSubtitle(SubtitleKind.off), l10n),
      'Off',
    );
    expect(
      subtitleText(const TileSubtitle(SubtitleKind.notConnected), l10n),
      'Not Connected',
    );
    expect(
      subtitleText(const TileSubtitle(SubtitleKind.custom, 'ap'), l10n),
      'ap',
    );
    expect(
      subtitleText(const TileSubtitle(SubtitleKind.none), l10n),
      '',
    );
  });
}
