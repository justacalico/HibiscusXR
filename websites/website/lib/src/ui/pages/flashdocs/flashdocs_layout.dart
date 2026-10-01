import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../../flashdocs.dart';
import '../../../routes.dart';
import '../../../theme.dart';
import '../../shell.dart';
import '../../widgets.dart';

/// Shared frame for the flashing docs section: a sidebar with the
/// section's pages on the left of a text column. On mobile the sidebar
/// collapses into a plain list above the content.
class FlashDocsLayout extends StatelessWidget {
  const FlashDocsLayout({
    super.key,
    required this.activePath,
    required this.child,
  });

  /// Path of the sidebar item to highlight.
  final String activePath;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    final sidebar = _SideNav(active: activePath);
    final content = Layout.isMobile(context)
        ? Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              sidebar,
              const SizedBox(height: 32),
              child,
            ],
          )
        : Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              SizedBox(width: 200, child: sidebar),
              const SizedBox(width: 56),
              Expanded(child: child),
            ],
          );
    return PageBody(
      children: [
        Band(
          width: Layout.content,
          padding: const EdgeInsets.symmetric(vertical: 64),
          child: content,
        ),
      ],
    );
  }
}

class _SideNav extends StatelessWidget {
  const _SideNav({required this.active});

  final String active;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _SideItem(
          label: l10n.flashdocsSidebarHome,
          path: Routes.flashdocs,
          active: active == Routes.flashdocs,
        ),
        const SizedBox(height: 20),
        Text(
          l10n.flashdocsDevicesTitle.toUpperCase(),
          style: context.text.labelSmall!.copyWith(
            letterSpacing: 1.4,
            fontWeight: FontWeight.w600,
          ),
        ),
        const SizedBox(height: 4),
        for (final d in flashDocDevices) ...[
          _SideItem(
            label: d.name(l10n),
            path: d.path,
            active: active == d.path,
          ),
          for (final s in d.systems)
            _SideItem(
              label: s.name(l10n),
              path: d.guidePath(s.slug),
              active: active == d.guidePath(s.slug),
              depth: 1,
            ),
        ],
      ],
    );
  }
}

class _SideItem extends StatefulWidget {
  const _SideItem({
    required this.label,
    required this.path,
    required this.active,
    this.depth = 0,
  });

  final String label;
  final String path;
  final bool active;
  final int depth;

  @override
  State<_SideItem> createState() => _SideItemState();
}

class _SideItemState extends State<_SideItem> {
  bool _hover = false;
  bool _focus = false;

  @override
  Widget build(BuildContext context) {
    final active = _hover || _focus;
    return FocusableActionDetector(
      onShowHoverHighlight: (v) => setState(() => _hover = v),
      onShowFocusHighlight: (v) => setState(() => _focus = v),
      actions: activateActions(() => context.go(widget.path)),
      mouseCursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: () => context.go(widget.path),
        child: Container(
          width: double.infinity,
          padding: EdgeInsets.only(
            left: 12 + widget.depth * 18,
            top: 8,
            bottom: 8,
          ),
          decoration: BoxDecoration(
            border: Border(
              left: BorderSide(
                width: 2,
                color: widget.active
                    ? context.colors.primary
                    : context.colors.outline,
              ),
            ),
          ),
          child: Text(
            widget.label,
            style: context.text.labelLarge!.copyWith(
              fontSize: 15,
              color: widget.active || active
                  ? context.colors.onSurface
                  : context.colors.secondary,
              fontWeight:
                  widget.active ? FontWeight.w500 : FontWeight.w400,
            ),
          ),
        ),
      ),
    );
  }
}

/// A tappable card that navigates to [path] - the device and OS pickers
/// in the flashing docs are built from these.
class FlashDocsPickCard extends StatefulWidget {
  const FlashDocsPickCard({
    super.key,
    required this.title,
    required this.subtitle,
    required this.path,
  });

  final String title;
  final String subtitle;
  final String path;

  @override
  State<FlashDocsPickCard> createState() => _FlashDocsPickCardState();
}

class _FlashDocsPickCardState extends State<FlashDocsPickCard> {
  bool _hover = false;
  bool _focus = false;

  @override
  Widget build(BuildContext context) {
    final accent = context.colors.primary;
    final active = _hover || _focus;
    return FocusableActionDetector(
      onShowHoverHighlight: (v) => setState(() => _hover = v),
      onShowFocusHighlight: (v) => setState(() => _focus = v),
      actions: activateActions(() => context.go(widget.path)),
      mouseCursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: () => context.go(widget.path),
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 150),
          padding: const EdgeInsets.all(24),
          decoration: BoxDecoration(
            color: context.colors.surface,
            borderRadius: BorderRadius.circular(20),
            border: Border.all(
              color: active
                  ? accent.withValues(alpha: 0.6)
                  : context.colors.outline,
              width: 1,
            ),
          ),
          child: Row(
            children: [
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      widget.title,
                      style: context.text.titleMedium,
                    ),
                    const SizedBox(height: 6),
                    Text(
                      widget.subtitle,
                      style: context.text.bodyMedium!.copyWith(fontSize: 15),
                    ),
                  ],
                ),
              ),
              const SizedBox(width: 16),
              Icon(Icons.chevron_right, color: accent, size: 22),
            ],
          ),
        ),
      ),
    );
  }
}
