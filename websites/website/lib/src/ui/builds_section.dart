import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:intl/intl.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../builds.dart';
import '../links.dart';
import '../theme.dart';
import 'widgets.dart';

/// Section on the download page that lists dist pipeline releases by channel.
/// Fetches from the GitLab releases API at runtime, so every new build the CI
/// publishes shows up without anyone touching the site.
class BuildsSection extends StatefulWidget {
  const BuildsSection({super.key});

  @override
  State<BuildsSection> createState() => _BuildsSectionState();
}

class _BuildsSectionState extends State<BuildsSection> {
  BuildChannel _channel = BuildChannel.release;
  Future<List<BuildRelease>>? _future;

  @override
  void initState() {
    super.initState();
    _future = _fetch();
  }

  Future<List<BuildRelease>> _fetch() async {
    final res = await http.get(Uri.parse(Links.releasesApi));
    if (res.statusCode != 200) {
      throw http.ClientException('HTTP ${res.statusCode}', res.request?.url);
    }
    return parseReleases(jsonDecode(res.body) as List<dynamic>);
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(l10n.buildsTitle, style: context.text.titleLarge),
        const SizedBox(height: 8),
        Text(l10n.buildsSubtitle,
            style: context.text.bodyMedium!.copyWith(fontSize: 15)),
        const SizedBox(height: 24),
        _ChannelSelector(
          channel: _channel,
          onChanged: (c) => setState(() => _channel = c),
        ),
        const SizedBox(height: 24),
        FutureBuilder<List<BuildRelease>>(
          future: _future,
          builder: (context, snapshot) {
            if (snapshot.hasError) {
              return _ErrorState(
                message: l10n.buildsError,
                retry: l10n.buildsRetry,
                onRetry: () => setState(() => _future = _fetch()),
              );
            }
            if (!snapshot.hasData) {
              return _Loading(message: l10n.buildsLoading);
            }
            final builds = releasesInChannel(snapshot.data!, _channel);
            if (builds.isEmpty) {
              return _Empty(message: l10n.buildsEmpty);
            }
            return Column(
              children: [
                for (final b in builds.take(5)) _BuildCard(release: b),
              ],
            );
          },
        ),
      ],
    );
  }
}

class _ChannelSelector extends StatelessWidget {
  const _ChannelSelector({
    required this.channel,
    required this.onChanged,
  });

  final BuildChannel channel;
  final ValueChanged<BuildChannel> onChanged;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final labels = {
      BuildChannel.release: l10n.buildsChannelRelease,
      BuildChannel.beta: l10n.buildsChannelBeta,
      BuildChannel.alpha: l10n.buildsChannelAlpha,
    };
    return SegmentedButton<BuildChannel>(
      segments: [
        for (final ch in BuildChannel.values)
          ButtonSegment(value: ch, label: Text(labels[ch]!)),
      ],
      selected: {channel},
      onSelectionChanged: (s) => onChanged(s.first),
      style: ButtonStyle(
        visualDensity: VisualDensity.comfortable,
        shape: WidgetStatePropertyAll(
          RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
        ),
      ),
    );
  }
}

class _BuildCard extends StatelessWidget {
  const _BuildCard({required this.release});

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
                child: Text(
                  release.tag,
                  style: context.text.titleMedium,
                ),
              ),
              Text(date, style: context.text.labelSmall),
            ],
          ),
          const SizedBox(height: 16),
          Wrap(
            spacing: 12,
            runSpacing: 10,
            children: [
              if (release.fullImage!.url.isNotEmpty)
                _AssetChip(
                  label: l10n.buildsFullImage,
                  url: release.fullImage!.url,
                  prominent: true,
                ),
              if (release.cleanImage!.url.isNotEmpty)
                _AssetChip(
                  label: l10n.buildsCleanImage,
                  url: release.cleanImage!.url,
                ),
              for (final a in release.assets)
                if (a.name == 'build-logs.tar.xz')
                  _AssetChip(label: l10n.buildsLogs, url: a.url),
              for (final a in release.assets)
                if (a.name == 'SHA256SUMS.txt')
                  _AssetChip(label: l10n.buildsChecksums, url: a.url),
            ],
          ),
          const SizedBox(height: 14),
          ChevronLink(
            label: l10n.buildsViewRelease,
            large: false,
            onPressed: () => launchUrl(Uri.parse(Links.releasePage(release.tag))),
          ),
        ],
      ),
    );
  }
}

class _AssetChip extends StatefulWidget {
  const _AssetChip({
    required this.label,
    required this.url,
    this.prominent = false,
  });

  final String label;
  final String url;
  final bool prominent;

  @override
  State<_AssetChip> createState() => _AssetChipState();
}

class _AssetChipState extends State<_AssetChip> {
  bool _hover = false;

  @override
  Widget build(BuildContext context) {
    final accent = context.colors.primary;
    final bg = widget.prominent
        ? accent
        : (_hover ? accent.withValues(alpha: 0.12) : Colors.transparent);
    final fg = widget.prominent ? Colors.white : accent;
    return FocusableActionDetector(
      onShowHoverHighlight: (v) => setState(() => _hover = v),
      actions: activateActions(() => launchUrl(Uri.parse(widget.url))),
      mouseCursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: () => launchUrl(Uri.parse(widget.url)),
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 120),
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 9),
          decoration: BoxDecoration(
            color: bg,
            borderRadius: BorderRadius.circular(980),
            border: Border.all(color: accent, width: 1),
          ),
          child: Text(
            widget.label,
            style: context.text.labelSmall!.copyWith(
              color: fg,
              fontSize: 14,
              fontWeight: FontWeight.w500,
            ),
          ),
        ),
      ),
    );
  }
}

class _Loading extends StatelessWidget {
  const _Loading({required this.message});
  final String message;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 48),
      child: Center(
        child: Column(
          children: [
            SizedBox(
              width: 22,
              height: 22,
              child: CircularProgressIndicator(
                strokeWidth: 2,
                color: context.colors.secondary,
              ),
            ),
            const SizedBox(height: 16),
            Text(message, style: context.text.bodyMedium),
          ],
        ),
      ),
    );
  }
}

class _Empty extends StatelessWidget {
  const _Empty({required this.message});
  final String message;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 48),
      child: Center(
        child: Text(message, style: context.text.bodyMedium),
      ),
    );
  }
}

class _ErrorState extends StatelessWidget {
  const _ErrorState({
    required this.message,
    required this.retry,
    required this.onRetry,
  });

  final String message;
  final String retry;
  final VoidCallback onRetry;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 48),
      child: Center(
        child: Column(
          children: [
            Icon(Icons.cloud_off_outlined,
                size: 28, color: context.colors.secondary),
            const SizedBox(height: 12),
            Text(message, style: context.text.bodyMedium),
            const SizedBox(height: 16),
            PillButton(label: retry, small: true, onPressed: onRetry),
          ],
        ),
      ),
    );
  }
}
