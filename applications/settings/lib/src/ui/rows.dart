import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../catalog.dart';
import '../labels.dart';
import '../models.dart';
import '../settings_controller.dart';
import '../settings_store.dart';
import 'battery_icon.dart';
import 'brand_card.dart';
import 'scan_card.dart';
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
    final enabled = implementedOf(id);
    if (kindOf(id) == ItemKind.brand) {
      return BrandCard(
        name: itemTitle(l10n, id),
        caption: itemDescription(l10n, id),
      );
    }
    if (kindOf(id) == ItemKind.scanCard) {
      final scanning = store.isOn(id);
      return ScanCard(
        title: itemTitle(l10n, id),
        status: scanStatusLabel(l10n, scanning),
        scanning: scanning,
        slots: [
          ScanSlot(
            name: itemTitle(l10n, ItemId.controllerLeft),
            state: controllerLinkLabel(
              l10n,
              store.controllerOf(ItemId.controllerLeft).link,
            ),
          ),
          ScanSlot(
            name: itemTitle(l10n, ItemId.controllerRight),
            state: controllerLinkLabel(
              l10n,
              store.controllerOf(ItemId.controllerRight).link,
            ),
          ),
        ],
        onTap: enabled ? () => controller.runAction(id) : null,
      );
    }
    final row = Padding(
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
          _Control(
            id: id,
            store: store,
            controller: controller,
            enabled: enabled,
          ),
        ],
      ),
    );
    return enabled ? row : Opacity(opacity: 0.45, child: row);
  }
}

class _Control extends StatelessWidget {
  const _Control({
    required this.id,
    required this.store,
    required this.controller,
    required this.enabled,
  });

  final ItemId id;
  final SettingsStore store;
  final SettingsController controller;
  final bool enabled;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    switch (kindOf(id)) {
      case ItemKind.toggle:
        return Switch(
          value: store.isOn(id),
          onChanged: enabled ? (_) => controller.toggleItem(id) : null,
          activeThumbColor: PanelTheme.accent,
        );
      case ItemKind.slider:
        return SizedBox(
          width: 260,
          child: Slider(
            value: store.sliderValue(id),
            onChanged: enabled ? (v) => controller.setSlider(id, v) : null,
            activeColor: PanelTheme.accent,
            inactiveColor: PanelTheme.surfaceHigh,
          ),
        );
      case ItemKind.action:
        return TextButton(
          onPressed: enabled ? () => controller.runAction(id) : null,
          style: TextButton.styleFrom(
            foregroundColor: PanelTheme.accent,
            minimumSize: const Size(0, 34),
            tapTargetSize: MaterialTapTargetSize.shrinkWrap,
          ),
          child: const Icon(Icons.chevron_right, size: 26),
        );
      case ItemKind.scanCard:
      case ItemKind.brand:
        // handled in SettingsRow.build, before the row layout
        return const SizedBox.shrink();
      case ItemKind.controller:
        final info = store.controllerOf(id);
        return Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              controllerLinkLabel(l10n, info.link),
              style: const TextStyle(
                fontSize: 15,
                color: PanelTheme.textSecondary,
              ),
            ),
            const SizedBox(width: 10),
            BatteryIcon(level: info.battery, charging: info.charging),
          ],
        );
      case ItemKind.info:
        final text = store.textOf(id);
        final empty = id == ItemId.wifiSsid
            ? l10n.valueNotConnected
            : l10n.valueUnknown;
        return Text(
          text == null || text.isEmpty ? empty : text,
          style: const TextStyle(fontSize: 15, color: PanelTheme.textSecondary),
        );
    }
  }
}
