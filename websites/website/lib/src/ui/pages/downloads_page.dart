import 'dart:ui' show ImageFilter;

import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../links.dart';
import '../../theme.dart';
import '../builds_section.dart';
import '../shell.dart';
import '../widgets.dart';

class DownloadsPage extends StatefulWidget {
  const DownloadsPage({super.key});

  @override
  State<DownloadsPage> createState() => _DownloadsPageState();
}

class _DownloadsPageState extends State<DownloadsPage> {
  bool _backedUp = false;

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
              Reveal(child: _AlphaCard(l10n: l10n)),
              const SizedBox(height: 32),
              Reveal(child: _WarnCard(l10n: l10n)),
              const SizedBox(height: 32),
              Reveal(
                child: _BackupCard(
                  l10n: l10n,
                  confirmed: _backedUp,
                  onChanged: (v) => setState(() => _backedUp = v),
                ),
              ),
              const SizedBox(height: 48),
              Reveal(
                child: _Locked(
                  unlocked: _backedUp,
                  hint: l10n.downloadLockedHint,
                  child: _Steps(l10n: l10n),
                ),
              ),
              const SizedBox(height: 48),
              Reveal(child: _Requirements(l10n: l10n)),
              const SizedBox(height: 64),
              Reveal(
                child: _Locked(
                  unlocked: _backedUp,
                  hint: l10n.downloadLockedHint,
                  child: const BuildsSection(),
                ),
              ),
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

class _AlphaCard extends StatelessWidget {
  const _AlphaCard({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return _Card(
      tint: AppColors.accent.withValues(alpha: 0.4),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.science_outlined,
                  size: 20, color: context.colors.primary),
              const SizedBox(width: 10),
              Text(l10n.downloadAlphaTitle, style: context.text.titleMedium),
            ],
          ),
          const SizedBox(height: 12),
          Text(l10n.downloadAlphaBody, style: context.text.bodyMedium),
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

class _BackupCard extends StatelessWidget {
  const _BackupCard({
    required this.l10n,
    required this.confirmed,
    required this.onChanged,
  });

  final AppLocalizations l10n;
  final bool confirmed;
  final ValueChanged<bool> onChanged;

  @override
  Widget build(BuildContext context) {
    final steps = [
      l10n.downloadBackupStep1,
      l10n.downloadBackupStep2,
      l10n.downloadBackupStep3,
    ];
    return _Card(
      tint: confirmed
          ? AppColors.ok.withValues(alpha: 0.5)
          : AppColors.accent.withValues(alpha: 0.4),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(Icons.shield_outlined,
                  size: 20, color: context.colors.primary),
              const SizedBox(width: 10),
              Text(l10n.downloadBackupTitle, style: context.text.titleMedium),
            ],
          ),
          const SizedBox(height: 12),
          Text(l10n.downloadBackupBody, style: context.text.bodyMedium),
          const SizedBox(height: 20),
          for (var i = 0; i < steps.length; i++)
            Padding(
              padding: const EdgeInsets.only(bottom: 12),
              child: _NumRow(index: i + 1, text: steps[i]),
            ),
          const SizedBox(height: 8),
          _BackupConfirm(
            label: l10n.downloadBackupConfirm,
            confirmed: confirmed,
            onChanged: onChanged,
          ),
        ],
      ),
    );
  }
}

class _BackupConfirm extends StatelessWidget {
  const _BackupConfirm({
    required this.label,
    required this.confirmed,
    required this.onChanged,
  });

  final String label;
  final bool confirmed;
  final ValueChanged<bool> onChanged;

  @override
  Widget build(BuildContext context) {
    final accent = context.colors.primary;
    return FocusableActionDetector(
      actions: activateActions(() => onChanged(!confirmed)),
      mouseCursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: () => onChanged(!confirmed),
        child: Row(
          children: [
            AnimatedContainer(
              duration: const Duration(milliseconds: 150),
              width: 22,
              height: 22,
              decoration: BoxDecoration(
                color: confirmed ? accent : Colors.transparent,
                borderRadius: BorderRadius.circular(6),
                border: Border.all(
                  color: confirmed ? accent : context.colors.secondary,
                  width: 1.5,
                ),
              ),
              child: confirmed
                  ? const Icon(Icons.check, size: 16, color: Colors.white)
                  : null,
            ),
            const SizedBox(width: 12),
            Expanded(
              child: Text(
                label,
                style: context.text.labelLarge!.copyWith(fontSize: 15),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

/// Blurs its child and swallows taps until [unlocked] - used to hold back
/// the flashing steps and downloads until a backup is confirmed.
class _Locked extends StatelessWidget {
  const _Locked({
    required this.unlocked,
    required this.hint,
    required this.child,
  });

  final bool unlocked;
  final String hint;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    if (unlocked) return child;
    return ClipRect(
      child: Stack(
        children: [
          ExcludeSemantics(
            child: IgnorePointer(
              child: ImageFiltered(
                imageFilter: ImageFilter.blur(sigmaX: 5, sigmaY: 5),
                child: Opacity(opacity: 0.4, child: child),
              ),
            ),
          ),
          Positioned.fill(
            child: Center(
              child: Container(
                padding:
                    const EdgeInsets.symmetric(horizontal: 18, vertical: 10),
                decoration: BoxDecoration(
                  color: context.colors.surface,
                  borderRadius: BorderRadius.circular(980),
                  border:
                      Border.all(color: context.colors.outline, width: 1),
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(Icons.lock_outline,
                        size: 16, color: context.colors.secondary),
                    const SizedBox(width: 8),
                    Text(
                      hint,
                      style:
                          context.text.labelLarge!.copyWith(fontSize: 14),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _NumRow extends StatelessWidget {
  const _NumRow({required this.index, required this.text});

  final int index;
  final String text;

  @override
  Widget build(BuildContext context) {
    return Row(
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
            '$index',
            style: context.text.labelSmall!.copyWith(
              color: Colors.white,
              fontWeight: FontWeight.w600,
            ),
          ),
        ),
        const SizedBox(width: 14),
        Expanded(
          child: Text(
            text,
            style: context.text.bodyMedium!.copyWith(
              color: context.colors.onSurface,
              fontSize: 15,
            ),
          ),
        ),
      ],
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
            child: _NumRow(index: i + 1, text: steps[i]),
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
