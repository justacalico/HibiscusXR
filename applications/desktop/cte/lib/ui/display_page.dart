import 'package:flutter/material.dart';

import '../l10n/app_localizations.dart';
import '../src/app_state.dart';

/// Live screen mirror - screencap frames piped back to back.
class DisplayPage extends StatelessWidget {
  const DisplayPage({super.key, required this.state});

  final AppState state;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return ListenableBuilder(
      listenable: state,
      builder: (context, _) => Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Padding(
            padding: const EdgeInsets.fromLTRB(24, 16, 24, 8),
            child: Row(
              children: [
                Text(l10n.displayMirror,
                    style: Theme.of(context).textTheme.titleSmall),
                const SizedBox(width: 16),
                TextButton.icon(
                  onPressed: () => state.setMirror(!state.mirrorOn),
                  icon: Icon(state.mirrorOn ? Icons.stop : Icons.play_arrow,
                      size: 18),
                  label: Text(
                      state.mirrorOn ? l10n.displayStop : l10n.displayStart),
                ),
                const Spacer(),
                if (state.framesSeen > 0)
                  Text(
                      l10n.displayStats('${state.framesSeen}',
                          state.frameRate.toStringAsFixed(1)),
                      style: Theme.of(context)
                          .textTheme
                          .bodySmall
                          ?.copyWith(color: Colors.white54)),
              ],
            ),
          ),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 24),
            child: Text(l10n.displayHint,
                style: Theme.of(context)
                    .textTheme
                    .bodySmall
                    ?.copyWith(color: Colors.white38)),
          ),
          const SizedBox(height: 16),
          Expanded(
            child: Center(
              child: state.frame != null
                  ? InteractiveViewer(
                      child: Image.memory(
                        state.frame!,
                        gaplessPlayback: true,
                        fit: BoxFit.contain,
                      ),
                    )
                  : Text(l10n.displayNoFrame,
                      style: Theme.of(context)
                          .textTheme
                          .bodyMedium
                          ?.copyWith(color: Colors.white38)),
            ),
          ),
        ],
      ),
    );
  }
}
