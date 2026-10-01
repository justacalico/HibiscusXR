import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../store_controller.dart';
import 'theme.dart';

/// The repository address editor. Persisted and reloaded by the
/// controller - validation lives here since it is pure form logic.
Future<void> showRepoSheet(BuildContext context, StoreController controller) {
  return showDialog<void>(
    context: context,
    builder: (context) => _RepoDialog(controller: controller),
  );
}

/// http or https with a host - everything else is rejected up front.
String? repoUrlError(String raw, AppLocalizations l10n) {
  final uri = Uri.tryParse(raw.trim());
  if (uri == null ||
      !(uri.isScheme('http') || uri.isScheme('https')) ||
      uri.host.isEmpty) {
    return l10n.repoUrlInvalid;
  }
  return null;
}

class _RepoDialog extends StatefulWidget {
  const _RepoDialog({required this.controller});

  final StoreController controller;

  @override
  State<_RepoDialog> createState() => _RepoDialogState();
}

class _RepoDialogState extends State<_RepoDialog> {
  late final TextEditingController _url;
  String? _error;

  @override
  void initState() {
    super.initState();
    _url = TextEditingController(
      text: widget.controller.store.repoUrl.toString(),
    );
  }

  @override
  void dispose() {
    _url.dispose();
    super.dispose();
  }

  Future<void> _save() async {
    final l10n = AppLocalizations.of(context);
    final error = repoUrlError(_url.text, l10n);
    if (error != null) {
      setState(() => _error = error);
      return;
    }
    final navigator = Navigator.of(context);
    await widget.controller.setRepoUrl(_url.text.trim());
    navigator.pop();
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return AlertDialog(
      backgroundColor: StoreTheme.panel,
      title: Text(
        l10n.repoDialogTitle,
        style: const TextStyle(fontSize: 17, color: StoreTheme.textPrimary),
      ),
      content: SizedBox(
        width: 420,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              l10n.repoDialogBody,
              style: const TextStyle(
                fontSize: 13,
                color: StoreTheme.textSecondary,
              ),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _url,
              autofocus: true,
              style: const TextStyle(
                fontSize: 14,
                color: StoreTheme.textPrimary,
              ),
              onSubmitted: (_) => _save(),
              decoration: InputDecoration(
                labelText: l10n.repoUrlLabel,
                labelStyle: const TextStyle(
                  fontSize: 13,
                  color: StoreTheme.textSecondary,
                ),
                errorText: _error,
                isDense: true,
                filled: true,
                fillColor: StoreTheme.surface,
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: BorderSide.none,
                ),
              ),
            ),
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => setState(() {
            _url.text = StoreController.kDefaultRepo.toString();
            _error = null;
          }),
          child: Text(
            l10n.repoReset,
            style: const TextStyle(color: StoreTheme.textSecondary),
          ),
        ),
        TextButton(
          onPressed: () => Navigator.of(context).pop(),
          child: Text(
            l10n.cancel,
            style: const TextStyle(color: StoreTheme.textSecondary),
          ),
        ),
        FilledButton(
          onPressed: _save,
          style: FilledButton.styleFrom(
            foregroundColor: Colors.white,
            backgroundColor: StoreTheme.accent,
          ),
          child: Text(l10n.save),
        ),
      ],
    );
  }
}
