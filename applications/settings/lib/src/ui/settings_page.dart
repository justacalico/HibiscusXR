import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../catalog.dart';
import '../labels.dart';
import '../models.dart';
import '../settings_controller.dart';
import 'icons.dart';
import 'rows.dart';
import 'theme.dart';

/// Quest-style two-pane settings surface: a sidebar of sections on the
/// left, the selected section's rows on the right.
class SettingsPage extends StatefulWidget {
  const SettingsPage({
    super.key,
    required this.controller,
    this.uiOnlyMode = false,
  });

  final SettingsController controller;
  final bool uiOnlyMode;

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  @override
  void initState() {
    super.initState();
    if (widget.uiOnlyMode) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        showDialog<void>(
          context: context,
          builder: (context) {
            final l10n = AppLocalizations.of(context);
            return AlertDialog(
              title: Text(l10n.uiOnlyModeTitle),
              content: Text(l10n.uiOnlyModeBody),
              actions: [
                TextButton(
                  onPressed: () => Navigator.of(context).pop(),
                  child: Text(MaterialLocalizations.of(context).okButtonLabel),
                ),
              ],
            );
          },
        );
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    final controller = widget.controller;
    final l10n = AppLocalizations.of(context);
    return Scaffold(
      body: LayoutBuilder(
        builder: (context, constraints) => AnimatedBuilder(
          animation: controller.store,
          builder: (context, _) => Row(
            children: [
              _Sidebar(
                controller: controller,
                compact: constraints.maxWidth < 640,
              ),
              const VerticalDivider(
                width: 1,
                thickness: 1,
                color: PanelTheme.panel,
              ),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Padding(
                      padding: const EdgeInsets.fromLTRB(28, 26, 28, 8),
                      child: Text(
                        sectionTitle(l10n, controller.store.section),
                        style: const TextStyle(
                          fontSize: 15,
                          fontWeight: FontWeight.w600,
                          color: PanelTheme.textSecondary,
                        ),
                      ),
                    ),
                    Expanded(
                      child: ListView(
                        children: [
                          for (final id in sectionDef(
                            controller.store.section,
                          ).items)
                            SettingsRow(id: id, controller: controller),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _Sidebar extends StatelessWidget {
  const _Sidebar({required this.controller, this.compact = false});

  final SettingsController controller;

  /// Narrow windows drop the sidebar to an icon rail so the content
  /// pane keeps enough room for rows and their controls.
  final bool compact;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return SizedBox(
      width: compact ? 76 : 300,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          if (!compact)
            Padding(
              padding: const EdgeInsets.fromLTRB(20, 24, 20, 14),
              child: Text(
                l10n.appTitle,
                style: const TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.w700,
                  color: PanelTheme.textPrimary,
                ),
              ),
            )
          else
            const SizedBox(height: 14),
          Expanded(
            child: ListView(
              padding: const EdgeInsets.symmetric(horizontal: 12),
              children: [
                for (final s in kSections)
                  _SidebarTile(
                    section: s.id,
                    selected: controller.store.section == s.id,
                    controller: controller,
                    compact: compact,
                  ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _SidebarTile extends StatelessWidget {
  const _SidebarTile({
    required this.section,
    required this.selected,
    required this.controller,
    this.compact = false,
  });

  final SectionId section;
  final bool selected;
  final SettingsController controller;
  final bool compact;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final title = sectionTitle(l10n, section);
    final tile = InkWell(
      borderRadius: BorderRadius.circular(12),
      onTap: () => controller.selectSection(section),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 13),
        child: Row(
          mainAxisAlignment: compact
              ? MainAxisAlignment.center
              : MainAxisAlignment.start,
          children: [
            Icon(
              iconFor(section),
              size: 20,
              color: selected
                  ? PanelTheme.textPrimary
                  : PanelTheme.textSecondary,
            ),
            if (!compact) ...[
              const SizedBox(width: 14),
              Expanded(
                child: Text(
                  title,
                  style: TextStyle(
                    fontSize: 15,
                    fontWeight: selected ? FontWeight.w600 : FontWeight.w400,
                    color: selected
                        ? PanelTheme.textPrimary
                        : PanelTheme.textSecondary,
                  ),
                ),
              ),
            ],
          ],
        ),
      ),
    );
    return Padding(
      padding: const EdgeInsets.only(bottom: 2),
      child: Material(
        color: selected ? PanelTheme.surface : Colors.transparent,
        borderRadius: BorderRadius.circular(12),
        child: compact ? Tooltip(message: title, child: tile) : tile,
      ),
    );
  }
}
