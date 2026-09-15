import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../catalog.dart';
import '../labels.dart';
import '../settings_store.dart';
import 'theme.dart';

/// One tile in the panel grid. Toggles show state, actions just fire.
/// Focusable so controllers/d-pads can walk the grid.
class SettingTile extends StatefulWidget {
  const SettingTile({
    super.key,
    required this.spec,
    required this.store,
    required this.onTap,
    this.autofocus = false,
  });

  final TileSpec spec;
  final SettingsStore store;
  final VoidCallback onTap;
  final bool autofocus;

  @override
  State<SettingTile> createState() => _SettingTileState();
}

class _SettingTileState extends State<SettingTile> {
  var _focused = false;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final spec = widget.spec;
    final on = spec.toggleId != null && widget.store.isOn(spec.toggleId!);
    final label = tileLabel(spec, l10n);
    final bg = on
        ? PanelTheme.accent.withValues(alpha: 0.28)
        : PanelTheme.surface;
    final iconColor = on ? PanelTheme.accent : PanelTheme.textPrimary;
    final radius = spec.large
        ? PanelTheme.tileRadius
        : PanelTheme.smallTileRadius;

    return FocusableActionDetector(
      autofocus: widget.autofocus,
      onShowFocusHighlight: (f) => setState(() => _focused = f),
      actions: {
        ActivateIntent: CallbackAction<ActivateIntent>(
          onInvoke: (_) {
            widget.onTap();
            return null;
          },
        ),
      },
      child: Material(
        color: bg,
        borderRadius: BorderRadius.circular(radius),
        child: InkWell(
          onTap: widget.onTap,
          borderRadius: BorderRadius.circular(radius),
          child: Container(
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(radius),
              border: _focused
                  ? Border.all(color: PanelTheme.accent, width: 2)
                  : null,
            ),
            padding: EdgeInsets.all(spec.large ? 14 : 8),
            child: spec.large
                ? _large(label, iconColor, l10n)
                : _small(label, iconColor),
          ),
        ),
      ),
    );
  }

  Widget _large(String label, Color iconColor, AppLocalizations l10n) {
    final subtitle = widget.store.subtitleFor(widget.spec.toggleId!);
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Icon(widget.spec.icon, size: 22, color: iconColor),
        const Spacer(),
        Text(
          label,
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          style: const TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.w600,
            color: PanelTheme.textPrimary,
          ),
        ),
        Text(
          subtitleText(subtitle, l10n),
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          style: const TextStyle(fontSize: 12, color: PanelTheme.textSecondary),
        ),
      ],
    );
  }

  Widget _small(String label, Color iconColor) {
    final spec = widget.spec;
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        Icon(spec.icon, size: 20, color: iconColor),
        const SizedBox(height: 6),
        Text(
          label,
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          textAlign: TextAlign.center,
          style: const TextStyle(fontSize: 12, color: PanelTheme.textSecondary),
        ),
      ],
    );
  }
}
