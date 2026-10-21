import 'package:common/common.dart';
import 'package:flutter/material.dart';

import '../l10n/app_localizations.dart';
import '../src/app_state.dart';
import '../src/theme.dart';

class ConnectPage extends StatefulWidget {
  const ConnectPage({super.key, required this.state});

  final AppState state;

  @override
  State<ConnectPage> createState() => _ConnectPageState();
}

class _ConnectPageState extends State<ConnectPage> {
  final _host = TextEditingController();

  @override
  void initState() {
    super.initState();
    scanAfterFirstFrame(widget.state.scanAdb);
  }

  @override
  void dispose() {
    _host.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final s = widget.state;
    return ConnectScaffold(
      listenable: s,
      children: [
        _Header(l10n: l10n),
        const SizedBox(height: 24),
        if (s.lastError != null)
          ConnectErrorCard(
              message: s.lastError!, color: CteTheme.accent),
        AdbScanCard(
          title: l10n.connectUsbSection,
          titleStyle: Theme.of(context).textTheme.titleSmall,
          refreshTooltip: l10n.connectRefresh,
          noDevicesLabel: l10n.connectNoDevices,
          scanningLabel: l10n.connectScanning,
          devices: s.adbDevices,
          scanning: s.connState == ConnState.scanning,
          busy: s.connState == ConnState.scanning ||
              s.connState == ConnState.connecting,
          onRefresh: s.scanAdb,
          tileBuilder: (d) => AdbDeviceTile(
            device: d,
            showState: true,
            trailing: const Icon(Icons.chevron_right),
            onTap: d.isReady
                ? () => s.connectAdb(d.serial,
                    wireless: d.serial.contains(':'))
                : null,
          ),
        ),
        const SizedBox(height: 16),
        _WirelessCard(l10n: l10n, state: s, host: _host),
      ],
    );
  }
}

class _Header extends StatelessWidget {
  const _Header({required this.l10n});
  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        const Icon(Icons.developer_board, size: 40, color: CteTheme.accent),
        const SizedBox(height: 8),
        Text(l10n.appTitle,
            style: Theme.of(context)
                .textTheme
                .headlineMedium
                ?.copyWith(fontWeight: FontWeight.w700)),
        Text(l10n.appSubtitle,
            style: Theme.of(context)
                .textTheme
                .bodySmall
                ?.copyWith(color: Colors.white54)),
        const SizedBox(height: 8),
        Text(l10n.connectTitle,
            style: Theme.of(context).textTheme.titleMedium),
      ],
    );
  }
}

class _WirelessCard extends StatelessWidget {
  const _WirelessCard(
      {required this.l10n, required this.state, required this.host});
  final AppLocalizations l10n;
  final AppState state;
  final TextEditingController host;

  @override
  Widget build(BuildContext context) {
    final connecting = state.connState == ConnState.connecting;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(l10n.connectWirelessSection,
                style: Theme.of(context).textTheme.titleSmall),
            const SizedBox(height: 8),
            Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: host,
                    enabled: !connecting,
                    decoration: InputDecoration(
                      hintText: l10n.connectWirelessHint,
                      isDense: true,
                      border: const OutlineInputBorder(),
                    ),
                    keyboardType: TextInputType.number,
                    onSubmitted: (_) => state.connectWireless(host.text.trim()),
                  ),
                ),
                const SizedBox(width: 12),
                FilledButton(
                  onPressed: connecting
                      ? null
                      : () => state.connectWireless(host.text.trim()),
                  child: Text(connecting
                      ? l10n.connectConnecting
                      : l10n.connectWirelessButton),
                ),
              ],
            ),
            const SizedBox(height: 8),
            Text(l10n.connectWirelessNote,
                style: Theme.of(context)
                    .textTheme
                    .bodySmall
                    ?.copyWith(color: Colors.white54)),
          ],
        ),
      ),
    );
  }
}
