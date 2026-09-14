import 'package:flutter/material.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../../theme.dart';
import '../shell.dart';
import '../widgets.dart';
import 'features_page.dart' show PageHead;

class ScreenshotsPage extends StatelessWidget {
  const ScreenshotsPage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final shots = [
      ('assets/screenshots/library-grid.png', l10n.shotGridCaption),
      ('assets/screenshots/collection-menu.png', l10n.shotCollectionCaption),
      ('assets/screenshots/tile-menu.png', l10n.shotMenuCaption),
    ];
    return PageBody(
      children: [
        PageHead(
          title: l10n.screenshotsTitle,
          subtitle: l10n.screenshotsSubtitle,
        ),
        Band(
          color: context.colors.surfaceContainerHighest,
          padding: const EdgeInsets.symmetric(vertical: 72),
          child: LayoutBuilder(
            builder: (context, constraints) {
              final columns = constraints.maxWidth >= 900 ? 3 : 1;
              final width =
                  (constraints.maxWidth - (columns - 1) * 32) / columns;
              return Wrap(
                spacing: 32,
                runSpacing: 40,
                children: [
                  for (var i = 0; i < shots.length; i++)
                    Reveal(
                      delay: Duration(milliseconds: i * 100),
                      child: SizedBox(
                        width: width,
                        child: ShotCard(
                          asset: shots[i].$1,
                          caption: shots[i].$2,
                        ),
                      ),
                    ),
                ],
              );
            },
          ),
        ),
      ],
    );
  }
}
