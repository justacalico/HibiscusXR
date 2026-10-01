import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../../flashdocs.dart';
import '../../../links.dart';
import '../../../routes.dart';
import '../../../theme.dart';
import '../../widgets.dart';
import 'flashdocs_layout.dart';

/// The per-OS flashing guide. Today only the Neo 2 / Linux pair exists,
/// so this page is that guide - new device or OS guides get their own
/// pages next to it.
class FlashDocsGuidePage extends StatelessWidget {
  const FlashDocsGuidePage({
    super.key,
    required this.device,
    required this.system,
  });

  final FlashDocDevice device;
  final FlashDocSystem system;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return FlashDocsLayout(
      activePath: device.guidePath(system.slug),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Eyebrow('${device.name(l10n)} · ${system.name(l10n)}'),
          const SizedBox(height: 10),
          Text(
            l10n.flashdocsNeo2Title,
            style: context.text.headlineMedium,
          ),
          const SizedBox(height: 12),
          Text(
            l10n.flashdocsNeo2Intro,
            style: context.text.bodyMedium!.copyWith(fontSize: 17),
          ),
          const SizedBox(height: 28),
          _WarnCard(l10n: l10n),
          const SizedBox(height: 40),
          _Requirements(l10n: l10n),
          const SizedBox(height: 40),
          _StepSection(
            title: l10n.flashdocsNeo2RootTitle,
            body: l10n.flashdocsNeo2RootBody,
            l10n: l10n,
          ),
          const SizedBox(height: 16),
          PillButton(
            label: l10n.flashdocsNeo2RootDownload,
            small: true,
            onPressed: () => launchUrl(Uri.parse(Links.neo2RootBoot)),
          ),
          const SizedBox(height: 20),
          _Commands(lines: [
            l10n.flashdocsNeo2RootCmd1,
            l10n.flashdocsNeo2RootCmd2,
            l10n.flashdocsNeo2RootCmd3,
            l10n.flashdocsNeo2RootCmd4,
          ]),
          const SizedBox(height: 14),
          _Note(text: l10n.flashdocsNeo2RootNote),
          const SizedBox(height: 40),
          _StepSection(
            title: l10n.flashdocsNeo2BackupTitle,
            body: l10n.flashdocsNeo2BackupBody,
            l10n: l10n,
          ),
          const SizedBox(height: 16),
          PillButton(
            label: l10n.downloadBackupCta,
            small: true,
            onPressed: () => context.go(Routes.hbsupDownload),
          ),
          const SizedBox(height: 40),
          _StepSection(
            title: l10n.flashdocsNeo2FlashTitle,
            body: l10n.flashdocsNeo2FlashBody,
            l10n: l10n,
          ),
          const SizedBox(height: 20),
          _Commands(lines: [
            l10n.flashdocsNeo2FlashCmd1,
            l10n.flashdocsNeo2FlashCmd2,
            l10n.flashdocsNeo2FlashCmd3,
            l10n.flashdocsNeo2FlashCmd4,
          ]),
          const SizedBox(height: 14),
          _Note(text: l10n.flashdocsNeo2FlashNote),
          const SizedBox(height: 40),
          _StepSection(
            title: l10n.flashdocsNeo2DoneTitle,
            body: l10n.flashdocsNeo2DoneBody,
            l10n: l10n,
          ),
        ],
      ),
    );
  }
}

class _WarnCard extends StatelessWidget {
  const _WarnCard({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(24),
      decoration: BoxDecoration(
        color: context.colors.surface,
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
          color: AppColors.bad.withValues(alpha: 0.5),
          width: 1,
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.warning_amber_rounded,
                  size: 20, color: AppColors.bad),
              const SizedBox(width: 10),
              Text(l10n.flashdocsNeo2WarnTitle,
                  style: context.text.titleMedium),
            ],
          ),
          const SizedBox(height: 12),
          Text(l10n.flashdocsNeo2WarnBody,
              style: context.text.bodyMedium),
        ],
      ),
    );
  }
}

class _Requirements extends StatelessWidget {
  const _Requirements({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final reqs = [
      l10n.flashdocsNeo2Req1,
      l10n.flashdocsNeo2Req2,
      l10n.flashdocsNeo2Req3,
    ];
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(l10n.flashdocsNeo2ReqTitle, style: context.text.titleLarge),
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

class _StepSection extends StatelessWidget {
  const _StepSection({
    required this.title,
    required this.body,
    required this.l10n,
  });

  final String title;
  final String body;
  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(title, style: context.text.titleLarge),
        const SizedBox(height: 10),
        Text(
          body,
          style: context.text.bodyMedium!.copyWith(fontSize: 17),
        ),
      ],
    );
  }
}

/// A block of shell commands, one per line, in the site mono style.
class _Commands extends StatelessWidget {
  const _Commands({required this.lines});

  final List<String> lines;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
      decoration: BoxDecoration(
        color: context.colors.surfaceContainerHighest,
        borderRadius: BorderRadius.circular(14),
        border: Border.all(color: context.colors.outline, width: 1),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          for (final line in lines) Text(line, style: context.mono),
        ],
      ),
    );
  }
}

class _Note extends StatelessWidget {
  const _Note({required this.text});

  final String text;

  @override
  Widget build(BuildContext context) {
    return Text(
      text,
      style: context.text.labelSmall!.copyWith(fontSize: 13, height: 1.5),
    );
  }
}
