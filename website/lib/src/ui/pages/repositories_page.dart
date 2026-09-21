import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../../repos.dart';
import '../../theme.dart';
import '../shell.dart';
import '../widgets.dart';

class RepositoriesPage extends StatelessWidget {
  const RepositoriesPage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return PageBody(
      children: [
        PageHead(title: l10n.reposTitle, subtitle: l10n.reposSubtitle),
        _RepoSection(
          title: l10n.reposSoftwareTitle,
          body: l10n.reposSoftwareBody,
          entries: reposIn(RepoGroup.software),
          band: true,
        ),
        _RepoSection(
          title: l10n.reposSourceTitle,
          body: l10n.reposSourceBody,
          entries: reposIn(RepoGroup.source),
        ),
        _RepoSection(
          title: l10n.reposDumpsTitle,
          body: l10n.reposDumpsBody,
          entries: reposIn(RepoGroup.dumps),
          band: true,
        ),
      ],
    );
  }
}

class _RepoSection extends StatelessWidget {
  const _RepoSection({
    required this.title,
    required this.body,
    required this.entries,
    this.band = false,
  });

  final String title;
  final String body;
  final List<RepoEntry> entries;
  final bool band;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Band(
      color: band ? context.colors.surfaceContainerHighest : null,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Reveal(
            child: Row(
              children: [
                Text(
                  title,
                  style: context.text.headlineMedium!.copyWith(
                    fontSize: Layout.isMobile(context) ? 30 : 40,
                  ),
                ),
                const SizedBox(width: 14),
                Text(
                  l10n.reposCount(entries.length),
                  style: context.text.labelSmall!.copyWith(fontSize: 14),
                ),
              ],
            ),
          ),
          const SizedBox(height: 10),
          Reveal(
            child: Text(body, style: context.text.bodyMedium),
          ),
          const SizedBox(height: 36),
          LayoutBuilder(
            builder: (context, constraints) {
              final w = constraints.maxWidth;
              final columns = w >= 760 ? 3 : (w >= 480 ? 2 : 1);
              final width = (w - (columns - 1) * 20) / columns;
              return Wrap(
                spacing: 20,
                runSpacing: 20,
                children: [
                  for (var i = 0; i < entries.length; i++)
                    Reveal(
                      delay: Duration(milliseconds: (i % 6) * 40),
                      child: SizedBox(
                        width: width,
                        child: _RepoCard(entry: entries[i]),
                      ),
                    ),
                ],
              );
            },
          ),
        ],
      ),
    );
  }
}

class _RepoCard extends StatefulWidget {
  const _RepoCard({required this.entry});

  final RepoEntry entry;

  @override
  State<_RepoCard> createState() => _RepoCardState();
}

class _RepoCardState extends State<_RepoCard> {
  bool _hover = false;
  bool _focus = false;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final active = _hover || _focus;
    void open() => launchUrl(Uri.parse(widget.entry.url));
    return FocusableActionDetector(
      onShowHoverHighlight: (v) => setState(() => _hover = v),
      onShowFocusHighlight: (v) => setState(() => _focus = v),
      actions: activateActions(open),
      mouseCursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: open,
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 150),
          padding: const EdgeInsets.all(20),
          decoration: BoxDecoration(
            color: context.colors.surface,
            borderRadius: BorderRadius.circular(16),
            border: Border.all(
              color: active ? context.colors.primary : context.colors.outline,
              width: 1,
            ),
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  Expanded(
                    child: Text(
                      widget.entry.label,
                      style: context.text.titleMedium!.copyWith(
                        fontSize: 16,
                        letterSpacing: -0.2,
                      ),
                    ),
                  ),
                  Icon(
                    Icons.arrow_outward,
                    size: 16,
                    color: active
                        ? context.colors.primary
                        : context.colors.secondary,
                  ),
                ],
              ),
              const SizedBox(height: 8),
              Text(
                widget.entry.describe(l10n),
                style: context.text.bodyMedium!.copyWith(fontSize: 14),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
