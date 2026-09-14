import 'package:flutter/material.dart';

import '../models.dart';
import '../platform/icon_cache.dart';
import 'icon_colors.dart';
import 'theme.dart';

/// One app in the grid: tinted rounded tile with the icon centered, badges
/// top-left, a kebab top-right and the label underneath.
class AppTile extends StatefulWidget {
  const AppTile({
    super.key,
    required this.app,
    required this.icons,
    required this.pinned,
    required this.systemLabel,
    required this.onTap,
    required this.onMenu,
    this.dragging = false,
    this.focused = false,
  });

  final AppEntry app;
  final IconCache icons;
  final bool pinned;
  final String systemLabel;
  final VoidCallback onTap;

  /// Kebab tap / long-press position for anchoring the context menu.
  final ValueChanged<Offset> onMenu;
  final bool dragging;
  final bool focused;

  @override
  State<AppTile> createState() => _AppTileState();
}

class _AppTileState extends State<AppTile> {
  late Future<TileArt> _art = _load();

  Future<TileArt> _load() async {
    final bytes = await widget.icons.get(widget.app.packageName);
    final tint = bytes == null ? null : await dominantIconColor(bytes);
    return TileArt(bytes, tint);
  }

  @override
  void didUpdateWidget(AppTile old) {
    super.didUpdateWidget(old);
    if (old.app.packageName != widget.app.packageName ||
        !identical(old.icons, widget.icons)) {
      _art = _load();
    }
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      behavior: HitTestBehavior.translucent,
      onTap: widget.onTap,
      child: Column(
        children: [
          Expanded(
            child: FutureBuilder<TileArt>(
              future: _art,
              builder: (context, snap) {
                final art = snap.data;
                return _TileFace(
                  art: art,
                  app: widget.app,
                  pinned: widget.pinned,
                  systemLabel: widget.systemLabel,
                  dragging: widget.dragging,
                  focused: widget.focused,
                  onTap: widget.onTap,
                  onMenu: widget.onMenu,
                );
              },
            ),
          ),
          const SizedBox(height: 8),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 6),
            child: Text(
              widget.app.label,
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
              textAlign: TextAlign.center,
              style: const TextStyle(
                color: LibraryTheme.textPrimary,
                fontSize: 14,
                fontWeight: FontWeight.w500,
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _TileFace extends StatelessWidget {
  const _TileFace({
    required this.art,
    required this.app,
    required this.pinned,
    required this.systemLabel,
    required this.dragging,
    required this.focused,
    required this.onTap,
    required this.onMenu,
  });

  final TileArt? art;
  final AppEntry app;
  final bool pinned;
  final String systemLabel;
  final bool dragging;
  final bool focused;
  final VoidCallback onTap;
  final ValueChanged<Offset> onMenu;

  @override
  Widget build(BuildContext context) {
    final gradient = tileGradient(art?.tint);
    return AnimatedScale(
      scale: dragging ? 0.94 : 1.0,
      duration: const Duration(milliseconds: 140),
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 160),
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(LibraryTheme.tileRadius),
          border: focused
              ? Border.all(color: LibraryTheme.accent, width: 2.5)
              : null,
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: gradient,
          ),
          boxShadow: const [
            BoxShadow(
              color: Colors.black38,
              blurRadius: 14,
              offset: Offset(0, 6),
            ),
          ],
        ),
        clipBehavior: Clip.antiAlias,
        child: Material(
          color: Colors.transparent,
          child: InkWell(
            onTap: onTap,
            child: Stack(
              fit: StackFit.expand,
              children: [
                _icon(),
                _badges(),
                Positioned(top: 6, right: 6, child: _kebab()),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _icon() {
    final bytes = art?.bytes;
    if (bytes == null) {
      return Center(
        child: Text(
          app.label.isEmpty ? '?' : app.label.characters.first.toUpperCase(),
          style: const TextStyle(
            color: Colors.white70,
            fontSize: 46,
            fontWeight: FontWeight.w600,
          ),
        ),
      );
    }
    return Center(
      child: FractionallySizedBox(
        widthFactor: 0.42,
        heightFactor: 0.62,
        child: Image.memory(
          bytes,
          fit: BoxFit.contain,
          gaplessPlayback: true,
          frameBuilder: (context, child, frame, sync) => AnimatedOpacity(
            opacity: frame == null ? 0 : 1,
            duration: const Duration(milliseconds: 180),
            child: child,
          ),
        ),
      ),
    );
  }

  Widget _badges() {
    return Positioned(
      top: 8,
      left: 8,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          if (pinned) const _Badge(child: Icon(Icons.push_pin, size: 13)),
          if (app.isSystem)
            _Badge(
              child: Text(
                systemLabel,
                style: const TextStyle(fontSize: 11, color: Colors.white),
              ),
            ),
        ],
      ),
    );
  }

  Widget _kebab() {
    return GestureDetector(
      behavior: HitTestBehavior.opaque,
      onTapDown: (d) => onMenu(d.globalPosition),
      child: const SizedBox(
        width: 30,
        height: 30,
        child: Icon(Icons.more_vert, size: 18, color: Colors.white70),
      ),
    );
  }
}

class _Badge extends StatelessWidget {
  const _Badge({required this.child});

  final Widget child;

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.only(bottom: 6),
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: Colors.black54,
        borderRadius: BorderRadius.circular(10),
      ),
      child: child,
    );
  }
}
