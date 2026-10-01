import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../fdroid_index.dart';
import '../store_controller.dart';
import '../store_state.dart';
import 'theme.dart';

/// App icon that resolves through the controller's [ImageResolver] so
/// tests swap in fixed bytes. Falls back to an initial-letter tile when
/// there is no icon or the bytes fail to decode.
class AppIconImage extends StatelessWidget {
  const AppIconImage({
    super.key,
    required this.app,
    required this.repoAddress,
    required this.resolve,
    this.size = 56,
  });

  final RepoApp app;
  final String repoAddress;
  final ImageResolver resolve;
  final double size;

  @override
  Widget build(BuildContext context) {
    final placeholder = Container(
      width: size,
      height: size,
      decoration: BoxDecoration(
        color: StoreTheme.surfaceHigh,
        borderRadius: BorderRadius.circular(size * 0.24),
      ),
      alignment: Alignment.center,
      child: Text(
        app.name.isEmpty ? '?' : app.name.characters.first.toUpperCase(),
        style: TextStyle(
          fontSize: size * 0.4,
          fontWeight: FontWeight.w600,
          color: StoreTheme.textSecondary,
        ),
      ),
    );
    final iconPath = app.iconPath;
    final provider = iconPath == null
        ? null
        : resolve(repoFileUri(Uri.parse(repoAddress), iconPath).toString());
    if (provider == null) return placeholder;
    return ClipRRect(
      borderRadius: BorderRadius.circular(size * 0.24),
      child: Image(
        image: provider,
        width: size,
        height: size,
        fit: BoxFit.cover,
        errorBuilder: (context, error, stack) => placeholder,
      ),
    );
  }
}

/// One row in the catalog list.
class AppCard extends StatelessWidget {
  const AppCard({
    super.key,
    required this.app,
    required this.controller,
    required this.selected,
    required this.onTap,
  });

  final RepoApp app;
  final StoreController controller;
  final bool selected;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final installed =
        controller.store.progressOf(app.packageName).phase ==
        InstallPhase.installed;
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 4),
      child: Material(
        color: selected ? StoreTheme.surfaceHigh : StoreTheme.surface,
        borderRadius: BorderRadius.circular(StoreTheme.cardRadius),
        child: InkWell(
          borderRadius: BorderRadius.circular(StoreTheme.cardRadius),
          onTap: onTap,
          child: Padding(
            padding: const EdgeInsets.all(14),
            child: Row(
              children: [
                AppIconImage(
                  app: app,
                  repoAddress: controller.store.index?.address ??
                      controller.store.repoUrl.toString(),
                  resolve: controller.imageResolver,
                ),
                const SizedBox(width: 14),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        children: [
                          Flexible(
                            child: Text(
                              app.name,
                              maxLines: 1,
                              overflow: TextOverflow.ellipsis,
                              style: const TextStyle(
                                fontSize: 15,
                                fontWeight: FontWeight.w600,
                                color: StoreTheme.textPrimary,
                              ),
                            ),
                          ),
                          if (installed) ...[
                            const SizedBox(width: 8),
                            const _InstalledBadge(),
                          ],
                        ],
                      ),
                      const SizedBox(height: 3),
                      Text(
                        app.summary,
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                        style: const TextStyle(
                          fontSize: 13,
                          color: StoreTheme.textSecondary,
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _InstalledBadge extends StatelessWidget {
  const _InstalledBadge();

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
      decoration: BoxDecoration(
        color: StoreTheme.good.withValues(alpha: 0.16),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Text(
        AppLocalizations.of(context).installedBadge,
        style: const TextStyle(
          fontSize: 11,
          fontWeight: FontWeight.w600,
          color: StoreTheme.good,
        ),
      ),
    );
  }
}

/// The install/open affordance on the detail pane. Shape changes with
/// the phase: button, progress bar, spinner, or open button.
class InstallButton extends StatelessWidget {
  const InstallButton({super.key, required this.app, required this.controller});

  final RepoApp app;
  final StoreController controller;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final progress = controller.store.progressOf(app.packageName);
    const textStyle = TextStyle(fontSize: 14, fontWeight: FontWeight.w600);
    switch (progress.phase) {
      case InstallPhase.downloading:
        return SizedBox(
          width: 132,
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.end,
            children: [
              Text(
                l10n.downloading,
                style: textStyle.copyWith(color: StoreTheme.textSecondary),
              ),
              const SizedBox(height: 6),
              SizedBox(
                height: 6,
                child: LinearProgressIndicator(
                  value: progress.progress > 0 ? progress.progress : null,
                  color: StoreTheme.accent,
                  backgroundColor: StoreTheme.surfaceHigh,
                  borderRadius: BorderRadius.circular(3),
                ),
              ),
            ],
          ),
        );
      case InstallPhase.installing:
        return Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            const SizedBox(
              width: 16,
              height: 16,
              child: CircularProgressIndicator(
                strokeWidth: 2,
                color: StoreTheme.accent,
              ),
            ),
            const SizedBox(width: 10),
            Text(
              l10n.installing,
              style: textStyle.copyWith(color: StoreTheme.textSecondary),
            ),
          ],
        );
      case InstallPhase.prompted:
        return FilledButton.icon(
          onPressed: () => controller.install(app),
          icon: const Icon(Icons.download, size: 16),
          label: Text(l10n.install),
          style: FilledButton.styleFrom(
            foregroundColor: Colors.white,
            backgroundColor: StoreTheme.accent,
          ),
        );
      case InstallPhase.installed:
        return FilledButton.tonalIcon(
          onPressed: () => controller.openApp(app.packageName),
          icon: const Icon(Icons.open_in_new, size: 16),
          label: Text(l10n.open),
          style: FilledButton.styleFrom(
            foregroundColor: StoreTheme.textPrimary,
            backgroundColor: StoreTheme.surfaceHigh,
          ),
        );
      case InstallPhase.failed:
        return FilledButton.icon(
          onPressed: () => controller.install(app),
          icon: const Icon(Icons.refresh, size: 16),
          label: Text(l10n.retry),
          style: FilledButton.styleFrom(
            foregroundColor: StoreTheme.textPrimary,
            backgroundColor: StoreTheme.danger,
          ),
        );
      case InstallPhase.none:
        return FilledButton.icon(
          onPressed: app.latest == null ? null : () => controller.install(app),
          icon: const Icon(Icons.download, size: 16),
          label: Text(l10n.install),
          style: FilledButton.styleFrom(
            foregroundColor: Colors.white,
            backgroundColor: StoreTheme.accent,
          ),
        );
    }
  }
}

/// Label + value pair in the detail meta row.
class MetaItem extends StatelessWidget {
  const MetaItem({super.key, required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      mainAxisSize: MainAxisSize.min,
      children: [
        Text(
          label,
          style: const TextStyle(fontSize: 11, color: StoreTheme.textSecondary),
        ),
        const SizedBox(height: 2),
        Text(
          value,
          style: const TextStyle(
            fontSize: 13,
            fontWeight: FontWeight.w600,
            color: StoreTheme.textPrimary,
          ),
        ),
      ],
    );
  }
}

/// Section heading inside the detail pane.
class SectionTitle extends StatelessWidget {
  const SectionTitle(this.text, {super.key});

  final String text;

  @override
  Widget build(BuildContext context) {
    return Text(
      text,
      style: const TextStyle(
        fontSize: 15,
        fontWeight: FontWeight.w600,
        color: StoreTheme.textSecondary,
      ),
    );
  }
}

/// Centered state for the list pane: spinner, error card, or the
/// empty-search notice.
class StatusPane extends StatelessWidget {
  const StatusPane({
    super.key,
    required this.icon,
    required this.title,
    this.body,
    this.onRetry,
  });

  final IconData icon;
  final String title;
  final String? body;
  final VoidCallback? onRetry;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(icon, size: 40, color: StoreTheme.textSecondary),
            const SizedBox(height: 14),
            Text(
              title,
              textAlign: TextAlign.center,
              style: const TextStyle(
                fontSize: 15,
                fontWeight: FontWeight.w600,
                color: StoreTheme.textPrimary,
              ),
            ),
            if (body != null) ...[
              const SizedBox(height: 6),
              Text(
                body!,
                textAlign: TextAlign.center,
                style: const TextStyle(
                  fontSize: 13,
                  color: StoreTheme.textSecondary,
                ),
              ),
            ],
            if (onRetry != null) ...[
              const SizedBox(height: 16),
              FilledButton.tonal(
                onPressed: onRetry,
                style: FilledButton.styleFrom(
                  foregroundColor: StoreTheme.textPrimary,
                  backgroundColor: StoreTheme.surfaceHigh,
                ),
                child: Text(l10n.retry),
              ),
            ],
          ],
        ),
      ),
    );
  }
}


