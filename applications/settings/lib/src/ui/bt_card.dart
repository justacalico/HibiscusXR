import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../labels.dart';
import '../models.dart';
import '../settings_controller.dart';
import 'theme.dart';

/// Full-width card for the bluetooth device list: bonded devices and
/// whatever the running discovery has found. The header doubles as the
/// discovery control, so no row bounces out to the system bluetooth
/// page.
class BtCard extends StatelessWidget {
  const BtCard({
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
    final l10n = AppLocalizations.of(context);
    final store = controller.store;
    final discovering = store.btDiscovering;
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
                  SizedBox(
                    width: 26,
                    height: 26,
                    child: discovering
                        ? CircularProgressIndicator(
                            strokeWidth: 2.4,
                            color: PanelTheme.accent,
                          )
                        : Icon(
                            Icons.bluetooth,
                            size: 24,
                            color: PanelTheme.accent,
                          ),
                  ),
                  const SizedBox(width: 14),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          title,
                          style: TextStyle(
                            fontSize: 17,
                            color: PanelTheme.textPrimary,
                          ),
                        ),
                        const SizedBox(height: 3),
                        Text(
                          btScanLabel(l10n, discovering),
                          style: TextStyle(
                            fontSize: 12,
                            color: discovering
                                ? PanelTheme.accent
                                : PanelTheme.textSecondary,
                          ),
                        ),
                      ],
                    ),
                  ),
                  TextButton(
                    onPressed: enabled && !discovering
                        ? () => controller.scanBt()
                        : null,
                    style: TextButton.styleFrom(
                      foregroundColor: PanelTheme.accent,
                    ),
                    child: Text(l10n.btRescan),
                  ),
                ],
              ),
              const SizedBox(height: 14),
              if (store.btDevices.isEmpty)
                Text(
                  l10n.btEmpty,
                  style: TextStyle(
                    fontSize: 13,
                    color: PanelTheme.textSecondary,
                  ),
                )
              else
                for (final dev in store.btDevices)
                  _DeviceRow(
                    dev: dev,
                    enabled: enabled,
                    onTap: () => dev.bonded
                        ? _forgetDialog(context, controller, dev)
                        : controller.pairBt(dev.address),
                  ),
            ],
          ),
        ),
      ),
    );
  }
}

class _DeviceRow extends StatelessWidget {
  const _DeviceRow({
    required this.dev,
    required this.enabled,
    required this.onTap,
  });

  final BtDevice dev;
  final bool enabled;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return InkWell(
      borderRadius: BorderRadius.circular(10),
      onTap: enabled ? onTap : null,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
        child: Row(
          children: [
            Icon(
              dev.connected ? Icons.bluetooth_connected : Icons.bluetooth,
              size: 20,
              color: dev.connected
                  ? PanelTheme.accent
                  : PanelTheme.textSecondary,
            ),
            const SizedBox(width: 14),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    dev.name,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      fontSize: 15,
                      color: PanelTheme.textPrimary,
                    ),
                  ),
                  Text(
                    btDeviceLabel(l10n, dev),
                    style: TextStyle(
                      fontSize: 12,
                      color: dev.connected
                          ? PanelTheme.accent
                          : PanelTheme.textSecondary,
                    ),
                  ),
                ],
              ),
            ),
            if (!dev.bonded)
              Text(
                l10n.btPair,
                style: TextStyle(
                  fontSize: 13,
                  fontWeight: FontWeight.w600,
                  color: PanelTheme.accent,
                ),
              )
            else
              Icon(
                Icons.chevron_right,
                size: 22,
                color: PanelTheme.textSecondary,
              ),
          ],
        ),
      ),
    );
  }
}

/// Bonded rows confirm before the bond is dropped.
Future<void> _forgetDialog(
  BuildContext context,
  SettingsController controller,
  BtDevice dev,
) async {
  final l10n = AppLocalizations.of(context);
  final forget = await showDialog<bool>(
    context: context,
    builder: (context) => AlertDialog(
      title: Text(dev.name.isEmpty ? dev.address : dev.name),
      content: Text(dev.address),
      actions: [
        TextButton(
          onPressed: () => Navigator.of(context).pop(false),
          child: Text(MaterialLocalizations.of(context).cancelButtonLabel),
        ),
        TextButton(
          onPressed: () => Navigator.of(context).pop(true),
          child: Text(
            l10n.btForget,
            style: TextStyle(color: PanelTheme.danger),
          ),
        ),
      ],
    ),
  );
  if (forget == true) await controller.unpairBt(dev.address);
}
