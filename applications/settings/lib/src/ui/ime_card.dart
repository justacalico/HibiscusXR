import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../models.dart';
import '../settings_controller.dart';
import 'theme.dart';

/// Installed input methods as a radio list. Picking one writes the
/// secure default on the platform side - no trip to the system
/// keyboard settings page.
class ImeCard extends StatelessWidget {
  const ImeCard({
    super.key,
    required this.title,
    required this.controller,
    this.enabled = true,
  });

  final String title;
  final SettingsController controller;
  final bool enabled;

  @override
  Widget build(BuildContext context) {
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
                  Icon(Icons.keyboard, size: 24, color: PanelTheme.accent),
                  const SizedBox(width: 14),
                  Text(
                    title,
                    style: TextStyle(
                      fontSize: 17,
                      color: PanelTheme.textPrimary,
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 14),
              for (final ime in controller.store.imeOptions)
                _ImeRow(ime: ime, enabled: enabled, controller: controller),
            ],
          ),
        ),
      ),
    );
  }
}

class _ImeRow extends StatelessWidget {
  const _ImeRow({
    required this.ime,
    required this.enabled,
    required this.controller,
  });

  final ImeOption ime;
  final bool enabled;
  final SettingsController controller;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return InkWell(
      borderRadius: BorderRadius.circular(10),
      onTap: enabled && !ime.active
          ? () => controller.setIme(ime.id)
          : null,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
        child: Row(
          children: [
            Icon(
              ime.active
                  ? Icons.radio_button_checked
                  : Icons.radio_button_off,
              size: 20,
              color: ime.active
                  ? PanelTheme.accent
                  : PanelTheme.textSecondary,
            ),
            const SizedBox(width: 14),
            Expanded(
              child: Text(
                ime.label,
                overflow: TextOverflow.ellipsis,
                style: TextStyle(
                  fontSize: 15,
                  color: PanelTheme.textPrimary,
                ),
              ),
            ),
            if (ime.active)
              Text(
                l10n.imeActive,
                style: TextStyle(fontSize: 12, color: PanelTheme.accent),
              ),
          ],
        ),
      ),
    );
  }
}
