import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../links.dart';
import '../../routes.dart';
import '../../theme.dart';
import '../shell.dart';
import '../widgets.dart';

/// Dedicated page for HCTE - the desktop companion tool for headsets
/// running Hibiscus. Lives at /cte.
class CtePage extends StatelessWidget {
  const CtePage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return PageBody(
      children: [
        PageHead(title: l10n.cteTitle, subtitle: l10n.cteSubtitle),
        Band(
          width: Layout.text,
          padding: const EdgeInsets.symmetric(vertical: 72),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _Block(title: l10n.cteWhatTitle, body: l10n.cteWhatBody),
              _Feature(
                title: l10n.cteFeatOverviewTitle,
                body: l10n.cteFeatOverviewBody,
              ),
              _Feature(
                title: l10n.cteFeatDisplayTitle,
                body: l10n.cteFeatDisplayBody,
              ),
              _Feature(
                title: l10n.cteFeatInstallTitle,
                body: l10n.cteFeatInstallBody,
              ),
              _Feature(
                title: l10n.cteFeatTrackingTitle,
                body: l10n.cteFeatTrackingBody,
              ),
              _Feature(
                title: l10n.cteFeatDebugTitle,
                body: l10n.cteFeatDebugBody,
              ),
            ],
          ),
        ),
        Band(
          color: context.colors.surfaceContainerHighest,
          width: Layout.text,
          padding: const EdgeInsets.symmetric(vertical: 72),
          child: Reveal(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(l10n.cteConnectTitle, style: context.text.titleLarge),
                const SizedBox(height: 10),
                Text(
                  l10n.cteConnectBody,
                  style: context.text.bodyMedium!.copyWith(fontSize: 17),
                ),
                const SizedBox(height: 32),
                Text(l10n.cteGetTitle, style: context.text.titleLarge),
                const SizedBox(height: 10),
                Text(
                  l10n.cteGetBody,
                  style: context.text.bodyMedium!.copyWith(fontSize: 17),
                ),
                const SizedBox(height: 24),
                Wrap(
                  spacing: 28,
                  runSpacing: 12,
                  children: [
                    ChevronLink(
                      label: l10n.cteReleasesCta,
                      onPressed: () => context.go(Routes.cteDownload),
                    ),
                    ChevronLink(
                      label: l10n.cteSourceCta,
                      onPressed: () =>
                          launchUrl(Uri.parse(Links.tree('applications/desktop/cte'))),
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
        Band(
          width: Layout.text + 96,
          padding: const EdgeInsets.symmetric(vertical: 72),
          child: _CteShots(l10n: l10n),
        ),
      ],
    );
  }
}

/// The app's test goldens, shown as a preview strip on the cte page.
/// Files are copied from applications/desktop/cte/test/golden/goldens -
/// cte_assets_test.dart fails when the two sets drift apart.
class _CteShots extends StatelessWidget {
  const _CteShots({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final shots = [
      ('assets/cte/cte_connect.png', l10n.cteShotConnect),
      ('assets/cte/cte_overview.png', l10n.cteShotOverview),
      ('assets/cte/cte_display.png', l10n.cteShotDisplay),
      ('assets/cte/cte_install.png', l10n.cteShotInstall),
      ('assets/cte/cte_tracking.png', l10n.cteShotTracking),
      ('assets/cte/cte_debug.png', l10n.cteShotDebug),
    ];
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Reveal(
          child:
              Text(l10n.cteShotsTitle, style: context.text.titleLarge),
        ),
        const SizedBox(height: 32),
        ShotGrid(shots: shots),
      ],
    );
  }
}

class _Block extends StatelessWidget {
  const _Block({required this.title, required this.body});

  final String title;
  final String body;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 40),
      child: Reveal(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: context.text.titleLarge),
            const SizedBox(height: 10),
            Text(
              body,
              style: context.text.bodyMedium!.copyWith(fontSize: 17),
            ),
          ],
        ),
      ),
    );
  }
}

class _Feature extends StatelessWidget {
  const _Feature({required this.title, required this.body});

  final String title;
  final String body;

  @override
  Widget build(BuildContext context) {
    return Reveal(
      child: Padding(
        padding: const EdgeInsets.only(bottom: 20),
        child: Row(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Padding(
              padding: const EdgeInsets.only(top: 7),
              child: Icon(Icons.circle,
                  size: 7, color: context.colors.secondary),
            ),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(title, style: context.text.titleMedium),
                  const SizedBox(height: 4),
                  Text(body, style: context.text.bodyMedium),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
