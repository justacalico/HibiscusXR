import 'package:flutter/material.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../../flashdocs.dart';
import '../../../theme.dart';
import '../../builds_section.dart';
import '../../widgets.dart';
import 'flashdocs_layout.dart';
import 'guide_parts.dart';

/// The vmd guide: not a flashing flow, since vmd is the qemu target -
/// fetch the image, build the host tool, boot the VM.
class VmdGuidePage extends StatelessWidget {
  const VmdGuidePage({
    super.key,
    required this.device,
    required this.system,
  });

  final FlashDocDevice device;
  final FlashDocSystem system;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return FlashDocsLayout(
      activePath: device.guidePath(system.slug),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Eyebrow('${device.name(l10n)} · ${system.name(l10n)}'),
          const SizedBox(height: 10),
          Text(
            l10n.flashdocsVmdTitle,
            style: context.text.headlineMedium,
          ),
          const SizedBox(height: 12),
          Text(
            l10n.flashdocsVmdIntro,
            style: context.text.bodyMedium!.copyWith(fontSize: 17),
          ),
          const SizedBox(height: 40),
          _VmdRequirements(l10n: l10n),
          const SizedBox(height: 40),
          GuideStep(
            title: l10n.flashdocsVmdGetTitle,
            body: l10n.flashdocsVmdGetBody,
            l10n: l10n,
          ),
          const SizedBox(height: 24),
          BuildsSection(device: device),
          const SizedBox(height: 20),
          CommandBlock(lines: [l10n.flashdocsVmdGetCmd1]),
          const SizedBox(height: 40),
          GuideStep(
            title: l10n.flashdocsVmdRunTitle,
            body: l10n.flashdocsVmdRunBody,
            l10n: l10n,
          ),
          const SizedBox(height: 20),
          CommandBlock(lines: [
            l10n.flashdocsVmdRunCmd1,
            l10n.flashdocsVmdRunCmd2,
          ]),
          const SizedBox(height: 14),
          GuideNote(text: l10n.flashdocsVmdRunNote),
          const SizedBox(height: 40),
          GuideStep(
            title: l10n.flashdocsVmdAdbTitle,
            body: l10n.flashdocsVmdAdbBody,
            l10n: l10n,
          ),
          const SizedBox(height: 20),
          CommandBlock(lines: [
            l10n.flashdocsVmdAdbCmd1,
            l10n.flashdocsVmdAdbCmd2,
          ]),
          const SizedBox(height: 14),
          GuideNote(text: l10n.flashdocsVmdAdbNote),
        ],
      ),
    );
  }
}

class _VmdRequirements extends StatelessWidget {
  const _VmdRequirements({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final reqs = [
      l10n.flashdocsVmdReq1,
      l10n.flashdocsVmdReq2,
      l10n.flashdocsVmdReq3,
    ];
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(l10n.flashdocsNeo2ReqTitle, style: context.text.titleLarge),
        const SizedBox(height: 16),
        for (final req in reqs)
          Padding(
            padding: const EdgeInsets.only(bottom: 10),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Icon(Icons.check_circle_outline,
                    size: 18, color: AppColors.ok),
                const SizedBox(width: 12),
                Expanded(
                  child: Text(
                    req,
                    style: context.text.bodyMedium!
                        .copyWith(color: context.colors.onSurface),
                  ),
                ),
              ],
            ),
          ),
      ],
    );
  }
}
