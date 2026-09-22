import 'package:flutter/material.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../theme.dart';
import '../shell.dart';
import '../widgets.dart';

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
              final wide = constraints.maxWidth >= 760;
              final half = (constraints.maxWidth - 32) / 2;
              return Column(
                children: [
                  Reveal(
                    child: ShotCard(
                      asset: shots[0].$1,
                      caption: shots[0].$2,
                    ),
                  ),
                  const SizedBox(height: 40),
                  Wrap(
                    spacing: 32,
                    runSpacing: 40,
                    children: [
                      for (var i = 1; i < shots.length; i++)
                        Reveal(
                          delay: Duration(milliseconds: i * 100),
                          child: SizedBox(
                            width: wide ? half : constraints.maxWidth,
                            child: ShotCard(
                              asset: shots[i].$1,
                              caption: shots[i].$2,
                            ),
                          ),
                        ),
                    ],
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
