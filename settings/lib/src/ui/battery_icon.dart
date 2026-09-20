import 'package:flutter/material.dart';

import 'theme.dart';

/// Small battery gauge for controller rows. `level` is the 0-5 bar
/// value the controller reports; out-of-range and negative values draw
/// an empty outline instead of pretending a number we don't have.
class BatteryIcon extends StatelessWidget {
  const BatteryIcon({super.key, required this.level, this.charging = false});

  final int level;
  final bool charging;

  IconData get _icon {
    if (charging) return Icons.battery_charging_full;
    return switch (level) {
      >= 5 => Icons.battery_full,
      4 => Icons.battery_5_bar,
      3 => Icons.battery_4_bar,
      2 => Icons.battery_3_bar,
      1 => Icons.battery_2_bar,
      0 => Icons.battery_0_bar,
      _ => Icons.battery_unknown,
    };
  }

  @override
  Widget build(BuildContext context) => Icon(
    _icon,
    size: 22,
    color: level == 1 && !charging
        ? Colors.orangeAccent
        : PanelTheme.textSecondary,
  );
}
