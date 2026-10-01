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
