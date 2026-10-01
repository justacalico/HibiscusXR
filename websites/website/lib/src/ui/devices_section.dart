import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../devices.dart';
import '../routes.dart';
import '../theme.dart';
import 'widgets.dart';

/// The README's compatibility table as a card grid - one card per
/// headset with its port status. Sits on the home page.
class DevicesSection extends StatelessWidget {
  const DevicesSection({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Band(
      child: Column(
        children: [
          Reveal(child: Eyebrow(l10n.homeDevicesEyebrow, center: true)),
          const SizedBox(height: 12),
          Reveal(
            child: Text(
              l10n.homeDevicesTitle,
              textAlign: TextAlign.center,
              style: context.text.displayMedium!.copyWith(
                fontSize: Layout.isMobile(context) ? 36 : 56,
              ),
            ),
          ),
          const SizedBox(height: 16),
          Reveal(
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: Layout.text),
              child: Text(
                l10n.homeDevicesBody,
                textAlign: TextAlign.center,
                style: context.text.bodyLarge!
                    .copyWith(color: context.colors.secondary),
              ),
            ),
          ),
          const SizedBox(height: 48),
          LayoutBuilder(
            builder: (context, constraints) {
              final columns =
                  constraints.maxWidth >= 760 ? devices.length : 1;
              final width =
                  (constraints.maxWidth - (columns - 1) * 24) / columns;
              return Wrap(
                spacing: 24,
                runSpacing: 24,
                children: [
                  for (var i = 0; i < devices.length; i++)
                    Reveal(
                      delay: Duration(milliseconds: i * 100),
                      child: SizedBox(
                        width: width,
                        child: _DeviceCard(device: devices[i]),
                      ),
                    ),
                ],
              );
            },
          ),
        ],
      ),
    );
  }
}

class _DeviceCard extends StatelessWidget {
  const _DeviceCard({required this.device});

  final DeviceEntry device;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final tint = switch (device.status) {
      DeviceStatus.supported => AppColors.ok,
      DeviceStatus.planned => context.colors.secondary,
    };
    return Container(
      padding: const EdgeInsets.all(28),
      decoration: BoxDecoration(
        color: context.colors.surfaceContainerHighest,
        borderRadius: BorderRadius.circular(24),
        border: Border.all(color: context.colors.outline, width: 1),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            device.name(l10n),
            style: context.text.titleMedium!.copyWith(fontSize: 21),
          ),
          const SizedBox(height: 12),
          _StatusChip(label: device.statusLabel(l10n), tint: tint),
          const SizedBox(height: 14),
          Text(
            device.specs(l10n),
            style: context.text.labelSmall!.copyWith(fontSize: 13),
          ),
          const SizedBox(height: 12),
          Text(device.describe(l10n), style: context.text.bodyMedium),
          if (device.status == DeviceStatus.supported) ...[
            const SizedBox(height: 16),
            ChevronLink(
              label: l10n.homeStatusCta,
              large: false,
              onPressed: () => context.go(Routes.status),
            ),
          ],
        ],
      ),
    );
  }
}

/// Small pill with a dot and the status label, tinted by status.
class _StatusChip extends StatelessWidget {
  const _StatusChip({required this.label, required this.tint});

  final String label;
  final Color tint;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(980),
        border: Border.all(color: tint.withValues(alpha: 0.4), width: 1),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 6,
            height: 6,
            decoration: BoxDecoration(color: tint, shape: BoxShape.circle),
          ),
          const SizedBox(width: 7),
          Text(
            label,
            style: context.text.labelSmall!.copyWith(
              color: tint,
              fontSize: 13,
              fontWeight: FontWeight.w600,
            ),
          ),
        ],
      ),
    );
  }
}
