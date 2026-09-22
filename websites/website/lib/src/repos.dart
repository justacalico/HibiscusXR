import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import 'links.dart';

/// Which section of the repository map an entry belongs to.
enum RepoGroup { software, source, dumps }

/// One directory in the HibiscusXR monorepo. The name is the path inside
/// the repo, the description resolves through l10n like every other string.
class RepoEntry {
  const RepoEntry(this.name, this.group, this.describe);

  /// Path inside the monorepo, e.g. `applications/vrhome`.
  final String name;

  final RepoGroup group;

  /// Localised one-liner shown on the card.
  final String Function(AppLocalizations l10n) describe;

  String get url => Links.tree(name);

  /// Short display name - the last path segment.
  String get label => name.split('/').last;
}

/// Every public tree in the monorepo, matching docs/repos/index.md.
/// Order is display order.
/// Not const - the describe tear-offs are not constant expressions.
final repositories = <RepoEntry>[
  // Software - things people run or read.
  RepoEntry('applications/vrhome', RepoGroup.software, (l) => l.repoVrhome),
  RepoEntry('applications/library', RepoGroup.software, (l) => l.repoLibrary),
  RepoEntry('applications/vrdemo', RepoGroup.software, (l) => l.repoVrdemo),
  RepoEntry('websites/docs', RepoGroup.software, (l) => l.repoDocs),
  RepoEntry('websites/website', RepoGroup.software, (l) => l.repoWebsite),

  // Port source - our own work: device tree, fixes, tooling, research.
  RepoEntry('system/tools', RepoGroup.source, (l) => l.repoTools),
  RepoEntry('system/android', RepoGroup.source, (l) => l.repoAndroid),
  RepoEntry('system/overlay', RepoGroup.source, (l) => l.repoOverlay),
  RepoEntry('system/vendor_patch', RepoGroup.source, (l) => l.repoVendorPatch),
  RepoEntry('system/shim', RepoGroup.source, (l) => l.repoShim),
  RepoEntry('system/keylayout', RepoGroup.source, (l) => l.repoKeylayout),
  RepoEntry('system/lens', RepoGroup.source, (l) => l.repoLens),
  RepoEntry('system/persist_calib', RepoGroup.source, (l) => l.repoPersistCalib),
  RepoEntry('research/notes', RepoGroup.source, (l) => l.repoNotes),
  RepoEntry('dumps/extracted', RepoGroup.source, (l) => l.repoExtracted),
  RepoEntry('pvr/pvr_dex', RepoGroup.source, (l) => l.repoPvrDex),
  RepoEntry('pvr/pvr_stack', RepoGroup.source, (l) => l.repoPvrStack),
  RepoEntry('system/fullstage', RepoGroup.source, (l) => l.repoFullstage),

  // Dumps & staging - binaries pulled from hardware or mid-pipeline
  // artefacts. Proprietary blobs are never redistributed.
  RepoEntry('dumps/images', RepoGroup.dumps, (l) => l.repoImages),
  RepoEntry('dumps/backup_nonEye', RepoGroup.dumps, (l) => l.repoBackupNonEye),
  RepoEntry('system/gsi', RepoGroup.dumps, (l) => l.repoGsi),
  RepoEntry('pvr/pvr_apps', RepoGroup.dumps, (l) => l.repoPvrApps),
  RepoEntry('pvr/pvr_applibs', RepoGroup.dumps, (l) => l.repoPvrApplibs),
  RepoEntry('pvr/pvr_apps_dexed', RepoGroup.dumps, (l) => l.repoPvrAppsDexed),
  RepoEntry('pvr/pvr_apps_signed', RepoGroup.dumps, (l) => l.repoPvrAppsSigned),
  RepoEntry('pvr/pvr_apps_injected', RepoGroup.dumps,
      (l) => l.repoPvrAppsInjected),
  RepoEntry('pvr/pvr_apps_final', RepoGroup.dumps, (l) => l.repoPvrAppsFinal),
  RepoEntry('oem/oem_apps', RepoGroup.dumps, (l) => l.repoOemApps),
  RepoEntry('oem/oem_dex', RepoGroup.dumps, (l) => l.repoOemDex),
  RepoEntry('oem/oem_injected', RepoGroup.dumps, (l) => l.repoOemInjected),
  RepoEntry('oem/oem_final', RepoGroup.dumps, (l) => l.repoOemFinal),
  RepoEntry('applications/seethrough', RepoGroup.dumps, (l) => l.repoSeethrough),
  RepoEntry('system/sensorpatch', RepoGroup.dumps, (l) => l.repoSensorpatch),
  RepoEntry('system/airsvc', RepoGroup.dumps, (l) => l.repoAirsvc),
  RepoEntry('system/fan', RepoGroup.dumps, (l) => l.repoFan),
  RepoEntry('system/overlay_pvr', RepoGroup.dumps, (l) => l.repoOverlayPvr),
  RepoEntry('vendor/cdsp', RepoGroup.dumps, (l) => l.repoCdsp),
  RepoEntry('vendor/rfsa', RepoGroup.dumps, (l) => l.repoRfsa),
  RepoEntry('vendor/qvr', RepoGroup.dumps, (l) => l.repoQvr),
  RepoEntry('vendor/qvrlibs', RepoGroup.dumps, (l) => l.repoQvrlibs),
  RepoEntry('dumps/ndi_firmware', RepoGroup.dumps, (l) => l.repoNdiFirmware),
  RepoEntry('dumps/deadunit', RepoGroup.dumps, (l) => l.repoDeadunit),
  RepoEntry('dumps/eyeunit', RepoGroup.dumps, (l) => l.repoEyeunit),
  RepoEntry('system/build', RepoGroup.dumps, (l) => l.repoBuild),
  RepoEntry('system/out', RepoGroup.dumps, (l) => l.repoOut),
  RepoEntry('research/ref', RepoGroup.dumps, (l) => l.repoRef),
];

/// Entries of one group, in catalog order.
List<RepoEntry> reposIn(RepoGroup group) =>
    repositories.where((r) => r.group == group).toList();
