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
class SettingsPage extends StatelessWidget {
  const SettingsPage({super.key, required this.controller});

  final SettingsController controller;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Scaffold(
      body: AnimatedBuilder(
        animation: controller.store,
        builder: (context, _) => Row(
          children: [
            _Sidebar(controller: controller),
            const VerticalDivider(width: 1, thickness: 1),
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
                        for (final id
                            in sectionDef(controller.store.section).items)
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
    );
  }
}

class _Sidebar extends StatelessWidget {
  const _Sidebar({required this.controller});

  final SettingsController controller;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return SizedBox(
      width: 300,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
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
          ),
          Expanded(
            child: ListView(
              padding: const EdgeInsets.symmetric(horizontal: 12),
              children: [
                for (final s in kSections)
                  _SidebarTile(
                    section: s.id,
                    selected: controller.store.section == s.id,
                    controller: controller,
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
  });

  final SectionId section;
  final bool selected;
  final SettingsController controller;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Padding(
      padding: const EdgeInsets.only(bottom: 2),
      child: Material(
        color: selected ? PanelTheme.surface : Colors.transparent,
        borderRadius: BorderRadius.circular(12),
        child: InkWell(
          borderRadius: BorderRadius.circular(12),
          onTap: () => controller.selectSection(section),
          child: Padding(
            padding: const EdgeInsets.symmetric(
              horizontal: 14,
              vertical: 13,
            ),
            child: Row(
              children: [
                Icon(
                  iconFor(section),
                  size: 20,
                  color: selected
                      ? PanelTheme.textPrimary
                      : PanelTheme.textSecondary,
                ),
                const SizedBox(width: 14),
                Expanded(
                  child: Text(
                    sectionTitle(l10n, section),
                    style: TextStyle(
                      fontSize: 15,
                      fontWeight:
                          selected ? FontWeight.w600 : FontWeight.w400,
                      color: selected
                          ? PanelTheme.textPrimary
                          : PanelTheme.textSecondary,
                    ),
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
