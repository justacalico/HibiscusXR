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
    WidgetsBinding.instance
        .addPostFrameCallback((_) => widget.state.scanAdb());
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
    return Scaffold(
      body: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 560),
          child: ListenableBuilder(
            listenable: s,
            builder: (context, _) => ListView(
              shrinkWrap: true,
              padding: const EdgeInsets.all(32),
              children: [
                _Header(l10n: l10n),
                const SizedBox(height: 24),
                if (s.lastError != null)
                  Card(
                    child: Padding(
                      padding: const EdgeInsets.all(12),
                      child: Text(s.lastError!,
                          style: TextStyle(color: CteTheme.accent)),
                    ),
                  ),
                _UsbCard(l10n: l10n, state: s),
                const SizedBox(height: 16),
                _WirelessCard(l10n: l10n, state: s, host: _host),
              ],
            ),
          ),
        ),
      ),
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

class _UsbCard extends StatelessWidget {
  const _UsbCard({required this.l10n, required this.state});
  final AppLocalizations l10n;
  final AppState state;

  @override
  Widget build(BuildContext context) {
    final busy = state.connState == ConnState.scanning ||
        state.connState == ConnState.connecting;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Expanded(
                  child: Text(l10n.connectUsbSection,
                      style: Theme.of(context).textTheme.titleSmall),
                ),
                IconButton(
                  icon: const Icon(Icons.refresh, size: 18),
                  tooltip: l10n.connectRefresh,
                  onPressed: busy ? null : state.scanAdb,
                ),
              ],
            ),
            if (state.connState == ConnState.scanning)
              Text(l10n.connectScanning)
            else if (state.adbDevices.isEmpty)
              Text(l10n.connectNoDevices,
                  style: Theme.of(context).textTheme.bodySmall)
            else
              for (final d in state.adbDevices)
                ListTile(
                  dense: true,
                  contentPadding: EdgeInsets.zero,
                  leading: const Icon(Icons.usb, size: 18),
                  title: Text(d.model.isNotEmpty ? d.model : d.serial),
                  subtitle: Text('${d.serial} - ${d.state}'),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: d.isReady
                      ? () => state.connectAdb(d.serial,
                          wireless: d.serial.contains(':'))
                      : null,
                ),
          ],
        ),
      ),
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
