import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../../links.dart';
import '../../theme.dart';
import '../builds_section.dart';
import '../shell.dart';
import '../widgets.dart';

class DownloadsPage extends StatelessWidget {
  const DownloadsPage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return PageBody(
      children: [
        PageHead(title: l10n.downloadTitle, subtitle: l10n.downloadSubtitle),
        Band(
          color: context.colors.surfaceContainerHighest,
          width: Layout.text + 96,
          padding: const EdgeInsets.symmetric(vertical: 72),
          child: Column(
            children: [
              Reveal(child: _WarnCard(l10n: l10n)),
              const SizedBox(height: 32),
              Reveal(child: _ImageCard(l10n: l10n)),
              const SizedBox(height: 48),
              Reveal(child: _Steps(l10n: l10n)),
              const SizedBox(height: 48),
              Reveal(child: _Requirements(l10n: l10n)),
              const SizedBox(height: 48),
              Reveal(child: _Software(l10n: l10n)),
              const SizedBox(height: 64),
              Reveal(child: const BuildsSection()),
              const SizedBox(height: 48),
              Reveal(child: _IssueCard(l10n: l10n)),
            ],
          ),
        ),
      ],
    );
  }
}

class _Card extends StatelessWidget {
  const _Card({required this.child, this.tint});

  final Widget child;
  final Color? tint;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(32),
      decoration: BoxDecoration(
        color: context.colors.surface,
        borderRadius: BorderRadius.circular(24),
        border: Border.all(color: tint ?? context.colors.outline, width: 1),
      ),
      child: child,
    );
  }
}

class _WarnCard extends StatelessWidget {
  const _WarnCard({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return _Card(
      tint: AppColors.bad.withValues(alpha: 0.5),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.warning_amber_rounded,
                  size: 20, color: AppColors.bad),
              const SizedBox(width: 10),
              Text(l10n.downloadWarnTitle, style: context.text.titleMedium),
            ],
          ),
          const SizedBox(height: 12),
          Text(l10n.downloadWarnBody, style: context.text.bodyMedium),
        ],
      ),
    );
  }
}

class _ImageCard extends StatelessWidget {
  const _ImageCard({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return _Card(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.inventory_2_outlined,
                  size: 20, color: context.colors.primary),
              const SizedBox(width: 10),
              Text(l10n.downloadImageTitle, style: context.text.titleMedium),
            ],
          ),
          const SizedBox(height: 12),
          Text(l10n.downloadImageBody, style: context.text.bodyMedium),
          const SizedBox(height: 20),
          PillButton(
            label: l10n.downloadImageCta,
            small: true,
            onPressed: () => launchUrl(Uri.parse(Links.out)),
          ),
        ],
      ),
    );
  }
}

class _Steps extends StatelessWidget {
  const _Steps({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final steps = [
      l10n.downloadStep1,
      l10n.downloadStep2,
      l10n.downloadStep3,
      l10n.downloadStep4,
    ];
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(l10n.downloadStepsTitle, style: context.text.titleLarge),
        const SizedBox(height: 20),
        for (var i = 0; i < steps.length; i++)
          Padding(
            padding: const EdgeInsets.only(bottom: 14),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Container(
                  width: 26,
                  height: 26,
                  alignment: Alignment.center,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: context.colors.primary,
                  ),
                  child: Text(
                    '${i + 1}',
                    style: context.text.labelSmall!.copyWith(
                      color: Colors.white,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                ),
                const SizedBox(width: 14),
                Expanded(
                  child: Text(
                    steps[i],
                    style: context.text.bodyMedium!.copyWith(
                      color: context.colors.onSurface,
                      fontSize: 15,
                    ),
                  ),
                ),
              ],
            ),
          ),
        const SizedBox(height: 8),
        Text(
          l10n.downloadStepsNote,
          style: context.text.labelSmall!.copyWith(fontSize: 13, height: 1.5),
        ),
      ],
    );
  }
}

class _Requirements extends StatelessWidget {
  const _Requirements({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final reqs = [
      l10n.downloadReq1,
      l10n.downloadReq2,
      l10n.downloadReq3,
    ];
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(l10n.downloadReqTitle, style: context.text.titleLarge),
        const SizedBox(height: 16),
        for (final req in reqs)
          Padding(
            padding: const EdgeInsets.only(bottom: 10),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Icon(Icons.check_circle_outline,
                    size: 18, color: AppColors.ok),
                const SizedBox(width: 12),
                Expanded(
                  child: Text(
                    req,
                    style: context.text.bodyMedium!
                        .copyWith(color: context.colors.onSurface),
                  ),
                ),
              ],
            ),
          ),
      ],
    );
  }
}

class _Software extends StatelessWidget {
  const _Software({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return _Card(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(l10n.downloadSoftwareTitle, style: context.text.titleMedium),
          const SizedBox(height: 12),
          Text(l10n.downloadSoftwareBody, style: context.text.bodyMedium),
          const SizedBox(height: 16),
          Wrap(
            spacing: 24,
            runSpacing: 12,
            children: [
              ChevronLink(
                label: l10n.downloadVrhomeCta,
                large: false,
                onPressed: () => launchUrl(Uri.parse(Links.vrhome)),
              ),
              ChevronLink(
                label: l10n.downloadLibraryCta,
                large: false,
                onPressed: () => launchUrl(Uri.parse(Links.library)),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _IssueCard extends StatelessWidget {
  const _IssueCard({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return _Card(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.bug_report_outlined,
                  size: 20, color: context.colors.primary),
              const SizedBox(width: 10),
              Text(l10n.issuesButton, style: context.text.titleMedium),
            ],
          ),
          const SizedBox(height: 12),
          Text(l10n.issuesSubtitle, style: context.text.bodyMedium),
          const SizedBox(height: 20),
          PillButton(
            label: l10n.issuesButton,
            small: true,
            onPressed: () => launchUrl(Uri.parse(Links.newIssue)),
          ),
        ],
      ),
    );
  }
}
