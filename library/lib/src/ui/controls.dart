import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../library_store.dart';
import '../menu_actions.dart';
import '../models.dart';
import 'glass.dart';
import 'theme.dart';

/// Rounded pill search field wired to the store's query.
class LibrarySearchField extends StatelessWidget {
  const LibrarySearchField({
    super.key,
    required this.controller,
    required this.store,
  });

  final TextEditingController controller;
  final LibraryStore store;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return ConstrainedBox(
      constraints: const BoxConstraints(maxWidth: 460),
      child: SizedBox(
        height: 48,
        child: TextField(
          controller: controller,
          onChanged: store.setQuery,
          textInputAction: TextInputAction.search,
          style: const TextStyle(color: LibraryTheme.textPrimary),
          decoration: InputDecoration(
            hintText: l10n.searchHint,
            prefixIcon: const Icon(
              Icons.search,
              color: LibraryTheme.textSecondary,
            ),
            suffixIcon: store.query.isEmpty
                ? null
                : IconButton(
                    icon: const Icon(
                      Icons.close,
                      size: 18,
                      color: LibraryTheme.textSecondary,
                    ),
                    tooltip: l10n.clearSearch,
                    onPressed: () {
                      controller.clear();
                      store.setQuery('');
                    },
                  ),
          ),
        ),
      ),
    );
  }
}

/// Pill button opening the collection menu: flat filters, groups, then
/// the create-group action.
class CollectionDropdown extends StatelessWidget {
  const CollectionDropdown({
    super.key,
    required this.store,
    required this.onNewGroup,
    required this.onRenameGroup,
    required this.onDeleteGroup,
  });

  final LibraryStore store;
  final VoidCallback onNewGroup;
  final ValueChanged<AppGroup> onRenameGroup;
  final ValueChanged<AppGroup> onDeleteGroup;

  String _labelFor(AppLocalizations l10n, LibraryFilter f) {
    switch (f) {
      case FilterAll():
        return l10n.collectionAll;
      case FilterPinned():
        return l10n.collectionPinned;
      case FilterUserApps():
        return l10n.collectionUserApps;
      case FilterSystemApps():
        return l10n.collectionSystemApps;
      case FilterGroup(groupId: final id):
        return store.groupById(id)?.name ?? l10n.collectionAll;
    }
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final active = store.filter;
    final activeLabel = active is FilterGroup
        ? _labelFor(l10n, active)
        : l10n.collectionCount(_labelFor(l10n, active), store.countFor(active));
    final activeIsGroup = active is FilterGroup;

    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        _Pill(
          child: PopupMenuButton<CollectionItem>(
            tooltip: '',
            position: PopupMenuPosition.under,
            offset: const Offset(0, 8),
            onSelected: (item) {
              switch (item) {
                case FilterItem(filter: final f):
                  store.setFilter(f);
                case GroupItem(group: final g):
                  store.setFilter(FilterGroup(g.id));
                case NewGroupItem():
                  onNewGroup();
              }
            },
            itemBuilder: (context) => [
              for (final item in collectionItems(store))
                _entry(context, l10n, item),
            ],
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                const Icon(
                  Icons.apps,
                  size: 16,
                  color: LibraryTheme.textSecondary,
                ),
                const SizedBox(width: 8),
                Text(activeLabel, style: _pillText),
                const SizedBox(width: 6),
                const Icon(
                  Icons.keyboard_arrow_down,
                  size: 18,
                  color: LibraryTheme.textSecondary,
                ),
              ],
            ),
          ),
        ),
        if (activeIsGroup) ...[
          const SizedBox(width: 4),
          PopupMenuButton<_GroupOp>(
            tooltip: '',
            position: PopupMenuPosition.under,
            icon: const Icon(
              Icons.more_horiz,
              size: 20,
              color: LibraryTheme.textSecondary,
            ),
            onSelected: (op) {
              final g = store.groupById(active.groupId);
              if (g == null) return;
              switch (op) {
                case _GroupOp.rename:
                  onRenameGroup(g);
                case _GroupOp.delete:
                  onDeleteGroup(g);
              }
            },
            itemBuilder: (context) => [
              PopupMenuItem(
                value: _GroupOp.rename,
                child: Row(
                  children: [
                    const Icon(
                      Icons.edit_outlined,
                      size: 18,
                      color: LibraryTheme.textSecondary,
                    ),
                    const SizedBox(width: 12),
                    Text(l10n.renameGroup),
                  ],
                ),
              ),
              PopupMenuItem(
                value: _GroupOp.delete,
                child: Row(
                  children: [
                    const Icon(
                      Icons.delete_outline,
                      size: 18,
                      color: LibraryTheme.danger,
                    ),
                    const SizedBox(width: 12),
                    Text(
                      l10n.deleteGroup,
                      style: const TextStyle(color: LibraryTheme.danger),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ],
      ],
    );
  }

  PopupMenuEntry<CollectionItem> _entry(
    BuildContext context,
    AppLocalizations l10n,
    CollectionItem item,
  ) {
    switch (item) {
      case FilterItem(filter: final f):
        return _menuItem(
          item,
          l10n.collectionCount(_labelFor(l10n, f), store.countFor(f)),
          selected: store.filter == f,
        );
      case GroupItem(group: final g):
        return _menuItem(
          item,
          l10n.collectionCount(g.name, g.members.length),
          selected: store.filter == FilterGroup(g.id),
          leading: const Icon(
            Icons.folder_outlined,
            size: 18,
            color: LibraryTheme.textSecondary,
          ),
        );
      case NewGroupItem():
        return _menuItem(
          item,
          l10n.newGroup,
          leading: const Icon(Icons.add, size: 18, color: LibraryTheme.accent),
        );
    }
  }

  PopupMenuItem<CollectionItem> _menuItem(
    CollectionItem item,
    String label, {
    bool selected = false,
    Widget? leading,
  }) {
    return PopupMenuItem<CollectionItem>(
      value: item,
      child: Row(
        children: [
          if (leading != null) ...[leading, const SizedBox(width: 10)],
          Expanded(child: Text(label)),
          if (selected)
            const Icon(Icons.check, size: 18, color: LibraryTheme.accent),
        ],
      ),
    );
  }
}

enum _GroupOp { rename, delete }

/// Pill button choosing the grid sort.
class SortDropdown extends StatelessWidget {
  const SortDropdown({super.key, required this.store});

  final LibraryStore store;

  static const _options = [
    LibrarySort.custom,
    LibrarySort.nameAsc,
    LibrarySort.nameDesc,
    LibrarySort.newestFirst,
    LibrarySort.recentlyUpdated,
  ];

  String _label(AppLocalizations l10n, LibrarySort s) {
    switch (s) {
      case LibrarySort.custom:
        return l10n.sortCustom;
      case LibrarySort.nameAsc:
        return l10n.sortNameAsc;
      case LibrarySort.nameDesc:
        return l10n.sortNameDesc;
      case LibrarySort.newestFirst:
        return l10n.sortNewest;
      case LibrarySort.recentlyUpdated:
        return l10n.sortUpdated;
    }
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return _Pill(
      child: PopupMenuButton<LibrarySort>(
        tooltip: '',
        position: PopupMenuPosition.under,
        offset: const Offset(0, 8),
        onSelected: store.setSort,
        itemBuilder: (context) => [
          for (final s in _options)
            PopupMenuItem<LibrarySort>(
              value: s,
              child: Row(
                children: [
                  Expanded(child: Text(_label(l10n, s))),
                  if (store.sort == s)
                    const Icon(
                      Icons.check,
                      size: 18,
                      color: LibraryTheme.accent,
                    ),
                ],
              ),
            ),
        ],
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Icon(Icons.sort, size: 16, color: LibraryTheme.textSecondary),
            const SizedBox(width: 8),
            Text(_label(l10n, store.sort), style: _pillText),
            const SizedBox(width: 6),
            const Icon(
              Icons.keyboard_arrow_down,
              size: 18,
              color: LibraryTheme.textSecondary,
            ),
          ],
        ),
      ),
    );
  }
}

const _pillText = TextStyle(color: LibraryTheme.textPrimary, fontSize: 14);

class _Pill extends StatelessWidget {
  const _Pill({required this.child});

  final Widget child;

  @override
  Widget build(BuildContext context) {
    return Glass(
      blur: true,
      radius: LibraryTheme.pillRadius,
      child: SizedBox(
        height: 46,
        child: DefaultTextStyle.merge(
          style: _pillText,
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Center(child: child),
          ),
        ),
      ),
    );
  }
}
