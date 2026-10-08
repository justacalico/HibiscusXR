import 'package:hibiscusxr_website/l10n/app_localizations.dart';

/// Where a headset stands, matching the README compatibility table.
/// virtual is the qemu target - not hardware, its own thing.
enum DeviceStatus { supported, planned, virtual }

/// One target the OS runs on today or is planned to run on - headsets
/// plus the vmd virtual device.
/// Name, specs and the blurb all resolve through l10n like every
/// other string on the site.
class DeviceEntry {
  const DeviceEntry({
    required this.status,
    required this.name,
    required this.specs,
    required this.describe,
  });

  final DeviceStatus status;

  /// Marketing name, e.g. Pico Neo 2.
  final String Function(AppLocalizations l10n) name;

  /// Codename, SoC and panel in one short line.
  final String Function(AppLocalizations l10n) specs;

  /// Localised one-liner shown on the card.
  final String Function(AppLocalizations l10n) describe;

  /// Label for the status chip on the card.
  String statusLabel(AppLocalizations l10n) => switch (status) {
        DeviceStatus.supported => l10n.deviceStateSupported,
        DeviceStatus.planned => l10n.deviceStatePlanned,
        DeviceStatus.virtual => l10n.deviceStateVirtual,
      };
}

/// Targets from the README's compatibility table, in display order.
final devices = <DeviceEntry>[
  DeviceEntry(
    status: DeviceStatus.supported,
    name: (l) => l.deviceNeo2Name,
    specs: (l) => l.deviceNeo2Specs,
    describe: (l) => l.deviceNeo2Body,
  ),
  DeviceEntry(
    status: DeviceStatus.planned,
    name: (l) => l.deviceQuest1Name,
    specs: (l) => l.deviceQuest1Specs,
    describe: (l) => l.devicePlannedBody,
  ),
  DeviceEntry(
    status: DeviceStatus.planned,
    name: (l) => l.deviceNeo3Name,
    specs: (l) => l.deviceNeo3Specs,
    describe: (l) => l.devicePlannedBody,
  ),
  DeviceEntry(
    status: DeviceStatus.virtual,
    name: (l) => l.deviceVmdName,
    specs: (l) => l.deviceVmdSpecs,
    describe: (l) => l.deviceVmdBody,
  ),
];
