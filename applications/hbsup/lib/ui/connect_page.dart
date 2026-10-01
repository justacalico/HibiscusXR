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
    WidgetsBinding.instance
        .addPostFrameCallback((_) => widget.state.scanAdb());
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
                  Card(
                    child: Padding(
                      padding: const EdgeInsets.all(12),
                      child: Text(s.lastError!,
                          style: const TextStyle(color: HbsupTheme.bad)),
                    ),
                  ),
                Card(
                  child: Padding(
                    padding: const EdgeInsets.all(20),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            Text(l10n.connectTitle,
                                style: Theme.of(context)
                                    .textTheme
                                    .titleMedium),
                            const Spacer(),
                            if (s.scanState == ScanState.scanning)
                              const SizedBox(
                                width: 16,
                                height: 16,
                                child: CircularProgressIndicator(
                                    strokeWidth: 2),
                              )
                            else
                              IconButton(
                                tooltip: l10n.connectRefresh,
                                icon: const Icon(Icons.refresh, size: 18),
                                onPressed: s.scanAdb,
                              ),
                          ],
                        ),
                        const SizedBox(height: 12),
                        if (s.devices.isEmpty)
                          Padding(
                            padding:
                                const EdgeInsets.symmetric(vertical: 12),
                            child: Text(l10n.connectNoDevices,
                                style: Theme.of(context)
                                    .textTheme
                                    .bodySmall),
                          )
                        else
                          for (final d in s.devices)
                            ListTile(
                              dense: true,
                              contentPadding: EdgeInsets.zero,
                              leading: const Icon(Icons.usb, size: 20),
                              title: Text(d.model.isEmpty
                                  ? l10n.connectUnknown
                                  : d.model),
                              subtitle: Text(d.serial),
                              trailing: TextButton(
                                onPressed: d.isReady
                                    ? () => s.connect(d.serial)
                                    : null,
                                child: Text(l10n.connectButton),
                              ),
                            ),
                      ],
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
