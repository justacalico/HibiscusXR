import 'package:flutter/material.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../../theme.dart';
import '../shell.dart';
import '../widgets.dart';

class FeaturesPage extends StatelessWidget {
  const FeaturesPage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return PageBody(
      children: [
        _PageHead(title: l10n.featuresTitle, subtitle: l10n.featuresSubtitle),
        Band(
          color: context.colors.surfaceContainerHighest,
          padding: const EdgeInsets.only(top: 64, bottom: 96),
          child: _Lead(l10n: l10n),
        ),
        _Group(
          title: l10n.featFindTitle,
          items: [
            (l10n.featSearchTitle, l10n.featSearchBody),
            (l10n.featCollectionsTitle, l10n.featCollectionsBody),
            (l10n.featSortTitle, l10n.featSortBody),
          ],
        ),
        _Group(
          title: l10n.featArrangeTitle,
          band: true,
          items: [
            (l10n.featPinTitle, l10n.featPinBody),
            (l10n.featGroupsTitle, l10n.featGroupsBody),
          ],
        ),
        _Group(
          title: l10n.featControlTitle,
          items: [
            (l10n.featMenuTitle, l10n.featMenuBody),
            (l10n.featInstallTitle, l10n.featInstallBody),
          ],
        ),
        _Group(
          title: l10n.featNavigateTitle,
          band: true,
          items: [
            (l10n.featDpadTitle, l10n.featDpadBody),
            (l10n.featLiveTitle, l10n.featLiveBody),
            (l10n.featI18nTitle, l10n.featI18nBody),
          ],
        ),
      ],
    );
  }
}

/// Centred page masthead shared by the subpages.
class PageHead extends StatelessWidget {
  const PageHead({super.key, required this.title, required this.subtitle});

  final String title;
  final String subtitle;

  @override
  Widget build(BuildContext context) {
    return _PageHead(title: title, subtitle: subtitle);
  }
}

class _PageHead extends StatelessWidget {
  const _PageHead({required this.title, required this.subtitle});

  final String title;
  final String subtitle;

  @override
  Widget build(BuildContext context) {
    final mobile = Layout.isMobile(context);
    return Band(
      padding: EdgeInsets.only(
        top: mobile ? 56 : 96,
        bottom: mobile ? 40 : 56,
      ),
      child: Column(
        children: [
          Reveal(
            child: Text(
              title,
              textAlign: TextAlign.center,
              style: context.text.displayLarge!
                  .copyWith(fontSize: mobile ? 48 : 80),
            ),
          ),
          const SizedBox(height: 16),
          Reveal(
            delay: const Duration(milliseconds: 100),
            child: Text(
              subtitle,
              textAlign: TextAlign.center,
              style: context.text.bodyLarge!.copyWith(
                fontSize: 21,
                color: context.colors.secondary,
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _Lead extends StatelessWidget {
  const _Lead({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final mobile = Layout.isMobile(context);
    return Column(
      crossAxisAlignment:
          mobile ? CrossAxisAlignment.start : CrossAxisAlignment.center,
      children: [
        Reveal(
          child: Text(
            l10n.featGridTitle,
            textAlign: mobile ? TextAlign.start : TextAlign.center,
            style: context.text.headlineMedium!
                .copyWith(fontSize: mobile ? 30 : 40),
          ),
        ),
        const SizedBox(height: 16),
        Reveal(
          child: ConstrainedBox(
            constraints: const BoxConstraints(maxWidth: Layout.text),
            child: Text(
              l10n.featGridBody,
              textAlign: mobile ? TextAlign.start : TextAlign.center,
              style: context.text.bodyLarge!
                  .copyWith(color: context.colors.secondary),
            ),
          ),
        ),
        const SizedBox(height: 48),
        Reveal(
          child: Container(
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: context.colors.outline, width: 1),
            ),
            clipBehavior: Clip.antiAlias,
            child: Image.asset(
              'assets/screenshots/collection-menu.png',
              fit: BoxFit.cover,
            ),
          ),
        ),
      ],
    );
  }
}

class _Group extends StatelessWidget {
  const _Group({
    required this.title,
    required this.items,
    this.band = false,
  });

  final String title;
  final List<(String, String)> items;
  final bool band;

  @override
  Widget build(BuildContext context) {
    return Band(
      color: band ? context.colors.surfaceContainerHighest : null,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Reveal(
            child: Text(
              title,
              style: context.text.headlineMedium!.copyWith(
                fontSize: Layout.isMobile(context) ? 30 : 40,
              ),
            ),
          ),
          const SizedBox(height: 40),
          LayoutBuilder(
            builder: (context, constraints) {
              final columns = constraints.maxWidth >= 760 ? items.length : 1;
              final width =
                  (constraints.maxWidth - (columns - 1) * 48) / columns;
              return Wrap(
                spacing: 48,
                runSpacing: 40,
                children: [
                  for (var i = 0; i < items.length; i++)
                    Reveal(
                      delay: Duration(milliseconds: i * 80),
                      child: SizedBox(
                        width: width,
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Text(items[i].$1,
                                style: context.text.titleLarge),
                            const SizedBox(height: 8),
                            Text(items[i].$2,
                                style: context.text.bodyMedium),
                          ],
                        ),
                      ),
                    ),
                ],
              );
            },
          ),
        ],
      ),
    );
  }
}
