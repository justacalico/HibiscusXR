import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../../l10n/app_localizations.dart';
import '../library_controller.dart';
import '../library_store.dart';
import '../menu_actions.dart';
import '../models.dart';
import 'app_grid.dart';
import 'controls.dart';
import 'menus.dart';
import 'theme.dart';

/// The library window: title bar, search + collection + sort row, and the
/// tile grid (or the matching empty/loading/error state).
class LibraryPage extends StatefulWidget {
  const LibraryPage({super.key, required this.controller});

  final LibraryController controller;

  @override
  State<LibraryPage> createState() => _LibraryPageState();
}

class _LibraryPageState extends State<LibraryPage> {
  final _search = TextEditingController();

  LibraryController get _c => widget.controller;
  LibraryStore get _store => _c.store;

  @override
  void initState() {
    super.initState();
    _c.init();
  }

  @override
  void dispose() {
    _search.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: Listenable.merge([_c, _store]),
      builder: (context, _) {
        final l10n = AppLocalizations.of(context);
        return Scaffold(
          body: SafeArea(
            child: Column(
              children: [
                const SizedBox(height: 14),
                _header(l10n),
                const SizedBox(height: 18),
                _controls(),
                Expanded(child: _body(l10n)),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _header(AppLocalizations l10n) {
    return SizedBox(
      height: 40,
      child: Stack(
        children: [
          Center(
            child: Text(
              l10n.appTitle,
              style: const TextStyle(
                fontSize: 17,
                fontWeight: FontWeight.w600,
                color: LibraryTheme.textPrimary,
              ),
            ),
          ),
          Positioned(
            right: 16,
            top: 0,
            bottom: 0,
            child: Row(
              children: [
                _HeaderIcon(
                  icon: Icons.add,
                  tooltip: l10n.installApp,
                  onTap: () => _c.installApk(),
                ),
                const SizedBox(width: 6),
                _HeaderIcon(
                  icon: Icons.close,
                  tooltip: l10n.cancel,
                  onTap: () => SystemNavigator.pop(),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _controls() {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 24),
      child: Row(
        children: [
          Flexible(
            child: LibrarySearchField(controller: _search, store: _store),
          ),
          const SizedBox(width: 12),
          const Spacer(),
          CollectionDropdown(
            store: _store,
            onNewGroup: _newGroup,
            onRenameGroup: _renameGroup,
            onDeleteGroup: _deleteGroup,
          ),
          const SizedBox(width: 10),
          SortDropdown(store: _store),
        ],
      ),
    );
  }

  Widget _body(AppLocalizations l10n) {
    switch (_c.state) {
      case LoadState.loading:
        return _Centered(
          children: [
            const SizedBox(
              width: 28,
              height: 28,
              child: CircularProgressIndicator(
                strokeWidth: 2.5,
                color: LibraryTheme.accent,
              ),
            ),
            const SizedBox(height: 16),
            Text(l10n.loadingApps, style: _muted),
          ],
        );
      case LoadState.failed:
        return _Centered(
          children: [
            const Icon(
              Icons.cloud_off,
              size: 42,
              color: LibraryTheme.textSecondary,
            ),
            const SizedBox(height: 14),
            Text(l10n.loadFailed, style: _muted),
            const SizedBox(height: 16),
            FilledButton(onPressed: _c.init, child: Text(l10n.retry)),
          ],
        );
      case LoadState.ready:
        final apps = _store.visible;
        if (apps.isEmpty) {
          return _Centered(
            children: [
              const Icon(
                Icons.apps_outage,
                size: 42,
                color: LibraryTheme.textSecondary,
              ),
              const SizedBox(height: 14),
              Text(_emptyText(l10n), style: _muted),
            ],
          );
        }
        return AppGrid(
          apps: apps,
          store: _store,
          icons: _c.icons,
          generation: _c.generation,
          systemLabel: l10n.systemBadge,
          onOpen: _open,
          onMenu: _menu,
        );
    }
  }

  String _emptyText(AppLocalizations l10n) {
    if (_store.query.trim().isNotEmpty) {
      return l10n.emptySearch(_store.query.trim());
    }
    if (_store.filter is FilterGroup) return l10n.emptyGroup;
    return l10n.emptyLibrary;
  }

  static const _muted = TextStyle(
    color: LibraryTheme.textSecondary,
    fontSize: 14,
  );

  Future<void> _open(AppEntry app) async {
    if (!await _c.launch(app) && mounted) {
      _toast(l10nOf().launchFailed(app.label));
    }
  }

  Future<void> _menu(AppEntry app, Offset at) async {
    final l10n = l10nOf();
    final inGroup = _store.filter is FilterGroup;
    final picked = await showAppMenu(
      context,
      at,
      menuActionsFor(
        app,
        pinned: _store.isPinned(app.packageName),
        inGroup: inGroup,
      ),
    );
    if (picked == null || !mounted) return;
    switch (picked) {
      case AppAction.open:
        await _open(app);
      case AppAction.pin:
      case AppAction.unpin:
        _store.togglePin(app.packageName);
      case AppAction.addToGroup:
        await _addToGroup(app);
      case AppAction.removeFromGroup:
        final f = _store.filter;
        if (f is FilterGroup) {
          _store.removeFromGroup(f.groupId, app.packageName);
        }
      case AppAction.details:
        if (!mounted) return;
        await showAppDetails(
          context,
          app,
          onAppInfo: () => _c.openAppInfo(app).then((_) {}),
        );
      case AppAction.uninstall:
        if (!await _c.uninstall(app) && mounted) {
          _toast(l10n.uninstallFailed(app.label));
        }
    }
  }

  Future<void> _addToGroup(AppEntry app) async {
    final pick = await pickGroup(context, _store.groups);
    if (pick == null || !mounted) return;
    switch (pick) {
      case GroupPicked(id: final id):
        _store.addToGroup(id, app.packageName);
      case GroupCreateNew():
        final name = await promptGroupName(context);
        if (name != null) {
          final id = _store.createGroup(name);
          _store.addToGroup(id, app.packageName);
        }
    }
  }

  Future<void> _newGroup() async {
    final name = await promptGroupName(context);
    if (name != null && mounted) {
      final id = _store.createGroup(name);
      _store.setFilter(FilterGroup(id));
    }
  }

  Future<void> _renameGroup(AppGroup group) async {
    final name = await promptGroupName(
      context,
      initial: group.name,
      renaming: true,
    );
    if (name != null) _store.renameGroup(group.id, name);
  }

  Future<void> _deleteGroup(AppGroup group) async {
    if (await confirmDeleteGroup(context, group.name)) {
      _store.deleteGroup(group.id);
    }
  }

  AppLocalizations l10nOf() => AppLocalizations.of(context);

  void _toast(String message) {
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(SnackBar(content: Text(message)));
  }
}

class _HeaderIcon extends StatelessWidget {
  const _HeaderIcon({
    required this.icon,
    required this.tooltip,
    required this.onTap,
  });

  final IconData icon;
  final String tooltip;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    return Tooltip(
      message: tooltip,
      child: Material(
        color: LibraryTheme.surface,
        shape: const CircleBorder(),
        child: InkWell(
          customBorder: const CircleBorder(),
          onTap: onTap,
          child: Padding(
            padding: const EdgeInsets.all(8),
            child: Icon(icon, size: 18, color: LibraryTheme.textSecondary),
          ),
        ),
      ),
    );
  }
}

class _Centered extends StatelessWidget {
  const _Centered({required this.children});

  final List<Widget> children;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Column(mainAxisSize: MainAxisSize.min, children: children),
    );
  }
}
