import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../l10n/app_localizations.dart';
import '../src/app_state.dart';
import '../src/theme.dart';

/// getprop table + rolling logcat tail.
class DebugPage extends StatefulWidget {
  const DebugPage({super.key, required this.state});

  final AppState state;

  @override
  State<DebugPage> createState() => _DebugPageState();
}

class _DebugPageState extends State<DebugPage> {
  final _filter = TextEditingController();
  String _query = '';
  String? _savedPath;

  @override
  void dispose() {
    _filter.dispose();
    super.dispose();
  }

  Future<void> _dump() async {
    final s = widget.state;
    final bytes = await s.dumpDebug();
    if (bytes == null || !mounted) return;
    final uri = await FilePicker.saveFile(
      dialogTitle: AppLocalizations.of(context).dumpSaveTitle,
      fileName: s.dumpSuggestedName,
      bytes: bytes,
      type: FileType.custom,
      allowedExtensions: ['zip'],
    );
    if (uri == null || !mounted) return;
    setState(() =>
        _savedPath = uri.scheme == 'file' ? uri.toFilePath() : '$uri');
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final s = widget.state;
    return ListenableBuilder(
      listenable: s,
      builder: (context, _) {
        final props = s.props ?? const <String, String>{};
        final rows = props.entries
            .where((e) =>
                _query.isEmpty ||
                e.key.contains(_query) ||
                e.value.contains(_query))
            .toList()
          ..sort((a, b) => a.key.compareTo(b.key));
        final logs = s.logBuf.toList();
        return Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Padding(
              padding: const EdgeInsets.fromLTRB(20, 16, 20, 4),
              child: Row(
                children: [
                  FilledButton.icon(
                    onPressed:
                        s.connected && !s.dumping ? _dump : null,
                    icon: const Icon(Icons.archive_outlined, size: 18),
                    label: Text(l10n.dumpButton),
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Text(
                      s.dumping
                          ? l10n.dumpRunning(s.dumpCount)
                          : s.dumpError != null
                              ? l10n.dumpFailed(s.dumpError!)
                              : _savedPath != null
                                  ? l10n.dumpSaved(_savedPath!)
                                  : l10n.dumpHint,
                      overflow: TextOverflow.ellipsis,
                      style:
                          Theme.of(context).textTheme.bodySmall?.copyWith(
                                color: s.dumpError != null
                                    ? CteTheme.accent
                                    : Colors.white38,
                              ),
                    ),
                  ),
                ],
              ),
            ),
            Expanded(
              child: Row(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            SizedBox(
              width: 380,
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Padding(
                    padding: const EdgeInsets.fromLTRB(20, 20, 20, 8),
                    child: Row(
                      children: [
                        Text(l10n.debugProps,
                            style: Theme.of(context).textTheme.titleSmall),
                        const SizedBox(width: 12),
                        Expanded(
                          child: TextField(
                            controller: _filter,
                            decoration: InputDecoration(
                              hintText: l10n.debugFilter,
                              isDense: true,
                              border: const OutlineInputBorder(),
                            ),
                            onChanged: (v) => setState(() => _query = v),
                          ),
                        ),
                      ],
                    ),
                  ),
                  Expanded(
                    child: ListView.builder(
                      padding: const EdgeInsets.symmetric(horizontal: 20),
                      itemCount: rows.length,
                      itemBuilder: (context, i) {
                        final e = rows[i];
                        return Padding(
                          padding: const EdgeInsets.symmetric(vertical: 1),
                          child: Text.rich(
                            TextSpan(
                              children: [
                                TextSpan(
                                    text: '${e.key} ',
                                    style:
                                        const TextStyle(color: Colors.white70)),
                                TextSpan(
                                    text: e.value,
                                    style:
                                        const TextStyle(color: Colors.white38)),
                              ],
                            ),
                            style: Theme.of(context)
                                .textTheme
                                .bodySmall
                                ?.copyWith(
                                    fontFamily: 'monospace', fontSize: 11),
                          ),
                        );
                      },
                    ),
                  ),
                ],
              ),
            ),
            const VerticalDivider(width: 1),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Padding(
                    padding: const EdgeInsets.fromLTRB(20, 20, 20, 8),
                    child: Text(l10n.debugLogcat,
                        style: Theme.of(context).textTheme.titleSmall),
                  ),
                  Expanded(
                    child: logs.isEmpty
                        ? Center(
                            child: Text(l10n.debugEmpty,
                                style: Theme.of(context)
                                    .textTheme
                                    .bodySmall
                                    ?.copyWith(color: Colors.white38)),
                          )
                        : ListView.builder(
                            padding:
                                const EdgeInsets.symmetric(horizontal: 20),
                            itemCount: logs.length,
                            itemBuilder: (context, i) => Text(
                              logs[i],
                              style: Theme.of(context)
                                  .textTheme
                                  .bodySmall
                                  ?.copyWith(
                                      fontFamily: 'monospace',
                                      fontSize: 11,
                                      color: Colors.white54),
                            ),
                          ),
                  ),
                ],
              ),
            ),
          ],
              ),
            ),
          ],
        );
      },
    );
  }
}
