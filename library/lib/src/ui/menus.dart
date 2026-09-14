import 'package:flutter/material.dart';
import 'package:intl/intl.dart';

import '../../l10n/app_localizations.dart';
import '../menu_actions.dart';
import '../models.dart';
import 'theme.dart';

/// Anchored context menu for a tile. Returns the picked action.
Future<AppAction?> showAppMenu(
  BuildContext context,
  Offset at,
  List<AppAction> actions,
) {
  final l10n = AppLocalizations.of(context);
  final overlay = Overlay.of(context).context.findRenderObject() as RenderBox;
  return showMenu<AppAction>(
    context: context,
    position: RelativeRect.fromRect(
      at & const Size(1, 1),
      Offset.zero & overlay.size,
    ),
    items: [
      for (final a in actions)
        PopupMenuItem<AppAction>(
          value: a,
          child: Row(
            children: [
              Icon(
                _iconFor(a),
                size: 18,
                color: a == AppAction.uninstall
                    ? LibraryTheme.danger
                    : LibraryTheme.textSecondary,
              ),
              const SizedBox(width: 12),
              Text(
                _labelFor(l10n, a),
                style: TextStyle(
                  color: a == AppAction.uninstall
                      ? LibraryTheme.danger
                      : LibraryTheme.textPrimary,
                ),
              ),
            ],
          ),
        ),
    ],
  );
}

IconData _iconFor(AppAction a) {
  switch (a) {
    case AppAction.open:
      return Icons.open_in_new;
    case AppAction.pin:
      return Icons.push_pin_outlined;
    case AppAction.unpin:
      return Icons.push_pin;
    case AppAction.addToGroup:
      return Icons.playlist_add;
    case AppAction.removeFromGroup:
      return Icons.playlist_remove;
    case AppAction.details:
      return Icons.info_outline;
    case AppAction.uninstall:
      return Icons.delete_outline;
  }
}

String _labelFor(AppLocalizations l10n, AppAction a) {
  switch (a) {
    case AppAction.open:
      return l10n.openApp;
    case AppAction.pin:
      return l10n.pinApp;
    case AppAction.unpin:
      return l10n.unpinApp;
    case AppAction.addToGroup:
      return l10n.addToGroup;
    case AppAction.removeFromGroup:
      return l10n.removeFromGroup;
    case AppAction.details:
      return l10n.details;
    case AppAction.uninstall:
      return l10n.uninstallApp;
  }
}

/// What the group picker resolved to.
sealed class GroupPick {
  const GroupPick();
}

class GroupPicked extends GroupPick {
  const GroupPicked(this.id);
  final String id;
}

class GroupCreateNew extends GroupPick {
  const GroupCreateNew();
}

/// Lists the existing groups plus a "new group" row.
Future<GroupPick?> pickGroup(BuildContext context, List<AppGroup> groups) {
  final l10n = AppLocalizations.of(context);
  return showDialog<GroupPick>(
    context: context,
    builder: (context) => AlertDialog(
      title: Text(l10n.addToGroup),
      content: SizedBox(
        width: 340,
        child: groups.isEmpty
            ? Padding(
                padding: const EdgeInsets.symmetric(vertical: 12),
                child: Text(
                  l10n.noGroupsYet,
                  style: const TextStyle(color: LibraryTheme.textSecondary),
                ),
              )
            : ListView(
                shrinkWrap: true,
                children: [
                  for (final g in groups)
                    ListTile(
                      leading: const Icon(Icons.folder_outlined),
                      title: Text(g.name),
                      subtitle: Text(l10n.appCount(g.members.length)),
                      onTap: () => Navigator.pop(context, GroupPicked(g.id)),
                    ),
                ],
              ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: Text(l10n.cancel),
        ),
        FilledButton.icon(
          icon: const Icon(Icons.add, size: 18),
          label: Text(l10n.newGroup),
          onPressed: () => Navigator.pop(context, const GroupCreateNew()),
        ),
      ],
    ),
  );
}

/// Text input for creating or renaming a group. Returns the trimmed name
/// or null when cancelled.
Future<String?> promptGroupName(
  BuildContext context, {
  String? initial,
  bool renaming = false,
}) {
  final l10n = AppLocalizations.of(context);
  final controller = TextEditingController(text: initial);
  return showDialog<String>(
    context: context,
    builder: (context) => AlertDialog(
      title: Text(renaming ? l10n.renameGroup : l10n.newGroup),
      content: SizedBox(
        width: 340,
        child: TextField(
          controller: controller,
          autofocus: true,
          decoration: InputDecoration(hintText: l10n.groupNameHint),
          onSubmitted: (v) => Navigator.pop(context, v.trim()),
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: Text(l10n.cancel),
        ),
        FilledButton(
          onPressed: () => Navigator.pop(context, controller.text.trim()),
          child: Text(renaming ? l10n.save : l10n.create),
        ),
      ],
    ),
  ).then((v) => v == null || v.isEmpty ? null : v);
}

/// Delete confirmation for a group. The apps themselves stay installed.
Future<bool> confirmDeleteGroup(BuildContext context, String name) async {
  final l10n = AppLocalizations.of(context);
  final ok = await showDialog<bool>(
    context: context,
    builder: (context) => AlertDialog(
      title: Text(l10n.deleteGroup),
      content: Text(l10n.deleteGroupConfirm(name)),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context, false),
          child: Text(l10n.cancel),
        ),
        FilledButton(
          style: FilledButton.styleFrom(backgroundColor: LibraryTheme.danger),
          onPressed: () => Navigator.pop(context, true),
          child: Text(l10n.delete),
        ),
      ],
    ),
  );
  return ok ?? false;
}

/// Details dialog: package, version, install/update dates, plus a shortcut
/// to the system app-info page.
Future<void> showAppDetails(
  BuildContext context,
  AppEntry app, {
  required Future<void> Function() onAppInfo,
}) {
  final l10n = AppLocalizations.of(context);
  final locale = Localizations.localeOf(context).toString();
  String fmt(int ms) => ms <= 0
      ? l10n.notAvailable
      : DateFormat.yMMMd(
          locale,
        ).format(DateTime.fromMillisecondsSinceEpoch(ms));

  Widget row(String label, String value) => Padding(
    padding: const EdgeInsets.symmetric(vertical: 6),
    child: Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        SizedBox(
          width: 92,
          child: Text(
            label,
            style: const TextStyle(color: LibraryTheme.textSecondary),
          ),
        ),
        Expanded(
          child: SelectableText(
            value,
            style: const TextStyle(color: LibraryTheme.textPrimary),
          ),
        ),
      ],
    ),
  );

  return showDialog<void>(
    context: context,
    builder: (context) => AlertDialog(
      title: Text(l10n.detailsTitle(app.label)),
      content: SizedBox(
        width: 380,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            row(l10n.detailsPackage, app.packageName),
            row(l10n.detailsVersion, app.versionName ?? l10n.notAvailable),
            row(l10n.detailsInstalled, fmt(app.firstInstallTime)),
            row(l10n.detailsUpdated, fmt(app.lastUpdateTime)),
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: Text(l10n.cancel),
        ),
        FilledButton(
          onPressed: () {
            Navigator.pop(context);
            onAppInfo();
          },
          child: Text(l10n.appInfo),
        ),
      ],
    ),
  );
}
