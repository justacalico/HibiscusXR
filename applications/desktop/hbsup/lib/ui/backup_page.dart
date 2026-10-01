import 'package:flutter/material.dart';

import '../l10n/app_localizations.dart';
import '../src/app_state.dart';
import '../src/engine.dart';
import '../src/plan.dart';
import '../src/theme.dart';

class BackupPage extends StatelessWidget {
  const BackupPage({super.key, required this.state});

  final AppState state;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final s = state;
    return Scaffold(
      body: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 640),
          child: ListenableBuilder(
            listenable: s,
            builder: (context, _) => ListView(
              padding: const EdgeInsets.all(32),
              children: [
                Row(
                  children: [
                    Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(l10n.appTitle,
                            style: Theme.of(context)
                                .textTheme
                                .headlineSmall),
                        Text(s.activeSerial ?? '',
                            style:
                                Theme.of(context).textTheme.bodySmall),
                      ],
                    ),
                    const Spacer(),
                    TextButton.icon(
                      onPressed: s.running ? null : s.disconnect,
                      icon: const Icon(Icons.link_off, size: 16),
                      label: Text(l10n.disconnect),
                    ),
                  ],
                ),
                const SizedBox(height: 16),
                _DestCard(l10n: l10n, state: s, onPick: s.pickDestination),
                const SizedBox(height: 16),
                _PartsCard(l10n: l10n, state: s),
                const SizedBox(height: 16),
                _RunCard(l10n: l10n, state: s),
                if (s.logBuf.isNotEmpty) ...[
                  const SizedBox(height: 16),
                  _LogCard(l10n: l10n, state: s),
                ],
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _DestCard extends StatelessWidget {
  const _DestCard(
      {required this.l10n, required this.state, required this.onPick});

  final AppLocalizations l10n;
  final AppState state;
  final VoidCallback onPick;

  @override
  Widget build(BuildContext context) {
    final space = state.space;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(l10n.backupDestTitle,
                style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),
            Row(
              children: [
                Expanded(
                  child: Text(
                    state.destDir ?? l10n.backupDestNone,
                    style: Theme.of(context).textTheme.bodySmall,
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
                const SizedBox(width: 12),
                FilledButton.tonal(
                  onPressed: state.running ? null : onPick,
                  child: Text(l10n.backupDestPick),
                ),
              ],
            ),
            if (state.destDir != null) ...[
              const SizedBox(height: 8),
              Text(
                state.destFreeBytes == null
                    ? l10n.backupFreeUnknown
                    : l10n.backupFreeSpace(
                        formatBytes(state.destFreeBytes!)),
                style: Theme.of(context).textTheme.bodySmall,
              ),
            ],
            if (space != null) ...[
              const SizedBox(height: 10),
              Row(
                children: [
                  Icon(
                    space.fits
                        ? Icons.check_circle_outline
                        : Icons.error_outline,
                    size: 16,
                    color: space.fits ? HbsupTheme.accent : HbsupTheme.bad,
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: Text(
                      space.fits
                          ? l10n.backupSpaceOk
                          : l10n.backupSpaceShort(
                              formatBytes(space.needed),
                              formatBytes(space.free ?? 0),
                            ),
                      style: Theme.of(context).textTheme.bodySmall,
                    ),
                  ),
                ],
              ),
            ],
          ],
        ),
      ),
    );
  }
}

class _PartsCard extends StatelessWidget {
  const _PartsCard({required this.l10n, required this.state});

  final AppLocalizations l10n;
  final AppState state;

  @override
  Widget build(BuildContext context) {
    final parts = state.partitions;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(l10n.backupPartsTitle,
                style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),
            if (state.loadingPartitions)
              Padding(
                padding: const EdgeInsets.symmetric(vertical: 12),
                child: Row(
                  children: [
                    const SizedBox(
                        width: 16,
                        height: 16,
                        child:
                            CircularProgressIndicator(strokeWidth: 2)),
                    const SizedBox(width: 12),
                    Text(l10n.backupPartsLoading,
                        style: Theme.of(context).textTheme.bodySmall),
                  ],
                ),
              )
            else if (parts == null || parts.isEmpty)
              Padding(
                padding: const EdgeInsets.symmetric(vertical: 12),
                child: Text(l10n.backupPartsEmpty,
                    style: Theme.of(context).textTheme.bodySmall),
              )
            else ...[
              for (var i = 0; i < parts.length; i++)
                _PartRow(
                  name: parts[i].name,
                  size: parts[i].sizeBytes,
                  item:
                      i < state.items.length ? state.items[i] : null,
                  current: i == state.currentItem,
                ),
              const SizedBox(height: 8),
              Text(
                l10n.backupTotalSize(formatBytes(
                    parts.fold(0, (s, p) => s + p.sizeBytes))),
                style: Theme.of(context).textTheme.bodySmall,
              ),
            ],
          ],
        ),
      ),
    );
  }
}

class _PartRow extends StatelessWidget {
  const _PartRow({
    required this.name,
    required this.size,
    required this.item,
    required this.current,
  });

  final String name;
  final int size;
  final BackupItem? item;
  final bool current;

  @override
  Widget build(BuildContext context) {
    final phase = item?.phase;
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 6),
      child: Row(
        children: [
          SizedBox(
            width: 18,
            child: switch (phase) {
              ItemPhase.done => const Icon(Icons.check_circle,
                  size: 15, color: HbsupTheme.accent),
              ItemPhase.failed => const Icon(Icons.error,
                  size: 15, color: HbsupTheme.bad),
              ItemPhase.running => SizedBox(
                  width: 13,
                  height: 13,
                  child: CircularProgressIndicator(
                      strokeWidth: 2, value: item?.fraction)),
              _ => Icon(Icons.circle_outlined,
                  size: 13,
                  color: Theme.of(context).colorScheme.outline),
            },
          ),
          const SizedBox(width: 10),
          Expanded(child: Text(name)),
          Text(
            phase == ItemPhase.running
                ? formatBytes(item!.written)
                : formatBytes(size),
            style: Theme.of(context).textTheme.bodySmall,
          ),
          const SizedBox(width: 10),
          SizedBox(
            width: 90,
            child: LinearProgressIndicator(
              value: item?.fraction ?? 0,
              minHeight: 3,
            ),
          ),
        ],
      ),
    );
  }
}

class _RunCard extends StatelessWidget {
  const _RunCard({required this.l10n, required this.state});

  final AppLocalizations l10n;
  final AppState state;

  @override
  Widget build(BuildContext context) {
    final s = state;
    final canStart = !s.running &&
        s.plan != null &&
        (s.space?.fits ?? true) &&
        s.backupState != BackupState.running;
    final status = switch (s.backupState) {
      BackupState.done => l10n.backupDone,
      BackupState.failed => l10n.backupFailed(
          s.items
              .where((i) => i.error != null)
              .map((i) => i.error!)
              .firstOrNull ??
              ''),
      BackupState.cancelled => l10n.backupCancelled,
      _ => null,
    };
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            if (s.running)
              Text(
                l10n.backupProgress(
                  s.items
                      .where((i) => i.phase == ItemPhase.done)
                      .length,
                  s.items.length,
                ),
                style: Theme.of(context).textTheme.bodySmall,
              ),
            if (status != null)
              Padding(
                padding: const EdgeInsets.only(bottom: 12),
                child: Text(
                  status,
                  style: Theme.of(context).textTheme.bodyMedium!.copyWith(
                        color: s.backupState == BackupState.done
                            ? HbsupTheme.accent
                            : HbsupTheme.bad,
                      ),
                ),
              ),
            Row(
              children: [
                FilledButton.icon(
                  onPressed: canStart ? s.startBackup : null,
                  icon: const Icon(Icons.save_alt, size: 18),
                  label: Text(s.running
                      ? l10n.backupRunning
                      : l10n.backupStart),
                ),
                const SizedBox(width: 12),
                if (s.running)
                  TextButton(
                    onPressed: s.cancel,
                    child: Text(l10n.backupCancel),
                  ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _LogCard extends StatelessWidget {
  const _LogCard({required this.l10n, required this.state});

  final AppLocalizations l10n;
  final AppState state;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(l10n.backupLogTitle,
                style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 8),
            for (final line in state.logBuf)
              Text(line,
                  style: Theme.of(context)
                      .textTheme
                      .bodySmall!
                      .copyWith(fontFamily: 'monospace', fontSize: 11)),
          ],
        ),
      ),
    );
  }
}
