// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'Neosalsa';

  @override
  String get navStatus => 'Status';

  @override
  String get navRepos => 'Repos';

  @override
  String get navScreenshots => 'Screenshots';

  @override
  String get navDownload => 'Download';

  @override
  String get navAbout => 'About';

  @override
  String get navFaq => 'FAQ';

  @override
  String get navDocs => 'Docs';

  @override
  String get navMenu => 'Menu';

  @override
  String get navClose => 'Close';

  @override
  String get themeSystem => 'System';

  @override
  String get themeLight => 'Light';

  @override
  String get themeDark => 'Dark';

  @override
  String get themeLabel => 'Appearance';

  @override
  String get languageLabel => 'Language';

  @override
  String get heroEyebrow => 'Pico Neo 2 · LineageOS 17.1';

  @override
  String get heroTitle => 'Android back, in VR.';

  @override
  String get heroSubtitle =>
      'A full port of LineageOS 17.1 to the Pico Neo 2 headset.';

  @override
  String get heroPrimary => 'See the status';

  @override
  String get heroSecondary => 'Read the docs';

  @override
  String get heroShotCaption => 'The Neosalsa headset mark.';

  @override
  String get statSoc => 'Snapdragon 845';

  @override
  String get statSocLabel => 'with Adreno 630';

  @override
  String get statPanel => '3840×2160 @ 72 Hz';

  @override
  String get statPanelLabel => 'JDI 4K panel';

  @override
  String get statRepos => '46';

  @override
  String get statReposLabel => 'Repositories';

  @override
  String get statNotes => '~300';

  @override
  String get statNotesLabel => 'Research notes';

  @override
  String get homeShellEyebrow => 'The shell';

  @override
  String get homeShellTitle => 'An open VR home, already running.';

  @override
  String get homeShellBody =>
      'vrhome replaces the stock Pico shell with a NativeActivity of our own: 2D apps float as windows, VR apps still launch fullscreen, and the library grid is just another app.';

  @override
  String get homeShellCta => 'See screenshots';

  @override
  String get shotCollectionCaption => 'Collections and groups, with counts.';

  @override
  String get shotMenuCaption => 'The tile menu on a long press.';

  @override
  String get homeWayOverlayTitle => 'GSI plus overlay';

  @override
  String get homeWayOverlayBody =>
      'A LineageOS 17.1 GSI carries the system; a small overlay holds every fix we made. The vendor partition stays byte-identical to stock, so each problem gets solved on our side.';

  @override
  String get homeWayReposTitle => 'One repo per component';

  @override
  String get homeWayReposBody =>
      'Device tree, shim sources, dump repos, staging trees - 46 repositories under one group, each small enough to read in one sitting.';

  @override
  String get homeWayNotesTitle => 'Research in the open';

  @override
  String get homeWayNotesBody =>
      'Every dead end is written down. The notes repo holds around 300 numbered files of raw findings, from first boot to the last black frame.';

  @override
  String get homeWayCta => 'Browse the repositories';

  @override
  String get homeStatusEyebrow => 'Where it stands';

  @override
  String get homeStatusTitle => 'Boots. Shell runs. One bug left.';

  @override
  String get homeStatusBody =>
      'The port boots with audio, live head rotation and VRShell driving the real Pico compositor. The VR display still shows black - one tracking-state bug between here and a picture.';

  @override
  String get homeStatusCta => 'Full status';

  @override
  String get homeOpenTitle => 'Open all the way down.';

  @override
  String get homeOpenBody =>
      'Everything we wrote is AGPL-3.0. Pico\'s binaries stay Pico\'s - pulled from your own device, mapped in a manifest, never redistributed.';

  @override
  String get homeOpenSource => 'Browse the group';

  @override
  String get homeDownloadTitle => 'Flash it yourself.';

  @override
  String get homeDownloadBody =>
      'A full system image - GSI, our fixes and the complete Pico stack - sits in the out repository. One fastboot command puts it on the headset.';

  @override
  String get homeDownloadCta => 'Get the image';

  @override
  String get statusTitle => 'Status';

  @override
  String get statusSubtitle => 'The honest list. It moves as the port does.';

  @override
  String get statusWorksTitle => 'Working';

  @override
  String get statusWorks1 =>
      'Boots clean: audio, 3840×2160 landscape, suspend matching stock';

  @override
  String get statusWorks2 =>
      'pvrservice publishes live head rotation - valid unit quaternions';

  @override
  String get statusWorks3 =>
      'VRShell launches reliably and drives the real Pico compositor';

  @override
  String get statusWorks4 => 'airservice, virtual_input and pn2_qvrd all start';

  @override
  String get statusWorks5 =>
      'See-through calibration app installed, platform-signed, launching';

  @override
  String get statusWorks6 => '2D Pico apps render - VRUserCenter, Pico Store';

  @override
  String get statusWorks7 => 'Optional wireless adb via persist.pn2.adbwifi';

  @override
  String get statusBrokenTitle => 'Not yet';

  @override
  String get statusBroken1 =>
      'Passthrough imagery - libgui calls deleted in Android 10';

  @override
  String get statusBroken2 =>
      'CVService controllers - crashes on a wifi broadcast, disabled';

  @override
  String get statusBroken3 =>
      'Provision setup wizard - crashes in its language picker';

  @override
  String get statusBlockerTitle => 'One bug between here and a picture.';

  @override
  String get statusBlockerBody =>
      'pvrservice hands out good rotation, but the SDK inside each app reports trackingstate 0x0,0x0. The pose fails the compositor\'s unit-quaternion check and every frame is dropped. The data exists on the service side and arrives as \"no tracking\" on the client side.';

  @override
  String get statusBlockerCta => 'Read the internals docs';

  @override
  String get reposTitle => 'Repositories';

  @override
  String get reposSubtitle =>
      'One repo per component under the neosalsa group.';

  @override
  String get reposSoftwareTitle => 'Software';

  @override
  String get reposSoftwareBody => 'Things people run or read.';

  @override
  String get reposSourceTitle => 'Port source';

  @override
  String get reposSourceBody =>
      'Device tree, fixes, tooling and the research log - all our own work.';

  @override
  String get reposDumpsTitle => 'Dumps & staging';

  @override
  String get reposDumpsBody =>
      'Binaries pulled from hardware and mid-pipeline trees, kept for research. Proprietary Pico files belong to Pico and are never redistributed.';

  @override
  String reposCount(int count) {
    return '$count repos';
  }

  @override
  String get repoVrhome =>
      'Open VR home: 2D apps as floating windows, Pico VR apps fullscreen.';

  @override
  String get repoLibrary =>
      'Flutter app grid inside vrhome - search, pin, group, launch.';

  @override
  String get repoVrdemo =>
      'Minimal native VR test app (pn2vr) used for compositor bring-up.';

  @override
  String get repoDocs =>
      'The MkDocs documentation site - guides, internals, repo map.';

  @override
  String get repoWebsite => 'This site.';

  @override
  String get repoTools =>
      'Every script for the port, sorted by job: recon, build, flash.';

  @override
  String get repoAndroid =>
      'LineageOS device tree device/pico/A7B10, read from stock firmware.';

  @override
  String get repoOverlay =>
      'Files laid over the GSI: init rc fixes, patched libs, blob manifest.';

  @override
  String get repoVendorPatch => 'Vendor-side init and vintf patch files.';

  @override
  String get repoShim =>
      'Source for the ABI shims bridging 8.1 binaries to Android 10.';

  @override
  String get repoKeylayout =>
      'Input keylayout files for the headset\'s buttons.';

  @override
  String get repoLens =>
      'Lens, distortion and svrapi configs from /vendor/etc/qvr.';

  @override
  String get repoPersistCalib =>
      'Calibration files that live on /persist - camera, lens.';

  @override
  String get repoNotes =>
      'The research log - around 300 numbered files of raw findings.';

  @override
  String get repoExtracted =>
      'Decompiled boot images, dtbs, props and VR binaries.';

  @override
  String get repoPvrDex => 'Deodexed dex code of the PVR system apps.';

  @override
  String get repoPvrStack =>
      'PVR service binaries, libraries and configs pulled from stock.';

  @override
  String get repoFullstage =>
      'Staging tree mirroring /system for the full image build.';

  @override
  String get repoImages =>
      'Stock PUI 4.1.3 OTA, rebuilt images and a LUN0 snapshot.';

  @override
  String get repoBackupNonEye =>
      'Full partition backup of the non-Eye unit - the rollback source.';

  @override
  String get repoGsi =>
      'The LineageOS 17.1 GSI base and its raw ext4 conversion.';

  @override
  String get repoPvrApps => 'All PVR system apps pulled from stock, apk + oat.';

  @override
  String get repoPvrApplibs =>
      'App-private lib/ dirs that sit beside each system apk.';

  @override
  String get repoPvrAppsDexed => 'Deodex stage of the PVR repack pipeline.';

  @override
  String get repoPvrAppsSigned => 'Re-sign stage of the repack pipeline.';

  @override
  String get repoPvrAppsInjected =>
      'Native-lib injection stage of the repack pipeline.';

  @override
  String get repoPvrAppsFinal => 'Final repacked and signed PVR apps.';

  @override
  String get repoOemApps =>
      'Raw /oem partition apps - PVRLauncher, PVRHome and friends.';

  @override
  String get repoOemDex => 'Deodex stage for the /oem apps.';

  @override
  String get repoOemInjected =>
      '/oem apps with native libs injected before signing.';

  @override
  String get repoOemFinal => 'Final repacked and signed /oem apps.';

  @override
  String get repoSeethrough =>
      'The seethrough calibration app and its native libs.';

  @override
  String get repoSensorpatch => 'Binary-patch work area for libsensorservice.';

  @override
  String get repoAirsvc =>
      'Stock airservice and virtual_input daemons plus rc files.';

  @override
  String get repoFan => 'Stock fancontrol and thermalserviced binaries.';

  @override
  String get repoOverlayPvr =>
      'Pico\'s resource overlays and public.libraries.txt.';

  @override
  String get repoCdsp => 'Qualcomm CDSP RPC libraries from stock vendor.';

  @override
  String get repoRfsa => 'Hexagon DSP skel libs and rfsa filesystem pieces.';

  @override
  String get repoQvr => 'QVR service client libraries, both ABIs.';

  @override
  String get repoQvrlibs =>
      'QVR vendor libraries, including the Tobii eye-core stubs.';

  @override
  String get repoNdiFirmware =>
      'NDI eye-tracker firmware and w25q flasher ELFs.';

  @override
  String get repoDeadunit => 'SPI NOR dumps from a dead unit\'s eye board.';

  @override
  String get repoEyeunit => 'SPI flash dumps from a working eye-tracking unit.';

  @override
  String get repoBuild =>
      'Locally generated signing keys - real keys are never committed.';

  @override
  String get repoOut =>
      'Built images: system-pn2-full.img and the compiled shims.';

  @override
  String get repoRef => 'Local clone of alvr-pico-legacy kept for reference.';

  @override
  String get screenshotsTitle => 'Screenshots';

  @override
  String get screenshotsSubtitle =>
      'vrhome and its library window, on real hardware.';

  @override
  String get shotGridCaption => 'The app grid inside vrhome.';

  @override
  String get downloadTitle => 'Download';

  @override
  String get downloadSubtitle =>
      'Flash it yourself - but read the warning first.';

  @override
  String get downloadAlphaTitle => 'Alpha software';

  @override
  String get downloadAlphaBody =>
      'This is very early alpha. These are testing builds, not production releases. Things will break, features are missing, and the VR display is still black. Only flash if you know what you\'re doing and want to help test.';

  @override
  String get downloadWarnTitle => 'Flashing risk';

  @override
  String get downloadWarnBody =>
      'Only ever flash the system partition. Writing anything in the bootloader chain below the anti-rollback fuse is a permanent hard-brick on sdm845.';

  @override
  String get downloadBackupTitle => 'Back up first';

  @override
  String get downloadBackupBody =>
      'Flashing replaces your system partition for good. Before anything else, take a full backup of the stock system - if something goes wrong, that dump is your only way back.';

  @override
  String get downloadBackupStep1 => 'Boot stock and get rooted adb: adb root';

  @override
  String get downloadBackupStep2 =>
      'Dump every partition with dd over adb shell - the tools repo has a backup script that does it end to end';

  @override
  String get downloadBackupStep3 =>
      'Pull the dump to your computer and keep it somewhere safe';

  @override
  String get downloadBackupConfirm => 'I created a full backup of my headset';

  @override
  String get downloadLockedHint => 'Confirm your backup above to reveal';

  @override
  String get downloadImageTitle => 'The full system image';

  @override
  String get downloadImageBody =>
      'system-pn2-full.img - 3.6 GB, ext4, fsck-clean. The LineageOS GSI plus our fixes plus the complete Pico stack, ready to flash.';

  @override
  String get downloadImageCta => 'Open the out repo';

  @override
  String get downloadStepsTitle => 'Flashing';

  @override
  String get downloadStep1 => 'adb reboot bootloader';

  @override
  String get downloadStep2 => 'fastboot oem pico unlock';

  @override
  String get downloadStep3 =>
      'fastboot -S 128M flash system system-pn2-full.img';

  @override
  String get downloadStep4 => 'fastboot reboot';

  @override
  String get downloadStepsNote =>
      'The -S 128M chunk size is mandatory: larger chunks kill the USB link mid-flash. The tools repo has a script that handles both quirks for you.';

  @override
  String get downloadReqTitle => 'What you need';

  @override
  String get downloadReq1 =>
      'A Pico Neo 2 (A7B10) - Eye and non-Eye SKUs both work';

  @override
  String get downloadReq2 =>
      'Rooted stock firmware and an unlockable bootloader';

  @override
  String get downloadReq3 => 'A Linux host with adb and fastboot';

  @override
  String get downloadSoftwareTitle => 'The software on top';

  @override
  String get downloadSoftwareBody =>
      'The image ships vrhome and the library app. To hack on either, clone its repo - vrhome builds with a plain Makefile, library with Flutter.';

  @override
  String get downloadVrhomeCta => 'vrhome repo';

  @override
  String get downloadLibraryCta => 'library repo';

  @override
  String get issuesButton => 'Report an issue';

  @override
  String get issuesSubtitle => 'Bugs, feature requests, help - all welcome.';

  @override
  String get buildsTitle => 'Available builds';

  @override
  String get buildsSubtitle =>
      'Pulled live from the dist pipeline. Alpha and beta are prereleases.';

  @override
  String get buildsChannelRelease => 'Release';

  @override
  String get buildsChannelBeta => 'Beta';

  @override
  String get buildsChannelAlpha => 'Alpha';

  @override
  String get buildsEmpty => 'No builds in this channel yet.';

  @override
  String get buildsError => 'Couldn\'t load builds. Try again later.';

  @override
  String get buildsRetry => 'Retry';

  @override
  String get buildsFullImage => 'Full image';

  @override
  String get buildsCleanImage => 'Clean image';

  @override
  String get buildsLogs => 'Build logs';

  @override
  String get buildsChecksums => 'Checksums';

  @override
  String get buildsViewRelease => 'View on GitLab';

  @override
  String get buildsLoading => 'Loading builds…';

  @override
  String get faqTitle => 'FAQ';

  @override
  String get faqSubtitle => 'Short answers, no marketing.';

  @override
  String get faqQ1 => 'Does the port actually work?';

  @override
  String get faqA1 =>
      'It boots, VRShell runs and head tracking is live. The one blocker: the VR display still shows black because app-side tracking state arrives as zero. The status page keeps the current list.';

  @override
  String get faqQ2 => 'Is it safe to flash?';

  @override
  String get faqA2 =>
      'There is real risk. Only the system partition gets flashed - writing a bootloader image older than the anti-rollback fuse allows hard-bricks sdm845 permanently. Read the docs flashing guide first.';

  @override
  String get faqQ3 => 'Which headset does it run on?';

  @override
  String get faqA3 =>
      'The Pico Neo 2 (A7B10 / PICOA7B10). Both the Eye and non-Eye SKUs work; eye tracking is extra work on top.';

  @override
  String get faqQ4 => 'Where do Pico\'s proprietary files come from?';

  @override
  String get faqA4 =>
      'From your own device or its stock OTA. The overlay repo carries a manifest of every blob needed - path, size, sha256 prefix, purpose - and none of them are committed to source repos.';

  @override
  String get faqQ5 => 'What is vrhome?';

  @override
  String get faqA5 =>
      'Our own VR home environment. Stock VRShell needs the closed Pico compositor; vrhome is a NativeActivity that puts 2D apps on floating panels and still launches real VR apps fullscreen.';

  @override
  String get faqQ6 => 'What is the license?';

  @override
  String get faqA6 =>
      'Everything we wrote is AGPL-3.0. Dumped Pico and vendor binaries remain property of their owners and live in dump repos for research only.';

  @override
  String get aboutTitle => 'About';

  @override
  String get aboutSubtitle => 'One port, one headset, every step in the open.';

  @override
  String get aboutWhatTitle => 'What it is';

  @override
  String get aboutWhatBody =>
      'Neosalsa runs LineageOS 17.1 - Android 10 via a phh GSI - on the Pico Neo 2, a headset that shipped with Android 8.1 and a heavily proprietary VR stack.';

  @override
  String get aboutHowTitle => 'How it works';

  @override
  String get aboutHowBody =>
      'GSI plus overlay plus your own Pico stack. The vendor partition is never touched, so every compatibility problem - the vold deadlock, the missing sound card, the ABI breaks - gets fixed on the system side with init rules and shim libraries.';

  @override
  String get aboutGroupTitle => 'The group';

  @override
  String get aboutGroupBody =>
      'Forty-six repositories under neosalsa, one per component: the device tree, the shims, the research notes, the dump repos, the VR home and this site.';

  @override
  String get aboutLicenseTitle => 'License';

  @override
  String get aboutLicenseBody =>
      'All original work is AGPL-3.0. Proprietary Pico and vendor binaries in dump repos belong to their owners and are never redistributed. The Inter typeface bundled on this site is under the SIL Open Font License 1.1.';

  @override
  String get aboutRepoCta => 'GitLab group';

  @override
  String get aboutDocsCta => 'Project docs';

  @override
  String get aboutNotesCta => 'Research notes';

  @override
  String get footerSite => 'Site';

  @override
  String get footerProject => 'Project';

  @override
  String footerCopyright(int year) {
    return 'Copyright $year neosalsa';
  }

  @override
  String get footerLicense => 'Licensed under the AGPL-3.0';

  @override
  String get footerBuiltWith => 'Built with Flutter';

  @override
  String get notFoundTitle => 'Page not found';

  @override
  String get notFoundBody => 'The page you are looking for does not exist.';

  @override
  String get notFoundCta => 'Back to Neosalsa';
}
