import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../../links.dart';
import '../../routes.dart';
import '../../theme.dart';
import '../library_shot.dart';
import '../shell.dart';
import '../widgets.dart';

class HomePage extends StatelessWidget {
  const HomePage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return PageBody(
      children: [
        _Hero(l10n: l10n),
        _Stats(l10n: l10n),
        _ShellBand(l10n: l10n),
        _Trio(l10n: l10n),
        _StatusBand(l10n: l10n),
        _OpenBand(l10n: l10n),
        _DownloadBand(l10n: l10n),
      ],
    );
  }
}

class _Hero extends StatelessWidget {
  const _Hero({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final mobile = Layout.isMobile(context);
    return Band(
      padding: EdgeInsets.only(top: mobile ? 56 : 96),
      child: Column(
        children: [
          Reveal(
            child: Eyebrow(l10n.heroEyebrow, center: true),
          ),
          const SizedBox(height: 16),
          Reveal(
            delay: const Duration(milliseconds: 80),
            child: Text(
              l10n.heroTitle,
              textAlign: TextAlign.center,
              style: context.text.displayLarge!
                  .copyWith(fontSize: mobile ? 52 : 96),
            ),
          ),
          const SizedBox(height: 20),
          Reveal(
            delay: const Duration(milliseconds: 160),
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: Layout.text),
              child: Text(
                l10n.heroSubtitle,
                textAlign: TextAlign.center,
                style: context.text.bodyLarge!.copyWith(
                  fontSize: 21,
                  color: context.colors.secondary,
                ),
              ),
            ),
          ),
          const SizedBox(height: 28),
          Reveal(
            delay: const Duration(milliseconds: 240),
            child: Wrap(
              spacing: 28,
              runSpacing: 12,
              alignment: WrapAlignment.center,
              crossAxisAlignment: WrapCrossAlignment.center,
              children: [
                PillButton(
                  label: l10n.heroPrimary,
                  onPressed: () => context.go(Routes.status),
                ),
                ChevronLink(
                  label: l10n.heroSecondary,
                  onPressed: () => launchUrl(Uri.parse(Links.docs)),
                ),
              ],
            ),
          ),
          const SizedBox(height: 64),
          Reveal(
            delay: const Duration(milliseconds: 320),
            child: Container(
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(24),
                border: Border.all(color: context.colors.outline, width: 1),
                boxShadow: [
                  BoxShadow(
                    color: context.colors.onSurface.withValues(alpha: 0.08),
                    blurRadius: 48,
                    offset: const Offset(0, 24),
                  ),
                ],
              ),
              clipBehavior: Clip.antiAlias,
              child: const LibraryShot(),
            ),
          ),
        ],
      ),
    );
  }
}

/// Four-cell spec strip under the hero.
class _Stats extends StatelessWidget {
  const _Stats({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final stats = [
      (l10n.statSoc, l10n.statSocLabel),
      (l10n.statPanel, l10n.statPanelLabel),
      (l10n.statRepos, l10n.statReposLabel),
      (l10n.statNotes, l10n.statNotesLabel),
    ];
    return Band(
      padding: const EdgeInsets.symmetric(vertical: 56),
      child: LayoutBuilder(
        builder: (context, constraints) {
          final columns = constraints.maxWidth >= 760 ? 4 : 2;
          final width = (constraints.maxWidth - (columns - 1) * 32) / columns;
          return Wrap(
            spacing: 32,
            runSpacing: 32,
            alignment: WrapAlignment.center,
            children: [
              for (var i = 0; i < stats.length; i++)
                Reveal(
                  delay: Duration(milliseconds: i * 60),
                  child: SizedBox(
                    width: width,
                    child: Column(
                      children: [
                        Text(
                          stats[i].$1,
                          textAlign: TextAlign.center,
                          style: context.text.titleLarge!.copyWith(
                            fontSize: Layout.isMobile(context) ? 19 : 24,
                          ),
                        ),
                        const SizedBox(height: 6),
                        Text(
                          stats[i].$2,
                          textAlign: TextAlign.center,
                          style: context.text.labelSmall!
                              .copyWith(fontSize: 14),
                        ),
                      ],
                    ),
                  ),
                ),
            ],
          );
        },
      ),
    );
  }
}

/// vrhome + library showcase with the two remaining screenshots.
class _ShellBand extends StatelessWidget {
  const _ShellBand({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Band(
      color: context.colors.surfaceContainerHighest,
      child: Column(
        children: [
          Reveal(child: Eyebrow(l10n.homeShellEyebrow, center: true)),
          const SizedBox(height: 12),
          Reveal(
            child: Text(
              l10n.homeShellTitle,
              textAlign: TextAlign.center,
              style: context.text.displayMedium!.copyWith(
                fontSize: Layout.isMobile(context) ? 36 : 56,
              ),
            ),
          ),
          const SizedBox(height: 16),
          Reveal(
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: Layout.text),
              child: Text(
                l10n.homeShellBody,
                textAlign: TextAlign.center,
                style: context.text.bodyLarge!
                    .copyWith(color: context.colors.secondary),
              ),
            ),
          ),
          const SizedBox(height: 48),
          LayoutBuilder(
            builder: (context, constraints) {
              final two = constraints.maxWidth >= 760;
              final width = two ? (constraints.maxWidth - 32) / 2 : null;
              return Wrap(
                spacing: 32,
                runSpacing: 32,
                alignment: WrapAlignment.center,
                children: [
                  for (final (asset, caption) in [
                    ('assets/screenshots/collection-menu.png',
                        l10n.shotCollectionCaption),
                    ('assets/screenshots/tile-menu.png', l10n.shotMenuCaption),
                  ])
                    Reveal(
                      child: Container(
                        width: width,
                        decoration: BoxDecoration(
                          borderRadius: BorderRadius.circular(20),
                          border: Border.all(
                            color: context.colors.outline,
                            width: 1,
                          ),
                        ),
                        clipBehavior: Clip.antiAlias,
                        child: Image.asset(
                          asset,
                          fit: BoxFit.cover,
                          semanticLabel: caption,
                        ),
                      ),
                    ),
                ],
              );
            },
          ),
          const SizedBox(height: 32),
          Reveal(
            child: ChevronLink(
              label: l10n.homeShellCta,
              onPressed: () => context.go(Routes.screenshots),
            ),
          ),
        ],
      ),
    );
  }
}

/// The three pillars: overlay approach, repo structure, research log.
class _Trio extends StatelessWidget {
  const _Trio({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final items = [
      (l10n.homeWayOverlayTitle, l10n.homeWayOverlayBody),
      (l10n.homeWayReposTitle, l10n.homeWayReposBody),
      (l10n.homeWayNotesTitle, l10n.homeWayNotesBody),
    ];
    return Band(
      child: Column(
        children: [
          LayoutBuilder(
            builder: (context, constraints) {
              final columns = constraints.maxWidth >= 760 ? 3 : 1;
              final width =
                  (constraints.maxWidth - (columns - 1) * 48) / columns;
              return Wrap(
                spacing: 48,
                runSpacing: 40,
                children: [
                  for (var i = 0; i < items.length; i++)
                    Reveal(
                      delay: Duration(milliseconds: i * 100),
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
          const SizedBox(height: 36),
          Reveal(
            child: Align(
              alignment: Alignment.centerLeft,
              child: ChevronLink(
                label: l10n.homeWayCta,
                onPressed: () => context.go(Routes.repositories),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

/// Status teaser band - leads to the full status page.
class _StatusBand extends StatelessWidget {
  const _StatusBand({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Band(
      color: context.colors.surfaceContainerHighest,
      child: Column(
        children: [
          Reveal(child: Eyebrow(l10n.homeStatusEyebrow, center: true)),
          const SizedBox(height: 12),
          Reveal(
            child: Text(
              l10n.homeStatusTitle,
              textAlign: TextAlign.center,
              style: context.text.displayMedium!.copyWith(
                fontSize: Layout.isMobile(context) ? 36 : 56,
              ),
            ),
          ),
          const SizedBox(height: 16),
          Reveal(
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: Layout.text),
              child: Text(
                l10n.homeStatusBody,
                textAlign: TextAlign.center,
                style: context.text.bodyLarge!
                    .copyWith(color: context.colors.secondary),
              ),
            ),
          ),
          const SizedBox(height: 24),
          Reveal(
            child: ChevronLink(
              label: l10n.homeStatusCta,
              onPressed: () => context.go(Routes.status),
            ),
          ),
        ],
      ),
    );
  }
}

/// Black band regardless of theme - Apple's inverted contrast section.
class _OpenBand extends StatelessWidget {
  const _OpenBand({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Theme(
      data: darkTheme(),
      child: Builder(
        builder: (context) => Band(
          color: context.colors.surface,
          child: Column(
            children: [
              Reveal(
                child: Text(
                  l10n.homeOpenTitle,
                  textAlign: TextAlign.center,
                  style: context.text.displayMedium!.copyWith(
                    fontSize: Layout.isMobile(context) ? 36 : 56,
                  ),
                ),
              ),
              const SizedBox(height: 16),
              Reveal(
                child: ConstrainedBox(
                  constraints: const BoxConstraints(maxWidth: Layout.text),
                  child: Text(
                    l10n.homeOpenBody,
                    textAlign: TextAlign.center,
                    style: context.text.bodyLarge!
                        .copyWith(color: context.colors.secondary),
                  ),
                ),
              ),
              const SizedBox(height: 24),
              Reveal(
                child: ChevronLink(
                  label: l10n.homeOpenSource,
                  onPressed: () => launchUrl(Uri.parse(Links.group)),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _DownloadBand extends StatelessWidget {
  const _DownloadBand({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Band(
      color: context.colors.surfaceContainerHighest,
      child: Column(
        children: [
          Reveal(
            child: Text(
              l10n.homeDownloadTitle,
              textAlign: TextAlign.center,
              style: context.text.displayMedium!.copyWith(
                fontSize: Layout.isMobile(context) ? 36 : 56,
              ),
            ),
          ),
          const SizedBox(height: 16),
          Reveal(
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: Layout.text),
              child: Text(
                l10n.homeDownloadBody,
                textAlign: TextAlign.center,
                style: context.text.bodyLarge!
                    .copyWith(color: context.colors.secondary),
              ),
            ),
          ),
          const SizedBox(height: 24),
          Reveal(
            child: ChevronLink(
              label: l10n.homeDownloadCta,
              onPressed: () => context.go(Routes.download),
            ),
          ),
        ],
      ),
    );
  }
}
