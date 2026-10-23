import 'package:flutter/material.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../../routes.dart';
import '../../../theme.dart';
import '../../widgets.dart';
import 'flashdocs_layout.dart';
import 'guide_parts.dart';

/// The home-environment guide: what the dash floats over, how to pick,
/// install, fix and remove custom packs. A literal doc page under the
/// docs section, next to the per-device flashing guides.
class FlashDocsEnvPage extends StatelessWidget {
  const FlashDocsEnvPage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return FlashDocsLayout(
      activePath: Routes.flashdocsEnv,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Eyebrow('vrhome'),
          const SizedBox(height: 10),
          Text(
            l10n.flashdocsEnvTitle,
            style: context.text.headlineMedium,
          ),
          const SizedBox(height: 12),
          Text(
            l10n.flashdocsEnvIntro,
            style: context.text.bodyMedium!.copyWith(fontSize: 17),
          ),
          const SizedBox(height: 40),
          GuideStep(
            title: l10n.flashdocsEnvPickTitle,
            body: l10n.flashdocsEnvPickBody,
            l10n: l10n,
          ),
          const SizedBox(height: 40),
          GuideStep(
            title: l10n.flashdocsEnvInstallTitle,
            body: l10n.flashdocsEnvInstallBody,
            l10n: l10n,
          ),
          const SizedBox(height: 20),
          CommandBlock(lines: [l10n.flashdocsEnvInstallCmd1]),
          const SizedBox(height: 14),
          GuideNote(text: l10n.flashdocsEnvInstallNote),
          const SizedBox(height: 40),
          GuideStep(
            title: l10n.flashdocsEnvFormatTitle,
            body: l10n.flashdocsEnvFormatBody,
            l10n: l10n,
          ),
          const SizedBox(height: 40),
          GuideStep(
            title: l10n.flashdocsEnvFixTitle,
            body: l10n.flashdocsEnvFixBody,
            l10n: l10n,
          ),
          const SizedBox(height: 40),
          GuideStep(
            title: l10n.flashdocsEnvRemoveTitle,
            body: l10n.flashdocsEnvRemoveBody,
            l10n: l10n,
          ),
          const SizedBox(height: 40),
          GuideStep(
            title: l10n.flashdocsEnvDebugTitle,
            body: l10n.flashdocsEnvDebugBody,
            l10n: l10n,
          ),
          const SizedBox(height: 20),
          CommandBlock(lines: [
            l10n.flashdocsEnvDebugCmd1,
            l10n.flashdocsEnvDebugCmd2,
          ]),
          const SizedBox(height: 14),
          GuideNote(text: l10n.flashdocsEnvDebugNote),
        ],
      ),
    );
  }
}
