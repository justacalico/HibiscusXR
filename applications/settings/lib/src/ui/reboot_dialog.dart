import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import 'theme.dart';

/// Ask before a reboot-gated setting change. True means the user hit
/// Reboot; cancel and dismiss both come back false, so the caller can
/// treat anything but an explicit confirm as "leave it alone".
Future<bool> showRebootConfirm(BuildContext context) async =>
    await showDialog<bool>(
      context: context,
      builder: (context) => const RebootConfirmDialog(),
    ) ??
    false;

/// Shared "this change needs a reboot" prompt. Strings come from l10n
/// like every other user-facing label; the caller decides what the
/// confirm actually applies.
class RebootConfirmDialog extends StatelessWidget {
  const RebootConfirmDialog({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return AlertDialog(
      backgroundColor: PanelTheme.surface,
      title: Text(l10n.rebootRequiredTitle),
      content: Text(l10n.rebootRequiredBody),
      actions: [
        TextButton(
          onPressed: () => Navigator.of(context).pop(false),
          child: Text(MaterialLocalizations.of(context).cancelButtonLabel),
        ),
        FilledButton(
          onPressed: () => Navigator.of(context).pop(true),
          child: Text(l10n.rebootConfirm),
        ),
      ],
    );
  }
}
