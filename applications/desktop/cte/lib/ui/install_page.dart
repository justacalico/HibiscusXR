import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../l10n/app_localizations.dart';
import '../src/app_state.dart';
import '../src/theme.dart';

/// Push an APK to the headset over whichever link is up.
class InstallPage extends StatefulWidget {
  const InstallPage({super.key, required this.state});

  final AppState state;

  @override
  State<InstallPage> createState() => _InstallPageState();
}

class _InstallPageState extends State<InstallPage> {
  String? _apkPath;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final s = widget.state;
    return ListenableBuilder(
      listenable: s,
      builder: (context, _) => ListView(
        padding: const EdgeInsets.all(24),
        children: [
          Text(l10n.installTitle,
              style: Theme.of(context).textTheme.titleSmall),
          const SizedBox(height: 12),
          Row(
            children: [
              OutlinedButton.icon(
                onPressed: s.installing
                    ? null
                    : () async {
                        final f = await FilePicker.pickFile(
                            type: FileType.custom,
                            allowedExtensions: ['apk']);
                        final p = f?.path;
                        if (p != null) setState(() => _apkPath = p);
                      },
                icon: const Icon(Icons.folder_open, size: 18),
                label: Text(l10n.installPick),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: Text(
                  _apkPath ?? l10n.installDrop,
                  overflow: TextOverflow.ellipsis,
                  style: Theme.of(context)
                      .textTheme
                      .bodySmall
                      ?.copyWith(color: Colors.white54),
                ),
              ),
              const SizedBox(width: 12),
              FilledButton(
                onPressed:
                    s.installing || _apkPath == null ? null : () => s.install(_apkPath!),
                child:
                    Text(s.installing ? l10n.installRunning : l10n.installRun),
              ),
            ],
          ),
          const SizedBox(height: 16),
          if (s.installLog.isNotEmpty)
            Card(
              child: Padding(
                padding: const EdgeInsets.all(12),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    for (final line in s.installLog)
                      Text(
                        line,
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                              fontFamily: 'monospace',
                              color: line.startsWith('error')
                                  ? CteTheme.accent
                                  : Colors.white70,
                            ),
                      ),
                  ],
                ),
              ),
            ),
        ],
      ),
    );
  }
}
