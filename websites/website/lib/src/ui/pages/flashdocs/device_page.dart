import 'package:flutter/material.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../../flashdocs.dart';
import '../../../theme.dart';
import '../../widgets.dart';
import 'flashdocs_layout.dart';

/// One device under the flashing docs: asks which host OS the user is
/// on, then routes to that guide.
class FlashDocsDevicePage extends StatelessWidget {
  const FlashDocsDevicePage({super.key, required this.device});

  final FlashDocDevice device;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return FlashDocsLayout(
      activePath: device.path,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Eyebrow(device.name(l10n)),
          const SizedBox(height: 10),
          Text(l10n.flashdocsPickOsTitle,
              style: context.text.headlineMedium),
          const SizedBox(height: 12),
          Text(
            l10n.flashdocsPickOsBody,
            style: context.text.bodyMedium!.copyWith(fontSize: 17),
          ),
          const SizedBox(height: 32),
          for (final s in device.systems)
            Padding(
              padding: const EdgeInsets.only(bottom: 16),
              child: FlashDocsPickCard(
                title: s.name(l10n),
                subtitle: s.body?.call(l10n) ?? l10n.flashdocsOsCardBody,
                path: device.guidePath(s.slug),
              ),
            ),
        ],
      ),
    );
  }
}
