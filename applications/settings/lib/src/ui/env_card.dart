import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../envs/env_info.dart';
import '../settings_controller.dart';
import 'theme.dart';

/// Installed home environments as a radio list: passthrough and the
/// built-in scene first, then every zip found in the shared env dir.
/// Picking a row writes the hibiscus_environment global key; the trash
/// button deletes the zip.
class EnvCard extends StatelessWidget {
  const EnvCard({
    super.key,
    required this.title,
    required this.controller,
    this.enabled = true,
  });

  final String title;
  final SettingsController controller;
  final bool enabled;

  Future<void> _confirmRemove(BuildContext context, EnvOption env) async {
    final l10n = AppLocalizations.of(context);
    final ok = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(l10n.envRemoveTitle(env.label)),
        content: Text(l10n.envRemoveBody),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(false),
            child: Text(MaterialLocalizations.of(context).cancelButtonLabel),
          ),
          TextButton(
            onPressed: () => Navigator.of(context).pop(true),
            child: Text(l10n.envRemove),
          ),
        ],
      ),
    );
    if (ok == true) controller.deleteEnv(env.id);
  }

  String _subtitle(EnvOption env) {
    final parts = [
      if (env.version.isNotEmpty) 'v${env.version}',
      if (env.license.isNotEmpty) env.license,
      if (env.homepage.isNotEmpty) env.homepage else env.git,
      if (env.created.isNotEmpty) env.created,
    ];
    return parts.join('  ·  ');
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final selected = controller.store.homeEnv;
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
      child: Material(
        color: PanelTheme.surface,
        borderRadius: BorderRadius.circular(16),
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  Icon(
                    Icons.landscape_outlined,
                    size: 24,
                    color: PanelTheme.accent,
                  ),
                  const SizedBox(width: 14),
                  Expanded(
                    child: Text(
                      title,
                      style: TextStyle(
                        fontSize: 17,
                        color: PanelTheme.textPrimary,
                      ),
                    ),
                  ),
                  IconButton(
                    onPressed: enabled ? controller.refreshEnvs : null,
                    icon: const Icon(Icons.refresh, size: 20),
                    color: PanelTheme.textSecondary,
                    tooltip: l10n.btRescan,
                  ),
                ],
              ),
              const SizedBox(height: 6),
              _FixedRow(
                icon: Icons.videocam_outlined,
                name: l10n.envPassthrough,
                subtitle: l10n.envPassthroughDesc,
                selected: selected == kEnvPassthrough,
                onTap: enabled
                    ? () => controller.selectEnv(kEnvPassthrough)
                    : null,
              ),
              _FixedRow(
                icon: Icons.filter_hdr_outlined,
                name: l10n.envBuiltin,
                subtitle: l10n.envBuiltinDesc,
                selected: selected == kEnvBuiltin,
                onTap: enabled
                    ? () => controller.selectEnv(kEnvBuiltin)
                    : null,
              ),
              if (controller.store.envOptions.isEmpty)
                Padding(
                  padding: const EdgeInsets.symmetric(vertical: 10),
                  child: Text(
                    l10n.envEmpty,
                    style: TextStyle(
                      fontSize: 13,
                      color: PanelTheme.textSecondary,
                    ),
                  ),
                )
              else
                for (final env in controller.store.envOptions)
                  _EnvRow(
                    env: env,
                    subtitle: _subtitle(env),
                    selected: selected == env.id,
                    enabled: enabled,
                    onRemove: () => _confirmRemove(context, env),
                    onTap: () => controller.selectEnv(env.id),
                  ),
            ],
          ),
        ),
      ),
    );
  }
}

/// One of the fixed scene choices - passthrough or the built-in dome.
class _FixedRow extends StatelessWidget {
  const _FixedRow({
    required this.icon,
    required this.name,
    required this.subtitle,
    required this.selected,
    this.onTap,
  });

  final IconData icon;
  final String name;
  final String subtitle;
  final bool selected;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    return InkWell(
      borderRadius: BorderRadius.circular(10),
      onTap: onTap,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
        child: Row(
          children: [
            Icon(
              selected
                  ? Icons.radio_button_checked
                  : Icons.radio_button_off,
              size: 20,
              color: selected
                  ? PanelTheme.accent
                  : PanelTheme.textSecondary,
            ),
            const SizedBox(width: 14),
            Icon(icon, size: 20, color: PanelTheme.textSecondary),
            const SizedBox(width: 14),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    name,
                    style: TextStyle(
                      fontSize: 15,
                      color: PanelTheme.textPrimary,
                    ),
                  ),
                  Text(
                    subtitle,
                    style: TextStyle(
                      fontSize: 12,
                      color: PanelTheme.textSecondary,
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

/// One installed environment zip: map.png thumb when it ships one,
/// name + version on top, license and links underneath. Zips missing
/// map.obj or carrying an unusable filename list but can't be picked.
class _EnvRow extends StatelessWidget {
  const _EnvRow({
    required this.env,
    required this.subtitle,
    required this.selected,
    required this.enabled,
    required this.onRemove,
    required this.onTap,
  });

  final EnvOption env;
  final String subtitle;
  final bool selected;
  final bool enabled;
  final VoidCallback onRemove;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final reason = !env.readable
        ? l10n.envUnreadable
        : !env.hasMap
        ? l10n.envMissingMap
        : !envIdValid(env.id)
        ? l10n.envBadName
        : null;
    final tappable = enabled && reason == null;
    return Opacity(
      opacity: reason == null ? 1.0 : 0.5,
      child: InkWell(
        borderRadius: BorderRadius.circular(10),
        onTap: tappable ? onTap : null,
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
          child: Row(
            children: [
              Icon(
                selected
                    ? Icons.radio_button_checked
                    : Icons.radio_button_off,
                size: 20,
                color: selected
                    ? PanelTheme.accent
                    : PanelTheme.textSecondary,
              ),
              const SizedBox(width: 14),
              ClipRRect(
                borderRadius: BorderRadius.circular(8),
                child: SizedBox(
                  width: 44,
                  height: 44,
                  child: env.thumb != null
                      ? Image.memory(env.thumb!, fit: BoxFit.cover)
                      : ColoredBox(
                          color: PanelTheme.surfaceHigh,
                          child: Icon(
                            Icons.landscape_outlined,
                            size: 22,
                            color: PanelTheme.textSecondary,
                          ),
                        ),
                ),
              ),
              const SizedBox(width: 14),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      env.label,
                      overflow: TextOverflow.ellipsis,
                      style: TextStyle(
                        fontSize: 15,
                        color: PanelTheme.textPrimary,
                      ),
                    ),
                    Text(
                      reason ??
                          (subtitle.isEmpty ? env.id : subtitle),
                      overflow: TextOverflow.ellipsis,
                      style: TextStyle(
                        fontSize: 12,
                        color: PanelTheme.textSecondary,
                      ),
                    ),
                  ],
                ),
              ),
              IconButton(
                onPressed: enabled ? onRemove : null,
                icon: const Icon(Icons.delete_outline, size: 20),
                color: PanelTheme.textSecondary,
              ),
            ],
          ),
        ),
      ),
    );
  }
}
