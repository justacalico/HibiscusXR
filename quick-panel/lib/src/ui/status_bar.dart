import 'package:flutter/material.dart';
import 'package:intl/intl.dart';

import '../../l10n/app_localizations.dart';
import 'theme.dart';

/// Top strip of the panel: battery on the left, date centered,
/// settings gear on the right.
class PanelStatusBar extends StatelessWidget {
  const PanelStatusBar({
    super.key,
    required this.batteryLevel,
    required this.now,
    required this.onSettings,
  });

  final int batteryLevel;
  final DateTime now;
  final VoidCallback onSettings;

  IconData get _batteryIcon {
    if (batteryLevel >= 95) return Icons.battery_full;
    if (batteryLevel >= 60) return Icons.battery_5_bar;
    if (batteryLevel >= 30) return Icons.battery_3_bar;
    return Icons.battery_1_bar;
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return SizedBox(
      height: 40,
      child: Stack(
        alignment: Alignment.center,
        children: [
          Align(
            alignment: Alignment.centerLeft,
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(_batteryIcon, size: 22, color: PanelTheme.textPrimary),
                const SizedBox(width: 6),
                Text(
                  l10n.batteryPercent(batteryLevel),
                  style: const TextStyle(
                    fontSize: 14,
                    color: PanelTheme.textPrimary,
                  ),
                ),
              ],
            ),
          ),
          Text(
            DateFormat('E, MMM d, y').format(now),
            style: const TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.w500,
              color: PanelTheme.textPrimary,
            ),
          ),
          Align(
            alignment: Alignment.centerRight,
            child: IconPill(
              icon: Icons.settings,
              tooltip: l10n.settings,
              onTap: onSettings,
            ),
          ),
        ],
      ),
    );
  }
}

/// Small circular icon button used in the status bar and footer.
class IconPill extends StatelessWidget {
  const IconPill({
    super.key,
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
      child: InkWell(
        onTap: onTap,
        customBorder: const CircleBorder(),
        child: Padding(
          padding: const EdgeInsets.all(6),
          child: Icon(icon, size: 20, color: PanelTheme.textPrimary),
        ),
      ),
    );
  }
}
