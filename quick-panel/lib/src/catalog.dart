import 'package:flutter/material.dart';

import 'models.dart';

enum TileKind { toggle, action }

/// One tile in the panel grid: what it does, its icon, and whether it
/// renders as a large card or a small button. Order in [panelTiles] is
/// display order.
class TileSpec {
  const TileSpec.toggle(
    this.toggleId, {
    required this.icon,
    this.large = false,
    this.implemented = true,
  }) : kind = TileKind.toggle,
       actionId = null;

  const TileSpec.action(
    this.actionId, {
    required this.icon,
    this.large = false,
    this.implemented = true,
  }) : kind = TileKind.action,
       toggleId = null;

  final TileKind kind;
  final ToggleId? toggleId;
  final ActionId? actionId;
  final IconData icon;
  final bool large;

  /// False marks a tile whose platform side does not exist yet - it
  /// renders greyed out and inert until the OS layer catches up.
  final bool implemented;
}

/// Panel layout. Large row first, then the small rows - mirrors the
/// Quest-style quick settings layout minus the platform-specific tiles
/// (Quest Link, multi-window, voice commands).
const panelTiles = <TileSpec>[
  TileSpec.toggle(ToggleId.wifi, icon: Icons.wifi, large: true),
  TileSpec.toggle(
    ToggleId.boundary,
    icon: Icons.crop_free,
    large: true,
    implemented: false,
  ),
  TileSpec.toggle(ToggleId.bluetooth, icon: Icons.bluetooth, large: true),
  TileSpec.toggle(
    ToggleId.seethrough,
    icon: Icons.visibility,
    large: true,
    implemented: false,
  ),
  TileSpec.toggle(ToggleId.microphone, icon: Icons.mic),
  TileSpec.action(
    ActionId.resetView,
    icon: Icons.center_focus_strong,
    implemented: false,
  ),
  TileSpec.toggle(ToggleId.nightMode, icon: Icons.nightlight_round),
  TileSpec.toggle(ToggleId.doNotDisturb, icon: Icons.dark_mode),
  TileSpec.toggle(ToggleId.airplaneMode, icon: Icons.airplanemode_active),
  TileSpec.toggle(ToggleId.batterySaver, icon: Icons.battery_saver),
  TileSpec.action(
    ActionId.reportProblem,
    icon: Icons.flag_outlined,
    implemented: false,
  ),
  TileSpec.action(ActionId.aboutDevice, icon: Icons.info_outline),
];

List<TileSpec> get largeTiles =>
    panelTiles.where((t) => t.large).toList(growable: false);

List<TileSpec> get smallTiles =>
    panelTiles.where((t) => !t.large).toList(growable: false);
