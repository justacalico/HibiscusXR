import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../../links.dart';
import '../../theme.dart';
import '../shell.dart';
import '../widgets.dart';

class StatusPage extends StatelessWidget {
  const StatusPage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final works = [
      l10n.statusWorks1,
      l10n.statusWorks2,
      l10n.statusWorks3,
      l10n.statusWorks4,
      l10n.statusWorks5,
      l10n.statusWorks6,
      l10n.statusWorks7,
    ];
    final broken = [
      l10n.statusBroken1,
      l10n.statusBroken2,
      l10n.statusBroken3,
    ];
    return PageBody(
      children: [
        PageHead(title: l10n.statusTitle, subtitle: l10n.statusSubtitle),
        Band(
          color: context.colors.surfaceContainerHighest,
          padding: const EdgeInsets.symmetric(vertical: 72),
          child: LayoutBuilder(
            builder: (context, constraints) {
              final wide = constraints.maxWidth >= 760;
              final width = wide
                  ? (constraints.maxWidth - 32) / 2
                  : constraints.maxWidth;
              return Wrap(
                spacing: 32,
                runSpacing: 32,
                children: [
                  Reveal(
                    child: SizedBox(
                      width: width,
                      child: _StatusCard(
                        title: l10n.statusWorksTitle,
                        icon: Icons.check_circle_outline,
                        tint: AppColors.ok,
                        items: works,
                      ),
                    ),
                  ),
                  Reveal(
                    delay: const Duration(milliseconds: 120),
                    child: SizedBox(
                      width: width,
                      child: _StatusCard(
                        title: l10n.statusBrokenTitle,
                        icon: Icons.cancel_outlined,
                        tint: AppColors.bad,
                        items: broken,
                      ),
                    ),
                  ),
                ],
              );
            },
          ),
        ),
        _BlockerBand(l10n: l10n),
      ],
    );
  }
}

class _StatusCard extends StatelessWidget {
  const _StatusCard({
    required this.title,
    required this.icon,
    required this.tint,
    required this.items,
  });

  final String title;
  final IconData icon;
  final Color tint;
  final List<String> items;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(28),
      decoration: BoxDecoration(
        color: context.colors.surface,
        borderRadius: BorderRadius.circular(24),
        border: Border.all(color: context.colors.outline, width: 1),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(title, style: context.text.titleLarge),
          const SizedBox(height: 20),
          for (final item in items)
            Padding(
              padding: const EdgeInsets.only(bottom: 12),
              child: Row(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Icon(icon, size: 18, color: tint),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Text(
                      item,
                      style: context.text.bodyMedium!
                          .copyWith(color: context.colors.onSurface),
                    ),
                  ),
                ],
              ),
            ),
        ],
      ),
    );
  }
}

/// Dark callout explaining the single remaining blocker.
class _BlockerBand extends StatelessWidget {
  const _BlockerBand({required this.l10n});

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
                  l10n.statusBlockerTitle,
                  textAlign: TextAlign.center,
                  style: context.text.displayMedium!.copyWith(
                    fontSize: Layout.isMobile(context) ? 34 : 52,
                  ),
                ),
              ),
              const SizedBox(height: 16),
              Reveal(
                child: ConstrainedBox(
                  constraints: const BoxConstraints(maxWidth: Layout.text),
                  child: Text(
                    l10n.statusBlockerBody,
                    textAlign: TextAlign.center,
                    style: context.text.bodyLarge!
                        .copyWith(color: context.colors.secondary),
                  ),
                ),
              ),
              const SizedBox(height: 24),
              Reveal(
                child: ChevronLink(
                  label: l10n.statusBlockerCta,
                  onPressed: () => launchUrl(Uri.parse(Links.docs)),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
