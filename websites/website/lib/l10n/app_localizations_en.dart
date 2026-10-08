// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'HibiscusXR';

  @override
  String get navRepos => 'Repos';

  @override
  String get navGuide => 'Guide';

  @override
  String get navAbout => 'About';

  @override
  String get navFaq => 'FAQ';

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
  String get heroEyebrow => 'Standalone VR · LineageOS 17.1';

  @override
  String get heroTitle => 'Android back, in VR.';

  @override
  String get heroSubtitle =>
      'A custom operating system for standalone VR headsets, built on LineageOS 17.1.';

  @override
  String get heroSecondary => 'Read the docs';

  @override
  String get heroShotCaption => 'The library window floating inside vrhome.';

  @override
  String get shotMockTime => '3:52';

  @override
  String get shotMockSearch => 'Search';

  @override
  String shotMockFilter(int count) {
    return 'All ($count)';
  }

  @override
  String get shotMockSort => 'A-Z';

  @override
  String get shotMockSystem => 'System';

  @override
  String get shotAppCalendar => 'Calendar';

  @override
  String get shotAppCamera => 'Camera';

  @override
  String get shotAppChrome => 'Chrome';

  @override
  String get shotAppClock => 'Clock';

  @override
  String get shotAppContacts => 'Contacts';

  @override
  String get shotAppDrive => 'Drive';

  @override
  String get shotAppFiles => 'Files';

  @override
  String get shotAppGemini => 'Gemini';

  @override
  String get shotAppGlasses => 'Glasses';

  @override
  String get statBase => 'Android 10';

  @override
  String get statBaseLabel => 'LineageOS 17.1 via GSI';

  @override
  String get statVendor => 'Vendor untouched';

  @override
  String get statVendorLabel => 'fixes live in the overlay';

  @override
  String get statLicense => 'AGPL-3.0';

  @override
  String get statLicenseLabel => 'all original work';

  @override
  String get homeWayOverlayTitle => 'GSI plus overlay';

  @override
  String get homeWayOverlayBody =>
      'A LineageOS 17.1 GSI carries the system; a small overlay holds every fix we made. The vendor partition stays byte-identical to stock, so each problem gets solved on our side.';

  @override
  String get homeWayDeviceTitle => 'One OS, every headset';

  @override
  String get homeWayDeviceBody =>
      'Device-specific drivers and configs live in their own trees. Porting to a new headset means writing that layer, not forking the system.';

  @override
  String get homeWayNotesTitle => 'Research in the open';

  @override
  String get homeWayNotesBody =>
      'Every dead end is written down. The notes repo holds around 300 numbered files of raw findings, from first boot to the last black frame.';

  @override
  String get homeWayCta => 'Browse the repositories';

  @override
  String get homeOpenTitle => 'Open all the way down.';

  @override
  String get homeOpenBody =>
      'Everything we wrote is AGPL-3.0. Vendor binaries stay the vendor\'s - pulled from your own device, mapped in a manifest, never redistributed.';

  @override
  String get homeOpenSource => 'Browse the repo';

  @override
  String get homeDownloadTitle => 'Flash it yourself.';

  @override
  String get homeDownloadBody =>
      'A full system image - GSI, our fixes and the headset\'s own stack - sits in the out repository. One fastboot command puts it on the device.';

  @override
  String get homeDownloadCta => 'Get the image';

  @override
  String get homeDevicesEyebrow => 'Device compatibility';

  @override
  String get homeDevicesTitle => 'What it runs on.';

  @override
  String get homeDevicesBody =>
      'Every port carries its own drivers and configs. New headsets join the list as ports land.';

  @override
  String get deviceStateSupported => 'Supported, in development';

  @override
  String get deviceStatePlanned => 'Planned';

  @override
  String get deviceNeo2Name => 'Pico Neo 2';

  @override
  String get deviceNeo2Specs => 'A7B10 · Snapdragon 845 · 3840×2160';

  @override
  String get deviceNeo2Body => 'The current port and development target.';

  @override
  String get deviceQuest1Name => 'Oculus Quest 1';

  @override
  String get deviceQuest1Specs => 'Snapdragon 835 · 2880×1600 OLED';

  @override
  String get deviceNeo3Name => 'Pico Neo 3';

  @override
  String get deviceNeo3Specs => 'Snapdragon XR2 · 3664×1920';

  @override
  String get devicePlannedBody =>
      'Planned once the driver layer splits into its own tree.';

  @override
  String get reposTitle => 'Repositories';

  @override
  String get reposSubtitle =>
      'One tree per component in the HibiscusXR monorepo.';

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
      'Binaries pulled from hardware and mid-pipeline trees, kept for research. Proprietary vendor files belong to their owners and are never redistributed.';

  @override
  String reposCount(int count) {
    return '$count trees';
  }

  @override
  String get repoVrhome =>
      'Open VR home: 2D apps as floating windows, stock VR apps fullscreen.';

  @override
  String get repoVrdemo =>
      'Minimal native VR test app (pn2vr) used for compositor bring-up.';

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
  String get downloadBackupCta => 'Get HBSUP';

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
  String get faqQ1 => 'Does it actually work?';

  @override
  String get faqA1 =>
      'It boots on supported hardware, VRShell runs, head tracking is live and the VR display shows a real picture.';

  @override
  String get faqQ2 => 'Is it safe to flash?';

  @override
  String get faqA2 =>
      'There is real risk. Only the system partition gets flashed - writing a bootloader image older than the anti-rollback fuse allows can hard-brick a headset permanently. Read the flashing guide first.';

  @override
  String get faqQ3 => 'Which headset does it run on?';

  @override
  String get faqA3 =>
      'The Pico Neo 2 (A7B10 / PICOA7B10) is the only supported device today - both the Eye and non-Eye SKUs work, eye tracking is extra work on top. The OS itself is built to run across vendors, with more headsets planned as the driver layer splits out.';

  @override
  String get faqQ4 => 'Where do the proprietary vendor files come from?';

  @override
  String get faqA4 =>
      'From your own device or its stock OTA. The overlay repo carries a manifest of every blob needed - path, size, sha256 prefix, purpose - and none of them are committed to source repos.';

  @override
  String get faqQ5 => 'What is vrhome?';

  @override
  String get faqA5 =>
      'Our own VR home environment. Stock VRShell needs the closed vendor compositor; vrhome is a NativeActivity that puts 2D apps on floating panels and still launches real VR apps fullscreen.';

  @override
  String get faqQ6 => 'What is the license?';

  @override
  String get faqA6 =>
      'Everything we wrote is AGPL-3.0. Dumped vendor binaries remain property of their owners and live in dump repos for research only.';

  @override
  String get faqQ7 => 'Can I change the home environment?';

  @override
  String get faqA7 =>
      'Yes - the Settings app has a Home environment section with three picks: the passthrough camera feed, the built-in sky scene, or a custom zip pack pushed to /data/local/tmp/hibiscus/envs over adb. A pack is a map.obj whose SpawnUser part marks where you stand, plus an optional map.png screenshot and map.json metadata.';

  @override
  String get aboutTitle => 'About';

  @override
  String get aboutSubtitle => 'One OS, many headsets, every step in the open.';

  @override
  String get aboutWhatTitle => 'What it is';

  @override
  String get aboutWhatBody =>
      'Hibiscus is a custom operating system for standalone VR headsets, built on LineageOS 17.1 - Android 10 via a phh GSI. It targets headsets stuck on old Android releases with heavily proprietary VR stacks, one port at a time.';

  @override
  String get aboutHowTitle => 'How it works';

  @override
  String get aboutHowBody =>
      'GSI plus overlay plus the headset\'s own stack. The vendor partition is never touched, so every compatibility problem - the vold deadlock, the missing sound card, the ABI breaks - gets fixed on the system side with init rules and shim libraries.';

  @override
  String get aboutGroupTitle => 'The monorepo';

  @override
  String get aboutGroupBody =>
      'Everything lives in one repo on GitLab, one directory per component: the device tree, the shims, the research notes, the dumps, the VR home and this site.';

  @override
  String get aboutLicenseTitle => 'License';

  @override
  String get aboutLicenseBody =>
      'All original work is AGPL-3.0. Proprietary vendor binaries in dump repos belong to their owners and are never redistributed. The Inter typeface bundled on this site is under the SIL Open Font License 1.1.';

  @override
  String get aboutRepoCta => 'GitLab repo';

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
    return 'Copyright $year HibiscusXR';
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
  String get notFoundCta => 'Back to HibiscusXR';

  @override
  String get navCte => 'HCTE';

  @override
  String get navHbsup => 'HBSUP';

  @override
  String get cteTitle => 'HCTE';

  @override
  String get cteSubtitle =>
      'Hibiscus Controller Testing Environment - the desktop companion for Hibiscus headsets.';

  @override
  String get cteWhatTitle => 'What it is';

  @override
  String get cteWhatBody =>
      'HCTE is a desktop app (Linux, Windows, macOS) that pairs with a headset running Hibiscus and puts the whole development surface in one window. Plug in over USB or connect over Wi-Fi - no extra setup on the PC beyond adb, or none at all when the on-device CTE service is enabled.';

  @override
  String get cteFeatOverviewTitle => 'Device and controller overview';

  @override
  String get cteFeatOverviewBody =>
      'What the headset is - model, driver target, Android and Hibiscus versions, tracking mode - plus which controllers are paired, their battery and tracking state.';

  @override
  String get cteFeatDisplayTitle => 'Screen mirror';

  @override
  String get cteFeatDisplayBody =>
      'A live view of the headset\'s display at a few frames per second, enough to watch boot behavior and debug panels without wearing the headset.';

  @override
  String get cteFeatInstallTitle => 'APK install';

  @override
  String get cteFeatInstallBody =>
      'Push an APK to the device with one click, over adb or the socket channel.';

  @override
  String get cteFeatTrackingTitle => 'Live 6DoF/3DoF tracking';

  @override
  String get cteFeatTrackingBody =>
      'The head pose stream straight off the runtime: orientation, position, sample rate, a top-down trail, and per-controller poses.';

  @override
  String get cteFeatDebugTitle => 'Debug surface';

  @override
  String get cteFeatDebugBody =>
      'The full getprop table and a rolling logcat tail, for when something needs poking.';

  @override
  String get cteConnectTitle => 'How it connects';

  @override
  String get cteConnectBody =>
      'Two transports: adb over USB or wireless adb for the full feature set, and the on-device cted service on port 7340 when you don\'t want adb involved at all. cted is off by default and starts when you ask for it - setprop persist.hibiscus.cted 1 - same opt-in stance as wireless adb.';

  @override
  String get cteGetTitle => 'Getting it';

  @override
  String get cteGetBody =>
      'Desktop bundles ship with every cte-v* release on the monorepo - tarballs and zips for Linux, Windows, and macOS, mirrored to the GitLab release page so they never expire.';

  @override
  String get cteReleasesCta => 'CTE releases';

  @override
  String get cteSourceCta => 'Source';

  @override
  String get cteShotsTitle => 'What it looks like';

  @override
  String get cteShotConnect =>
      'The connect page: scanned USB/adb devices up top, wireless connect below.';

  @override
  String get cteShotOverview =>
      'Once connected: headset info, controller battery and tracking state, and the link.';

  @override
  String get cteShotDisplay =>
      'The display mirror pulling frames straight off the headset.';

  @override
  String get cteShotInstall =>
      'A finished APK push, with the install log underneath.';

  @override
  String get cteShotTracking =>
      'Head pose with the top-down trail and the raw pose log.';

  @override
  String get cteShotDebug =>
      'The debug page: a filterable getprop table next to the logcat tail.';

  @override
  String appDownloadsTitle(String app) {
    return '$app downloads';
  }

  @override
  String appDownloadsSubtitle(String app) {
    return 'Desktop bundles for $app, pulled live from the release pipeline.';
  }

  @override
  String get hbsupWindowsTitle => 'Windows is unsupported';

  @override
  String get hbsupWindowsBody =>
      'HBSUP ships Windows builds, but Windows is not a supported host OS - the app says the same thing on open, and things may not work there.';

  @override
  String get hbsupShotsTitle => 'What it looks like';

  @override
  String get hbsupShotConnect =>
      'The connect screen once adb sees the headset.';

  @override
  String get hbsupShotConnectUnsupported =>
      'On Windows the same screen opens with an unsupported-host banner.';

  @override
  String get hbsupShotBackupReady =>
      'Headset connected, folder picked, partitions sized - ready to dump.';

  @override
  String get hbsupShotBackupBlocked =>
      'A run that cannot start: the destination is short on space.';

  @override
  String get hbsupShotBackupDone => 'A finished dump, with the log underneath.';

  @override
  String get navFlashdocs => 'Flashing';

  @override
  String get flashdocsTitle => 'Flashing docs';

  @override
  String get flashdocsSidebarHome => 'Getting started';

  @override
  String get flashdocsDevicesTitle => 'Devices';

  @override
  String get flashdocsPickDeviceTitle => 'Which headset do you have?';

  @override
  String get flashdocsPickDeviceBody =>
      'Pick your headset to get its flashing guide. More devices land here as ports do.';

  @override
  String get flashdocsPickOsTitle => 'Which OS is your computer running?';

  @override
  String get flashdocsPickOsBody =>
      'The guide is per host OS. Only Linux is covered for now.';

  @override
  String get flashdocsOsLinux => 'Linux';

  @override
  String get flashdocsOsCardBody =>
      'adb and fastboot from your distro\'s repos.';

  @override
  String get flashdocsNeo2Title => 'Flash Hibiscus on the Pico Neo 2';

  @override
  String get flashdocsNeo2Intro =>
      'The full flow on a Linux host: root stock, back it up, then flash the system image.';

  @override
  String get flashdocsNeo2WarnTitle => 'Read this first';

  @override
  String get flashdocsNeo2WarnBody =>
      'Only the boot and system images below are safe to flash. Writing anything in the bootloader chain below the anti-rollback fuse is a permanent hard-brick on sdm845.';

  @override
  String get flashdocsNeo2ReqTitle => 'Before you start';

  @override
  String get flashdocsNeo2Req1 => 'A Linux PC with adb and fastboot';

  @override
  String get flashdocsNeo2Req2 =>
      'A Pico Neo 2 (A7B10) - Eye and non-Eye SKUs both work';

  @override
  String get flashdocsNeo2Req3 =>
      'A USB cable that carries data, not charge-only';

  @override
  String get flashdocsNeo2RootTitle => '1. Root the headset';

  @override
  String get flashdocsNeo2RootBody =>
      'Root on stock is what gives the backup step adb root. Download the Magisk-patched boot image, then run:';

  @override
  String get flashdocsNeo2RootDownload => 'Download the patched boot image';

  @override
  String get flashdocsNeo2RootCmd1 => 'adb reboot bootloader';

  @override
  String get flashdocsNeo2RootCmd2 => 'fastboot oem pico unlock';

  @override
  String get flashdocsNeo2RootCmd3 =>
      'fastboot flash boot magisk_patched_pico_neo_2_boot.img';

  @override
  String get flashdocsNeo2RootCmd4 => 'fastboot reboot';

  @override
  String get flashdocsNeo2RootNote =>
      'oem pico unlock is needed once per fastboot session. If it hangs, power the headset off, boot back into fastboot and run it again.';

  @override
  String get flashdocsNeo2BackupTitle => '2. Back up stock';

  @override
  String get flashdocsNeo2BackupBody =>
      'With rooted adb, dump every partition before flashing anything - HBSUP does it end to end, or the tools repo has a backup script if you would rather do it by hand. Keep the dump somewhere safe: it is your only way back to stock.';

  @override
  String get flashdocsNeo2FlashTitle => '3. Flash Hibiscus';

  @override
  String get flashdocsNeo2FlashBody =>
      'With the full system image downloaded, put the headset back into fastboot and run:';

  @override
  String get flashdocsNeo2FlashCmd1 => 'adb reboot bootloader';

  @override
  String get flashdocsNeo2FlashCmd2 => 'fastboot oem pico unlock';

  @override
  String get flashdocsNeo2FlashCmd3 =>
      'fastboot -S 128M flash system system-hibiscus-full-neo2.img';

  @override
  String get flashdocsNeo2FlashCmd4 => 'fastboot reboot';

  @override
  String get flashdocsNeo2FlashNote =>
      'The -S 128M chunk size is mandatory: larger chunks kill the USB link mid-flash.';

  @override
  String get flashdocsNeo2DoneTitle => '4. Done';

  @override
  String get flashdocsNeo2DoneBody =>
      'The headset restarts into Hibiscus. First boot takes a couple of minutes while the Pico stack settles.';
}
