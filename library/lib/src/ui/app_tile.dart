import 'dart:typed_data';

import 'package:flutter/material.dart';

import '../models.dart';
import '../platform/icon_cache.dart';
import 'glass.dart';
import 'theme.dart';

/// One app in the grid: a circular glass disc holding the round icon, with
/// the label underneath. Focus scales the disc up, gaze-hover style.
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
  late Future<Uint8List?> _icon = widget.icons.get(widget.app.packageName);

  @override
  void didUpdateWidget(AppTile old) {
    super.didUpdateWidget(old);
    if (old.app.packageName != widget.app.packageName ||
        !identical(old.icons, widget.icons)) {
      _icon = widget.icons.get(widget.app.packageName);
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
            child: FutureBuilder<Uint8List?>(
              future: _icon,
              builder: (context, snap) {
                return _Disc(
                  icon: snap.data,
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
          const SizedBox(height: 10),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 4),
            child: Text(
              widget.app.label,
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
              textAlign: TextAlign.center,
              style: TextStyle(
                color: widget.focused
                    ? LibraryTheme.textPrimary
                    : const Color(0xD9FFFFFF),
                fontSize: 14,
                fontWeight: FontWeight.w500,
                letterSpacing: 0.1,
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _Disc extends StatelessWidget {
  const _Disc({
    required this.icon,
    required this.app,
    required this.pinned,
    required this.systemLabel,
    required this.dragging,
    required this.focused,
    required this.onTap,
    required this.onMenu,
  });

  final Uint8List? icon;
  final AppEntry app;
  final bool pinned;
  final String systemLabel;
  final bool dragging;
  final bool focused;
  final VoidCallback onTap;
  final ValueChanged<Offset> onMenu;

  @override
  Widget build(BuildContext context) {
    return AnimatedScale(
      scale: dragging ? 0.9 : (focused ? 1.07 : 1.0),
      duration: const Duration(milliseconds: 160),
      curve: Curves.easeOutCubic,
      child: Center(
        child: AspectRatio(
          aspectRatio: 1,
          child: Stack(
            clipBehavior: Clip.none,
            children: [
              // the disc
              Positioned.fill(
                child: AnimatedContainer(
                  duration: const Duration(milliseconds: 160),
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    gradient: const LinearGradient(
                      begin: Alignment.topCenter,
                      end: Alignment.bottomCenter,
                      colors: [Color(0x1FFFFFFF), Color(0x0AFFFFFF)],
                    ),
                    border: Border.all(
                      color: focused
                          ? LibraryTheme.glassStrokeHi
                          : LibraryTheme.glassStroke,
                      width: 1,
                    ),
                    boxShadow: [
                      BoxShadow(
                        color: focused
                            ? const Color(0x59000000)
                            : const Color(0x3D000000),
                        blurRadius: focused ? 28 : 18,
                        offset: Offset(0, focused ? 14 : 8),
                      ),
                    ],
                  ),
                ),
              ),
              // icon clipped round, fills the disc like a visionOS icon
              Positioned.fill(
                child: Material(
                  color: Colors.transparent,
                  shape: const CircleBorder(),
                  clipBehavior: Clip.antiAlias,
                  child: InkWell(
                    onTap: onTap,
                    child: Padding(
                      padding: const EdgeInsets.all(6),
                      child: _iconImage(),
                    ),
                  ),
                ),
              ),
              _badges(),
              Positioned(top: 2, right: 2, child: _kebab()),
            ],
          ),
        ),
      ),
    );
  }

  Widget _iconImage() {
    final bytes = icon;
    if (bytes == null) {
      return Center(
        child: Text(
          app.label.isEmpty ? '?' : app.label.characters.first.toUpperCase(),
          style: const TextStyle(
            color: Colors.white70,
            fontSize: 40,
            fontWeight: FontWeight.w600,
          ),
        ),
      );
    }
    return ClipOval(
      child: SizedBox.expand(
        child: Image.memory(
          bytes,
          fit: BoxFit.cover,
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
    if (!pinned && !app.isSystem) return const SizedBox.shrink();
    return Positioned(
      top: 4,
      left: 4,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          if (pinned)
            const _Chip(child: Icon(Icons.push_pin, size: 12)),
          if (app.isSystem)
            _Chip(
              child: Text(
                systemLabel,
                style: const TextStyle(fontSize: 10, color: Colors.white),
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
      child: AnimatedOpacity(
        duration: const Duration(milliseconds: 140),
        opacity: focused ? 1.0 : 0.55,
        child: const Glass(
          circle: true,
          padding: EdgeInsets.all(5),
          child: Icon(Icons.more_horiz, size: 15, color: Colors.white70),
        ),
      ),
    );
  }
}

class _Chip extends StatelessWidget {
  const _Chip({required this.child});

  final Widget child;

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.only(bottom: 5),
      padding: const EdgeInsets.symmetric(horizontal: 7, vertical: 4),
      decoration: BoxDecoration(
        color: const Color(0xB31E252E),
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: LibraryTheme.glassStroke),
      ),
      child: child,
    );
  }
}
