import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../builds.dart';
import '../links.dart';
import '../theme.dart';
import 'builds_section.dart';
import 'widgets.dart';

/// Lists one desktop-app release lane (cte-v*, hbsup-v*) with every
/// asset as a download chip. Shares the releases API with BuildsSection.
class AppBuildsSection extends StatefulWidget {
  const AppBuildsSection({super.key, required this.tagPrefix});

  /// Lane prefix the releases must match, e.g. `hbsup-`.
  final String tagPrefix;

  @override
  State<AppBuildsSection> createState() => _AppBuildsSectionState();
}

class _AppBuildsSectionState extends State<AppBuildsSection> {
  Future<List<BuildRelease>>? _future;

  @override
  void initState() {
    super.initState();
    _future = fetchReleases();
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return FutureBuilder<List<BuildRelease>>(
      future: _future,
      builder: (context, snapshot) {
        if (snapshot.hasError) {
          return BuildsError(
            message: l10n.buildsError,
            retry: l10n.buildsRetry,
            onRetry: () =>
                setState(() => _future = fetchReleases()),
          );
        }
        if (!snapshot.hasData) {
          return BuildsLoading(message: l10n.buildsLoading);
        }
        final builds = appReleases(snapshot.data!, widget.tagPrefix);
        if (builds.isEmpty) {
          return BuildsEmpty(message: l10n.buildsEmpty);
        }
        return Column(
          children: [
            for (final b in builds.take(10)) _AppBuildCard(release: b),
          ],
        );
      },
    );
  }
}

class _AppBuildCard extends StatelessWidget {
  const _AppBuildCard({required this.release});

  final BuildRelease release;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final date = DateFormat.yMMMd().format(release.createdAt);
    return Container(
      width: double.infinity,
      margin: const EdgeInsets.only(bottom: 16),
      padding: const EdgeInsets.all(24),
      decoration: BoxDecoration(
        color: context.colors.surface,
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: context.colors.outline, width: 1),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Expanded(
                child:
                    Text(release.tag, style: context.text.titleMedium),
              ),
              Text(date, style: context.text.labelSmall),
            ],
          ),
          const SizedBox(height: 16),
          Wrap(
            spacing: 12,
            runSpacing: 10,
            children: [
              for (final a in release.assets)
                AssetChip(label: a.name, url: a.url),
            ],
          ),
          const SizedBox(height: 14),
          ChevronLink(
            label: l10n.buildsViewRelease,
            large: false,
            onPressed: () =>
                launchUrl(Uri.parse(Links.releasePage(release.tag))),
          ),
        ],
      ),
    );
  }
}
