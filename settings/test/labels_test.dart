import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/l10n/app_localizations.dart';
import 'package:pn2_settings/src/labels.dart';
import 'package:pn2_settings/src/models.dart';

Future<AppLocalizations> l10nOf(WidgetTester tester) async {
  late AppLocalizations found;
  await tester.pumpWidget(MaterialApp(
    localizationsDelegates: AppLocalizations.localizationsDelegates,
    supportedLocales: AppLocalizations.supportedLocales,
    home: Builder(builder: (context) {
      found = AppLocalizations.of(context);
      return const SizedBox.shrink();
    }),
  ));
  return found;
}

void main() {
  testWidgets('every section and item resolves to a string', (tester) async {
    final l10n = await l10nOf(tester);
    await tester.pump();

    for (final s in SectionId.values) {
      expect(sectionTitle(l10n, s), isNotEmpty, reason: '$s');
    }
    for (final i in ItemId.values) {
      expect(itemTitle(l10n, i), isNotEmpty, reason: '$i title');
      expect(itemDescription(l10n, i), isNotEmpty, reason: '$i desc');
    }
  });

  testWidgets('choice labels cover all option values', (tester) async {
    final l10n = await l10nOf(tester);
    await tester.pump();
    expect(choiceLabel(l10n, 'auto'), 'Auto');
    expect(choiceLabel(l10n, '60hz'), '60 Hz');
    expect(choiceLabel(l10n, '50hz'), '50 Hz');
    expect(choiceLabel(l10n, 'left'), 'Left');
    expect(choiceLabel(l10n, 'right'), 'Right');
    expect(choiceLabel(l10n, 'anything'), 'Auto');
  });

  testWidgets('controller link labels cover every state', (tester) async {
    final l10n = await l10nOf(tester);
    await tester.pump();
    expect(controllerLinkLabel(l10n, ControllerLink.connected), 'Connected');
    expect(
      controllerLinkLabel(l10n, ControllerLink.disconnected),
      'Disconnected',
    );
    expect(controllerLinkLabel(l10n, ControllerLink.pairing), 'Pairing…');
    expect(
      controllerLinkLabel(l10n, ControllerLink.unknown),
      isNotEmpty,
    );
  });

  testWidgets('scan status label flips with the flag', (tester) async {
    final l10n = await l10nOf(tester);
    await tester.pump();
    expect(scanStatusLabel(l10n, true), 'Scanning for controllers…');
    expect(scanStatusLabel(l10n, false), 'Not scanning');
  });
}
