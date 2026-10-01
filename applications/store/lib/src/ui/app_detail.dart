import 'package:flutter/material.dart';
import 'package:intl/intl.dart';

import '../../l10n/app_localizations.dart';
import '../fdroid_index.dart';
import '../store_controller.dart';
import '../store_state.dart';
import '../units.dart';
import 'theme.dart';
import 'widgets.dart';

/// The right-hand detail surface on wide layouts. Reads the selection
/// out of the store so list taps swap it live.
class AppDetailPane extends StatelessWidget {
  const AppDetailPane({super.key, required this.controller});

  final StoreController controller;

  @override
  Widget build(BuildContext context) {
    final app = controller.store.selected;
    if (app == null) {
      return StatusPane(
        icon: Icons.apps_outlined,
        title: AppLocalizations.of(context).selectAppPrompt,
      );
    }
    return AppDetailBody(app: app, controller: controller);
  }
}

/// The detail pushed as a route on compact layouts. Same body, plus a
/// back row.
class AppDetailPage extends StatelessWidget {
  const AppDetailPage({super.key, required this.controller});

  final StoreController controller;

  @override
  Widget build(BuildContext context) {
    var app = controller.store.selected;
    return Scaffold(
      body: AnimatedBuilder(
        animation: controller.store,
        builder: (context, _) {
          final current = controller.store.selected;
          if (current == null) {
            return StatusPane(
              icon: Icons.apps_outlined,
              title: AppLocalizations.of(context).selectAppPrompt,
            );
          }
          app = current;
          return Column(
            children: [
              Align(
                alignment: Alignment.centerLeft,
                child: Padding(
                  padding: const EdgeInsets.fromLTRB(8, 8, 8, 0),
                  child: IconButton(
                    icon: const Icon(Icons.arrow_back, size: 20),
                    color: StoreTheme.textSecondary,
                    onPressed: () => Navigator.of(context).maybePop(),
                  ),
                ),
              ),
              Expanded(
                child: AppDetailBody(app: app!, controller: controller),
              ),
            ],
          );
        },
      ),
    );
  }
}

/// Icon, name, actions, meta, screenshots, description, versions.
class AppDetailBody extends StatelessWidget {
  const AppDetailBody({super.key, required this.app, required this.controller});

  final StoreApp app;
  final StoreController controller;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final store = controller.store;
    final repoAddress =
        store.index?.address ?? store.repoUrl.toString();
    final latest = app.latest;
    final progress = store.progressOf(app.packageName);
    final description = stripHtml(app.description);
    return ListView(
      padding: const EdgeInsets.fromLTRB(32, 24, 32, 32),
      children: [
        Row(
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            AppIconImage(
              app: app,
              repoAddress: repoAddress,
              resolve: controller.imageResolver,
              size: 84,
            ),
            const SizedBox(width: 18),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    app.name,
                    style: const TextStyle(
                      fontSize: 20,
                      fontWeight: FontWeight.w700,
                      color: StoreTheme.textPrimary,
                    ),
                  ),
                  if (app.author != null) ...[
                    const SizedBox(height: 2),
                    Text(
                      l10n.byAuthor(app.author!),
                      style: const TextStyle(
                        fontSize: 13,
                        color: StoreTheme.textSecondary,
                      ),
                    ),
                  ],
                  const SizedBox(height: 2),
                  Text(
                    app.packageName,
                    style: const TextStyle(
                      fontSize: 12,
                      color: StoreTheme.textSecondary,
                    ),
                  ),
                ],
              ),
            ),
            InstallButton(app: app, controller: controller),
          ],
        ),
        if (progress.phase == InstallPhase.failed &&
            progress.error != null) ...[
          const SizedBox(height: 10),
          Text(
            '${l10n.installFailed}: ${progress.error}',
            style: const TextStyle(fontSize: 12, color: StoreTheme.danger),
          ),
        ],
        const SizedBox(height: 24),
        Wrap(
          spacing: 40,
          runSpacing: 14,
          children: [
            if (latest != null) ...[
              MetaItem(label: l10n.metaVersion, value: latest.versionName),
              MetaItem(label: l10n.metaSize, value: formatBytes(latest.size)),
            ],
            if (app.license != null)
              MetaItem(label: l10n.metaLicense, value: app.license!),
            MetaItem(
              label: l10n.metaUpdated,
              value: DateFormat.yMMMd().format(app.lastUpdated),
            ),
            if (latest?.minSdk != null)
              MetaItem(label: l10n.metaMinSdk, value: 'API ${latest!.minSdk}'),
          ],
        ),
        if (app.screenshots.isNotEmpty) ...[
          const SizedBox(height: 28),
          SectionTitle(l10n.sectionScreenshots),
          const SizedBox(height: 12),
          SizedBox(
            height: 200,
            child: ListView.separated(
              scrollDirection: Axis.horizontal,
              itemCount: app.screenshots.length,
              separatorBuilder: (_, _) => const SizedBox(width: 12),
              itemBuilder: (context, i) {
                final provider = controller.imageResolver(
                  repoFileUri(
                    Uri.parse(repoAddress),
                    app.screenshots[i],
                  ).toString(),
                );
                if (provider == null) return const SizedBox.shrink();
                return ClipRRect(
                  borderRadius: BorderRadius.circular(14),
                  child: Image(
                    image: provider,
                    height: 200,
                    fit: BoxFit.fitHeight,
                    errorBuilder: (context, error, stack) =>
                        const SizedBox.shrink(),
                  ),
                );
              },
            ),
          ),
        ],
        if (description.isNotEmpty) ...[
          const SizedBox(height: 28),
          SectionTitle(l10n.sectionDescription),
          const SizedBox(height: 10),
          Text(
            description,
            style: const TextStyle(
              fontSize: 14,
              height: 1.5,
              color: StoreTheme.textPrimary,
            ),
          ),
        ],
        if (app.versions.length > 1) ...[
          const SizedBox(height: 28),
          SectionTitle(l10n.sectionVersions),
          const SizedBox(height: 8),
          for (final v in app.versions.take(8))
            Padding(
              padding: const EdgeInsets.symmetric(vertical: 7),
              child: Row(
                children: [
                  Expanded(
                    child: Text(
                      l10n.versionRow(v.versionName, v.versionCode),
                      style: const TextStyle(
                        fontSize: 13,
                        color: StoreTheme.textPrimary,
                      ),
                    ),
                  ),
                  Text(
                    DateFormat.yMMMd().format(v.added),
                    style: const TextStyle(
                      fontSize: 12,
                      color: StoreTheme.textSecondary,
                    ),
                  ),
                  const SizedBox(width: 16),
                  Text(
                    formatBytes(v.size),
                    style: const TextStyle(
                      fontSize: 12,
                      color: StoreTheme.textSecondary,
                    ),
                  ),
                ],
              ),
            ),
        ],
      ],
    );
  }
}
