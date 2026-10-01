import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import 'routes.dart';

/// One host OS a flashing guide exists for.
class FlashDocSystem {
  const FlashDocSystem({required this.slug, required this.name});

  /// URL segment, e.g. linux.
  final String slug;
  final String Function(AppLocalizations l10n) name;
}

/// A headset with flashing docs. Each entry lists the host OSes the
/// guide is written for - today that is Linux only.
class FlashDocDevice {
  const FlashDocDevice({
    required this.slug,
    required this.name,
    required this.specs,
    required this.systems,
    required this.fullImageAssets,
    required this.cleanImageAssets,
  });

  /// URL segment, e.g. pico-neo-2.
  final String slug;
  final String Function(AppLocalizations l10n) name;

  /// Codename, SoC and panel in one short line, same as the device grid.
  final String Function(AppLocalizations l10n) specs;
  final List<FlashDocSystem> systems;

  /// Asset names the dist pipeline publishes this device's images under,
  /// preferred spelling first - older releases may still carry a
  /// pre-rename name further down the list.
  final List<String> fullImageAssets;
  final List<String> cleanImageAssets;

  /// In-site path of the device's overview page.
  String get path => Routes.flashdocsDevice(slug);

  /// In-site path of one host-OS guide under this device.
  String guidePath(String systemSlug) =>
      Routes.flashdocsGuide(slug, systemSlug);
}

/// Headsets covered by the flashing docs, in display order.
final flashDocDevices = <FlashDocDevice>[
  FlashDocDevice(
    slug: 'pico-neo-2',
    name: (l) => l.statDevice,
    specs: (l) => l.deviceNeo2Specs,
    systems: const [
      FlashDocSystem(slug: 'linux', name: _linuxName),
    ],
    fullImageAssets: const [
      'system-hibiscus-full.img.xz',
      'system-pn2-full.img.xz',
    ],
    cleanImageAssets: const [
      'system-hibiscus.img.xz',
      'system-pn2.img.xz',
    ],
  ),
];

String _linuxName(AppLocalizations l10n) => l10n.flashdocsOsLinux;

/// Look up a device by its URL slug - null when the slug is unknown.
FlashDocDevice? flashDocDevice(String? slug) {
  for (final d in flashDocDevices) {
    if (d.slug == slug) return d;
  }
  return null;
}

/// Look up a host-OS guide on [device] - null when the slug is unknown.
FlashDocSystem? flashDocSystem(FlashDocDevice device, String? slug) {
  for (final s in device.systems) {
    if (s.slug == slug) return s;
  }
  return null;
}
