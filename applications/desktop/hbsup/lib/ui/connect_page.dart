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
  @override
  void initState() {
    super.initState();
    scanAfterFirstFrame(widget.state.scanAdb);
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final s = widget.state;
    return ConnectScaffold(
      listenable: s,
      children: [
        Column(
          children: [
            ClipRRect(
              borderRadius: BorderRadius.circular(18),
              child: Image.asset('assets/icon.png',
                  width: 84, height: 84),
            ),
            const SizedBox(height: 16),
            Text(l10n.appTitle,
                style: Theme.of(context).textTheme.headlineMedium),
            Text(l10n.appSubtitle,
                style: Theme.of(context).textTheme.bodyMedium),
          ],
        ),
        const SizedBox(height: 8),
        Text(
          l10n.connectHint,
          textAlign: TextAlign.center,
          style: Theme.of(context).textTheme.bodySmall,
        ),
        const SizedBox(height: 24),
        if (s.lastError != null)
          ConnectErrorCard(
              message: s.lastError!, color: HbsupTheme.bad),
        AdbScanCard(
          title: l10n.connectTitle,
          titleStyle: Theme.of(context).textTheme.titleMedium,
          refreshTooltip: l10n.connectRefresh,
          noDevicesLabel: l10n.connectNoDevices,
          devices: s.devices,
          scanning: s.scanState == ScanState.scanning,
          onRefresh: s.scanAdb,
          padding: const EdgeInsets.all(20),
          titleGap: 12,
          emptyPadding: const EdgeInsets.symmetric(vertical: 12),
          tileBuilder: (d) => AdbDeviceTile(
            device: d,
            unknownLabel: l10n.connectUnknown,
            iconSize: 20,
            trailing: TextButton(
              onPressed:
                  d.isReady ? () => s.connect(d.serial) : null,
              child: Text(l10n.connectButton),
            ),
          ),
        ),
      ],
    );
  }
}
