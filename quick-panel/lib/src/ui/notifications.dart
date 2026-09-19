import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../models.dart';
import 'theme.dart';

/// Notification shade section: a header, the active rows and a clear
/// all affordance. Sits under the tile grid and scrolls inside a fixed
/// height so a busy shade cannot stretch the panel.
class NotificationSection extends StatelessWidget {
  const NotificationSection({
    super.key,
    required this.notifications,
    required this.onDismiss,
    required this.onDismissAll,
  });

  final List<NotificationItem> notifications;
  final void Function(String key) onDismiss;
  final VoidCallback onDismissAll;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final clearable = notifications.any((n) => n.clearable);
    return Column(
      mainAxisSize: MainAxisSize.min,
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Row(
          children: [
            Text(
              l10n.notifTitle,
              style: const TextStyle(
                color: PanelTheme.textSecondary,
                fontSize: 15,
                fontWeight: FontWeight.w600,
              ),
            ),
            const Spacer(),
            if (clearable)
              TextButton(
                onPressed: onDismissAll,
                child: Text(l10n.notifClearAll),
              ),
          ],
        ),
        const SizedBox(height: 6),
        if (notifications.isEmpty)
          Padding(
            padding: const EdgeInsets.symmetric(vertical: 18),
            child: Center(
              child: Text(
                l10n.notifEmpty,
                style: const TextStyle(
                  color: PanelTheme.textSecondary,
                  fontSize: 14,
                ),
              ),
            ),
          )
        else
          ConstrainedBox(
            constraints: const BoxConstraints(maxHeight: 172),
            child: ListView.separated(
              shrinkWrap: true,
              padding: EdgeInsets.zero,
              itemCount: notifications.length,
              separatorBuilder: (_, _) => const SizedBox(height: 8),
              itemBuilder: (context, i) =>
                  NotificationRow(item: notifications[i], onDismiss: onDismiss),
            ),
          ),
      ],
    );
  }
}

/// One shade entry: app letter badge, title and body, dismiss button.
class NotificationRow extends StatelessWidget {
  const NotificationRow({
    super.key,
    required this.item,
    required this.onDismiss,
  });

  final NotificationItem item;
  final void Function(String key) onDismiss;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
      decoration: BoxDecoration(
        color: PanelTheme.surface,
        borderRadius: BorderRadius.circular(PanelTheme.smallTileRadius),
      ),
      child: Row(
        children: [
          CircleAvatar(
            radius: 16,
            backgroundColor: PanelTheme.surfaceHigh,
            child: Text(
              item.app.isEmpty ? '?' : item.app.characters.first,
              style: const TextStyle(
                color: PanelTheme.textPrimary,
                fontSize: 14,
                fontWeight: FontWeight.w600,
              ),
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisSize: MainAxisSize.min,
              children: [
                Text(
                  item.app,
                  style: const TextStyle(
                    color: PanelTheme.textSecondary,
                    fontSize: 12,
                  ),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                if (item.title.isNotEmpty)
                  Text(
                    item.title,
                    style: const TextStyle(
                      color: PanelTheme.textPrimary,
                      fontSize: 15,
                      fontWeight: FontWeight.w600,
                    ),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                if (item.text.isNotEmpty)
                  Text(
                    item.text,
                    style: const TextStyle(
                      color: PanelTheme.textSecondary,
                      fontSize: 13,
                    ),
                    maxLines: 2,
                    overflow: TextOverflow.ellipsis,
                  ),
              ],
            ),
          ),
          if (item.clearable)
            IconButton(
              tooltip: l10n.notifDismiss,
              icon: const Icon(Icons.close, size: 20),
              color: PanelTheme.textSecondary,
              onPressed: () => onDismiss(item.key),
            ),
        ],
      ),
    );
  }
}
