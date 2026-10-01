import 'package:flutter/material.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../../flashdocs.dart';
import '../../../routes.dart';
import '../../../theme.dart';
import '../../widgets.dart';
import 'flashdocs_layout.dart';

/// Flashing docs landing: asks which headset the user has, then routes
/// to that device's page.
class FlashDocsHomePage extends StatelessWidget {
  const FlashDocsHomePage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return FlashDocsLayout(
      activePath: Routes.flashdocs,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Eyebrow(l10n.flashdocsTitle),
          const SizedBox(height: 10),
          Text(l10n.flashdocsPickDeviceTitle,
              style: context.text.headlineMedium),
          const SizedBox(height: 12),
          Text(
            l10n.flashdocsPickDeviceBody,
            style: context.text.bodyMedium!.copyWith(fontSize: 17),
          ),
          const SizedBox(height: 32),
          for (final d in flashDocDevices)
            Padding(
              padding: const EdgeInsets.only(bottom: 16),
              child: FlashDocsPickCard(
                title: d.name(l10n),
                subtitle: d.specs(l10n),
                path: d.path,
              ),
            ),
        ],
      ),
    );
  }
}
