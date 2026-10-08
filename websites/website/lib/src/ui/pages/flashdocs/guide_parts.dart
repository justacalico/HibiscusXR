import 'package:flutter/material.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../../theme.dart';

/// Shared building blocks for the per-device guides: a titled step with a
/// body, a mono command block, and a small footnote line.
class GuideStep extends StatelessWidget {
  const GuideStep({
    super.key,
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
class CommandBlock extends StatelessWidget {
  const CommandBlock({super.key, required this.lines});

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

class GuideNote extends StatelessWidget {
  const GuideNote({super.key, required this.text});

  final String text;

  @override
  Widget build(BuildContext context) {
    return Text(
      text,
      style: context.text.labelSmall!.copyWith(fontSize: 13, height: 1.5),
    );
  }
}
