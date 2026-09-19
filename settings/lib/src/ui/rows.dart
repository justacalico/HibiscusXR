import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../catalog.dart';
import '../labels.dart';
import '../models.dart';
import '../settings_controller.dart';
import '../settings_store.dart';
import 'theme.dart';

/// One row in a section page: title + description on the left, the
/// control the row's kind calls for on the right.
class SettingsRow extends StatelessWidget {
  const SettingsRow({super.key, required this.id, required this.controller});

  final ItemId id;
  final SettingsController controller;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final store = controller.store;
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 28, vertical: 18),
      child: Row(
        children: [
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  itemTitle(l10n, id),
                  style: const TextStyle(
                    fontSize: 17,
                    color: PanelTheme.textPrimary,
                  ),
                ),
                const SizedBox(height: 3),
                Text(
                  itemDescription(l10n, id),
                  style: const TextStyle(
                    fontSize: 12,
                    color: PanelTheme.textSecondary,
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(width: 24),
          _Control(id: id, store: store, controller: controller),
        ],
      ),
    );
  }
}

class _Control extends StatelessWidget {
  const _Control({
    required this.id,
    required this.store,
    required this.controller,
  });

  final ItemId id;
  final SettingsStore store;
  final SettingsController controller;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    switch (kindOf(id)) {
      case ItemKind.toggle:
        return Switch(
          value: store.isOn(id),
          onChanged: (_) => controller.toggleItem(id),
          activeThumbColor: PanelTheme.accent,
        );
      case ItemKind.slider:
        return SizedBox(
          width: 260,
          child: Slider(
            value: store.sliderValue(id),
            onChanged: (v) => controller.setSlider(id, v),
            activeColor: PanelTheme.accent,
            inactiveColor: PanelTheme.surfaceHigh,
          ),
        );
      case ItemKind.choice:
        return DropdownButton<String>(
          value: store.choiceOf(id),
          dropdownColor: PanelTheme.surface,
          underline: const SizedBox.shrink(),
          style: const TextStyle(color: PanelTheme.textPrimary, fontSize: 15),
          items: [
            for (final v in optionsOf(id))
              DropdownMenuItem(value: v, child: Text(choiceLabel(l10n, v))),
          ],
          onChanged: (v) {
            if (v != null) controller.selectChoice(id, v);
          },
        );
      case ItemKind.action:
        return TextButton(
          onPressed: () => controller.runAction(id),
          style: TextButton.styleFrom(
            foregroundColor: PanelTheme.accent,
            minimumSize: const Size(0, 34),
            tapTargetSize: MaterialTapTargetSize.shrinkWrap,
          ),
          child: const Icon(Icons.chevron_right, size: 26),
        );
      case ItemKind.info:
        final text = store.textOf(id);
        final empty = id == ItemId.wifiSsid
            ? l10n.valueNotConnected
            : l10n.valueUnknown;
        return Text(
          text == null || text.isEmpty ? empty : text,
          style: const TextStyle(
            fontSize: 15,
            color: PanelTheme.textSecondary,
          ),
        );
    }
  }
}
