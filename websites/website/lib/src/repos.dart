import 'package:pn2_website/l10n/app_localizations.dart';

import 'links.dart';

/// Which section of the repository map an entry belongs to.
enum RepoGroup { software, source, dumps }

/// One repository under gitlab.com/neosalsa. The name is the repo path,
/// the description resolves through l10n like every other string.
class RepoEntry {
  const RepoEntry(this.name, this.group, this.describe);

  /// Repo path under the group, e.g. `applications/vrhome`.
  final String name;

  final RepoGroup group;

  /// Localised one-liner shown on the card.
  final String Function(AppLocalizations l10n) describe;

  String get url => Links.repo(name);

  /// Short display name - the last path segment.
  String get label => name.split('/').last;
}

/// Every public repo in the group, matching docs/repos/index.md plus the
/// applications/ and websites/ subgroups. Order is display order.
/// Not const - the describe tear-offs are not constant expressions.
final repositories = <RepoEntry>[
  // Software - things people run or read.
  RepoEntry('applications/vrhome', RepoGroup.software, (l) => l.repoVrhome),
  RepoEntry('applications/library', RepoGroup.software, (l) => l.repoLibrary),
  RepoEntry('vrdemo', RepoGroup.software, (l) => l.repoVrdemo),
  RepoEntry('websites/docs', RepoGroup.software, (l) => l.repoDocs),
  RepoEntry('website', RepoGroup.software, (l) => l.repoWebsite),

  // Port source - our own work: device tree, fixes, tooling, research.
  RepoEntry('tools', RepoGroup.source, (l) => l.repoTools),
  RepoEntry('android', RepoGroup.source, (l) => l.repoAndroid),
  RepoEntry('overlay', RepoGroup.source, (l) => l.repoOverlay),
  RepoEntry('vendor_patch', RepoGroup.source, (l) => l.repoVendorPatch),
  RepoEntry('shim', RepoGroup.source, (l) => l.repoShim),
  RepoEntry('keylayout', RepoGroup.source, (l) => l.repoKeylayout),
  RepoEntry('lens', RepoGroup.source, (l) => l.repoLens),
  RepoEntry('persist_calib', RepoGroup.source, (l) => l.repoPersistCalib),
  RepoEntry('notes', RepoGroup.source, (l) => l.repoNotes),
  RepoEntry('extracted', RepoGroup.source, (l) => l.repoExtracted),
  RepoEntry('pvr_dex', RepoGroup.source, (l) => l.repoPvrDex),
  RepoEntry('pvr_stack', RepoGroup.source, (l) => l.repoPvrStack),
  RepoEntry('fullstage', RepoGroup.source, (l) => l.repoFullstage),

  // Dumps & staging - binaries pulled from hardware or mid-pipeline
  // artefacts. Proprietary blobs are never redistributed.
  RepoEntry('images', RepoGroup.dumps, (l) => l.repoImages),
  RepoEntry('backup_nonEye', RepoGroup.dumps, (l) => l.repoBackupNonEye),
  RepoEntry('gsi', RepoGroup.dumps, (l) => l.repoGsi),
  RepoEntry('pvr_apps', RepoGroup.dumps, (l) => l.repoPvrApps),
  RepoEntry('pvr_applibs', RepoGroup.dumps, (l) => l.repoPvrApplibs),
  RepoEntry('pvr_apps_dexed', RepoGroup.dumps, (l) => l.repoPvrAppsDexed),
  RepoEntry('pvr_apps_signed', RepoGroup.dumps, (l) => l.repoPvrAppsSigned),
  RepoEntry('pvr_apps_injected', RepoGroup.dumps, (l) => l.repoPvrAppsInjected),
  RepoEntry('pvr_apps_final', RepoGroup.dumps, (l) => l.repoPvrAppsFinal),
  RepoEntry('oem_apps', RepoGroup.dumps, (l) => l.repoOemApps),
  RepoEntry('oem_dex', RepoGroup.dumps, (l) => l.repoOemDex),
  RepoEntry('oem_injected', RepoGroup.dumps, (l) => l.repoOemInjected),
  RepoEntry('oem_final', RepoGroup.dumps, (l) => l.repoOemFinal),
  RepoEntry('seethrough', RepoGroup.dumps, (l) => l.repoSeethrough),
  RepoEntry('sensorpatch', RepoGroup.dumps, (l) => l.repoSensorpatch),
  RepoEntry('airsvc', RepoGroup.dumps, (l) => l.repoAirsvc),
  RepoEntry('fan', RepoGroup.dumps, (l) => l.repoFan),
  RepoEntry('overlay_pvr', RepoGroup.dumps, (l) => l.repoOverlayPvr),
  RepoEntry('cdsp', RepoGroup.dumps, (l) => l.repoCdsp),
  RepoEntry('rfsa', RepoGroup.dumps, (l) => l.repoRfsa),
  RepoEntry('qvr', RepoGroup.dumps, (l) => l.repoQvr),
  RepoEntry('qvrlibs', RepoGroup.dumps, (l) => l.repoQvrlibs),
  RepoEntry('ndi_firmware', RepoGroup.dumps, (l) => l.repoNdiFirmware),
  RepoEntry('deadunit', RepoGroup.dumps, (l) => l.repoDeadunit),
  RepoEntry('eyeunit', RepoGroup.dumps, (l) => l.repoEyeunit),
  RepoEntry('build', RepoGroup.dumps, (l) => l.repoBuild),
  RepoEntry('out', RepoGroup.dumps, (l) => l.repoOut),
  RepoEntry('ref', RepoGroup.dumps, (l) => l.repoRef),
];

/// Entries of one group, in catalog order.
List<RepoEntry> reposIn(RepoGroup group) =>
    repositories.where((r) => r.group == group).toList();
