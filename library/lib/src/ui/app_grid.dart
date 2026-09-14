import 'package:flutter/material.dart';

import '../library_store.dart';
import '../models.dart';
import '../platform/icon_cache.dart';
import 'app_tile.dart';
import 'theme.dart';

/// The scrollable tile grid. Tiles can be long-press-dragged onto other
/// tiles to reorder; each tile is also a drop target.
class AppGrid extends StatelessWidget {
  const AppGrid({
    super.key,
    required this.apps,
    required this.store,
    required this.icons,
    required this.generation,
    required this.systemLabel,
    required this.onOpen,
    required this.onMenu,
  });

  final List<AppEntry> apps;
  final LibraryStore store;
  final IconCache icons;

  /// Bumped by the controller whenever the catalog is re-read; part of the
  /// tile key so icons are re-decoded after installs/uninstalls.
  final int generation;
  final String systemLabel;
  final ValueChanged<AppEntry> onOpen;
  final void Function(AppEntry app, Offset at) onMenu;

  @override
  Widget build(BuildContext context) {
    return GridView.builder(
      padding: const EdgeInsets.fromLTRB(24, 8, 24, 24),
      gridDelegate: const SliverGridDelegateWithMaxCrossAxisExtent(
        maxCrossAxisExtent: 330,
        crossAxisSpacing: 20,
        mainAxisSpacing: 20,
        childAspectRatio: 1.62,
      ),
      itemCount: apps.length,
      itemBuilder: (context, i) => _GridCell(
        key: ValueKey('cell-$generation-${apps[i].packageName}'),
        index: i,
        app: apps[i],
        store: store,
        icons: icons,
        systemLabel: systemLabel,
        onOpen: onOpen,
        onMenu: onMenu,
      ),
    );
  }
}

class _GridCell extends StatefulWidget {
  const _GridCell({
    super.key,
    required this.index,
    required this.app,
    required this.store,
    required this.icons,
    required this.systemLabel,
    required this.onOpen,
    required this.onMenu,
  });

  final int index;
  final AppEntry app;
  final LibraryStore store;
  final IconCache icons;
  final String systemLabel;
  final ValueChanged<AppEntry> onOpen;
  final void Function(AppEntry app, Offset at) onMenu;

  @override
  State<_GridCell> createState() => _GridCellState();
}

class _GridCellState extends State<_GridCell> {
  bool _dragging = false;
  bool _dropHover = false;

  @override
  Widget build(BuildContext context) {
    final tile = AppTile(
      app: widget.app,
      icons: widget.icons,
      pinned: widget.store.isPinned(widget.app.packageName),
      systemLabel: widget.systemLabel,
      dragging: _dragging,
      onTap: () => widget.onOpen(widget.app),
      onMenu: (at) => widget.onMenu(widget.app, at),
    );

    return DragTarget<AppEntry>(
      onWillAcceptWithDetails: (d) {
        if (d.data.packageName == widget.app.packageName) return false;
        setState(() => _dropHover = true);
        return true;
      },
      onLeave: (_) => setState(() => _dropHover = false),
      onAcceptWithDetails: (d) {
        setState(() => _dropHover = false);
        widget.store.moveApp(d.data.packageName, widget.index);
      },
      builder: (context, candidates, rejected) {
        return AnimatedContainer(
          duration: const Duration(milliseconds: 120),
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(LibraryTheme.tileRadius),
            border: _dropHover
                ? Border.all(color: LibraryTheme.accent, width: 2)
                : null,
          ),
          child: LongPressDraggable<AppEntry>(
            data: widget.app,
            onDragStarted: () => setState(() => _dragging = true),
            onDragEnd: (_) => _clearDrag(),
            onDraggableCanceled: (_, _) => _clearDrag(),
            feedback: Material(
              color: Colors.transparent,
              child: SizedBox(
                width: 300,
                height: 185,
                child: Opacity(opacity: 0.92, child: tile),
              ),
            ),
            childWhenDragging: Opacity(opacity: 0.3, child: tile),
            child: tile,
          ),
        );
      },
    );
  }

  void _clearDrag() {
    if (mounted && _dragging) setState(() => _dragging = false);
  }
}
