import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../apps.dart';
import '../../links.dart';
import '../../theme.dart';
import '../app_builds_section.dart';
import '../shell.dart';
import '../widgets.dart';

/// Downloads page for one desktop app lane - /download/cte and
/// /download/hbsup share this shell.
class AppDownloadsPage extends StatelessWidget {
  const AppDownloadsPage({super.key, required this.app});

  final SiteApp app;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return PageBody(
      children: [
        PageHead(
          title: l10n.appDownloadsTitle(SiteApps.label(app)),
          subtitle: l10n.appDownloadsSubtitle(SiteApps.label(app)),
        ),
        Band(
          color: context.colors.surfaceContainerHighest,
          width: Layout.text + 96,
          padding: const EdgeInsets.symmetric(vertical: 72),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              if (app == SiteApp.hbsup) ...[
                Reveal(child: _WindowsNote(l10n: l10n)),
                const SizedBox(height: 32),
              ],
              Reveal(
                child: AppBuildsSection(
                    tagPrefix: SiteApps.tagPrefix(app)),
              ),
              const SizedBox(height: 24),
              Reveal(
                child: ChevronLink(
                  label: l10n.cteSourceCta,
                  onPressed: () => launchUrl(
                      Uri.parse(Links.tree(SiteApps.treePath(app)))),
                ),
              ),
            ],
          ),
        ),
        if (app == SiteApp.hbsup)
          Band(
            width: Layout.text + 96,
            padding: const EdgeInsets.symmetric(vertical: 72),
            child: _HbsupShots(l10n: l10n),
          ),
      ],
    );
  }
}

/// The app's test goldens, shown as a preview strip on the download page.
/// Files are copied from applications/hbsup/test/golden/goldens -
/// hbsup_assets_test.dart fails when the two sets drift apart.
class _HbsupShots extends StatelessWidget {
  const _HbsupShots({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final shots = [
      ('assets/hbsup/hbsup_connect.png', l10n.hbsupShotConnect),
      ('assets/hbsup/hbsup_connect_unsupported.png',
          l10n.hbsupShotConnectUnsupported),
      ('assets/hbsup/hbsup_backup_ready.png', l10n.hbsupShotBackupReady),
      ('assets/hbsup/hbsup_backup_blocked.png',
          l10n.hbsupShotBackupBlocked),
      ('assets/hbsup/hbsup_backup_done.png', l10n.hbsupShotBackupDone),
    ];
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Reveal(
          child:
              Text(l10n.hbsupShotsTitle, style: context.text.titleLarge),
        ),
        const SizedBox(height: 32),
        LayoutBuilder(
          builder: (context, constraints) {
            final wide = constraints.maxWidth >= 560;
            final half = (constraints.maxWidth - 32) / 2;
            return Wrap(
              spacing: 32,
              runSpacing: 40,
              children: [
                for (var i = 0; i < shots.length; i++)
                  Reveal(
                    delay: Duration(milliseconds: (i % 2) * 100),
                    child: SizedBox(
                      width: wide ? half : constraints.maxWidth,
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
      ],
    );
  }
}

class _WindowsNote extends StatelessWidget {
  const _WindowsNote({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(32),
      decoration: BoxDecoration(
        color: context.colors.surface,
        borderRadius: BorderRadius.circular(24),
        border: Border.all(
            color: AppColors.bad.withValues(alpha: 0.5), width: 1),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.warning_amber_rounded,
                  size: 20, color: AppColors.bad),
              const SizedBox(width: 10),
              Text(l10n.hbsupWindowsTitle,
                  style: context.text.titleMedium),
            ],
          ),
          const SizedBox(height: 12),
          Text(l10n.hbsupWindowsBody, style: context.text.bodyMedium),
        ],
      ),
    );
  }
}
