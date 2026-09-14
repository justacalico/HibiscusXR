import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../../links.dart';
import '../../routes.dart';
import '../../theme.dart';
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
        _Showcase(l10n: l10n),
        _Trio(l10n: l10n),
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
                  .copyWith(fontSize: mobile ? 56 : 96),
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
                  onPressed: () => context.go(Routes.features),
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
              child: Image.asset(
                'assets/screenshots/library-grid.png',
                fit: BoxFit.cover,
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _Showcase extends StatelessWidget {
  const _Showcase({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Band(
      color: context.colors.surfaceContainerHighest,
      child: Column(
        children: [
          Reveal(child: Eyebrow(l10n.homeShowcaseEyebrow, center: true)),
          const SizedBox(height: 12),
          Reveal(
            child: Text(
              l10n.homeShowcaseTitle,
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
                l10n.homeShowcaseBody,
                textAlign: TextAlign.center,
                style: context.text.bodyLarge!
                    .copyWith(color: context.colors.secondary),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _Trio extends StatelessWidget {
  const _Trio({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final items = [
      (l10n.homeFindTitle, l10n.homeFindBody),
      (l10n.homeArrangeTitle, l10n.homeArrangeBody),
      (l10n.homeControlTitle, l10n.homeControlBody),
    ];
    return Band(
      child: LayoutBuilder(
        builder: (context, constraints) {
          final columns = constraints.maxWidth >= 760 ? 3 : 1;
          final width = (constraints.maxWidth - (columns - 1) * 48) / columns;
          return Wrap(
            spacing: 48,
            runSpacing: 40,
            children: [
              for (var i = 0; i < items.length; i++)
                Reveal(
                  delay: Duration(milliseconds: i * 100),
                  child: SizedBox(
                    width: width,
                    child: _TrioItem(
                      title: items[i].$1,
                      body: items[i].$2,
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

class _TrioItem extends StatelessWidget {
  const _TrioItem({required this.title, required this.body});

  final String title;
  final String body;

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(title, style: context.text.titleLarge),
        const SizedBox(height: 8),
        Text(body, style: context.text.bodyMedium),
      ],
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
                  onPressed: () => launchUrl(Uri.parse(Links.repo)),
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
