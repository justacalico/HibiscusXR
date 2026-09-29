import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../labels.dart';
import '../models.dart';
import '../settings_controller.dart';
import 'theme.dart';

/// Full-width card listing the networks the last scan found. The
/// header doubles as the rescan control; each row opens a connect or
/// manage dialog instead of punting to the system wifi page.
class WifiCard extends StatelessWidget {
  const WifiCard({
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
    final scanning = store.wifiScanning;
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
                    child: scanning
                        ? CircularProgressIndicator(
                            strokeWidth: 2.4,
                            color: PanelTheme.accent,
                          )
                        : Icon(
                            Icons.wifi,
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
                          scanning ? l10n.wifiScanning : l10n.wifiScanIdle,
                          style: TextStyle(
                            fontSize: 12,
                            color: scanning
                                ? PanelTheme.accent
                                : PanelTheme.textSecondary,
                          ),
                        ),
                      ],
                    ),
                  ),
                  TextButton(
                    onPressed: enabled && !scanning
                        ? () => controller.scanWifi()
                        : null,
                    style: TextButton.styleFrom(
                      foregroundColor: PanelTheme.accent,
                    ),
                    child: Text(l10n.wifiRescan),
                  ),
                ],
              ),
              const SizedBox(height: 14),
              if (store.wifiNetworks.isEmpty)
                Text(
                  l10n.wifiEmpty,
                  style: TextStyle(
                    fontSize: 13,
                    color: PanelTheme.textSecondary,
                  ),
                )
              else
                for (final net in store.wifiNetworks)
                  _NetRow(
                    net: net,
                    enabled: enabled,
                    onTap: () => _joinDialog(context, controller, net),
                  ),
            ],
          ),
        ),
      ),
    );
  }
}

class _NetRow extends StatelessWidget {
  const _NetRow({
    required this.net,
    required this.enabled,
    required this.onTap,
  });

  final WifiNetwork net;
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
              net.level >= 2 ? Icons.wifi : Icons.wifi_outlined,
              size: 20,
              color: net.connected
                  ? PanelTheme.accent
                  : PanelTheme.textSecondary,
            ),
            const SizedBox(width: 14),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    net.ssid,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      fontSize: 15,
                      color: PanelTheme.textPrimary,
                    ),
                  ),
                  Text(
                    wifiNetworkLabel(l10n, net),
                    style: TextStyle(
                      fontSize: 12,
                      color: net.connected
                          ? PanelTheme.accent
                          : PanelTheme.textSecondary,
                    ),
                  ),
                ],
              ),
            ),
            if (net.security != WifiSecurity.open)
              Icon(
                Icons.lock_outline,
                size: 16,
                color: PanelTheme.textSecondary,
              ),
            const SizedBox(width: 8),
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

/// Tap on a network: connected and saved rows offer disconnect-free
/// management, unknown secured rows ask for the key.
Future<void> _joinDialog(
  BuildContext context,
  SettingsController controller,
  WifiNetwork net,
) async {
  final l10n = AppLocalizations.of(context);
  final password = TextEditingController();
  final join = await showDialog<WifiJoin>(
    context: context,
    builder: (context) {
      final actions = <Widget>[
        if (net.saved)
          TextButton(
            onPressed: () async {
              Navigator.of(context).pop();
              await controller.forgetWifi(net.savedId);
            },
            child: Text(
              l10n.wifiForget,
              style: TextStyle(color: PanelTheme.danger),
            ),
          ),
        TextButton(
          onPressed: () => Navigator.of(context).pop(),
          child: Text(MaterialLocalizations.of(context).cancelButtonLabel),
        ),
        if (!net.connected)
          TextButton(
            onPressed: () => Navigator.of(context).pop(
              WifiJoin(
                ssid: net.ssid,
                security: net.security,
                password: password.text,
              ),
            ),
            child: Text(l10n.wifiConnect),
          ),
      ];
      return AlertDialog(
        title: Text(l10n.wifiJoinTitle(net.ssid)),
        content: net.connected
            ? Text(wifiNetworkLabel(l10n, net))
            : net.security == WifiSecurity.open || net.saved
            ? null
            : TextField(
                controller: password,
                obscureText: true,
                autofocus: true,
                decoration: InputDecoration(labelText: l10n.wifiPassword),
              ),
        actions: actions,
      );
    },
  );
  password.dispose();
  if (join != null) await controller.connectWifi(join);
}
