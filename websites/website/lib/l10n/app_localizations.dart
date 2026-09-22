import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:intl/intl.dart' as intl;

import 'app_localizations_en.dart';
import 'app_localizations_zh.dart';

// ignore_for_file: type=lint

/// Callers can lookup localized strings with an instance of AppLocalizations
/// returned by `AppLocalizations.of(context)`.
///
/// Applications need to include `AppLocalizations.delegate()` in their app's
/// `localizationDelegates` list, and the locales they support in the app's
/// `supportedLocales` list. For example:
///
/// ```dart
/// import 'l10n/app_localizations.dart';
///
/// return MaterialApp(
///   localizationsDelegates: AppLocalizations.localizationsDelegates,
///   supportedLocales: AppLocalizations.supportedLocales,
///   home: MyApplicationHome(),
/// );
/// ```
///
/// ## Update pubspec.yaml
///
/// Please make sure to update your pubspec.yaml to include the following
/// packages:
///
/// ```yaml
/// dependencies:
///   # Internationalization support.
///   flutter_localizations:
///     sdk: flutter
///   intl: any # Use the pinned version from flutter_localizations
///
///   # Rest of dependencies
/// ```
///
/// ## iOS Applications
///
/// iOS applications define key application metadata, including supported
/// locales, in an Info.plist file that is built into the application bundle.
/// To configure the locales supported by your app, you’ll need to edit this
/// file.
///
/// First, open your project’s ios/Runner.xcworkspace Xcode workspace file.
/// Then, in the Project Navigator, open the Info.plist file under the Runner
/// project’s Runner folder.
///
/// Next, select the Information Property List item, select Add Item from the
/// Editor menu, then select Localizations from the pop-up menu.
///
/// Select and expand the newly-created Localizations item then, for each
/// locale your application supports, add a new item and select the locale
/// you wish to add from the pop-up menu in the Value field. This list should
/// be consistent with the languages listed in the AppLocalizations.supportedLocales
/// property.
abstract class AppLocalizations {
  AppLocalizations(String locale)
    : localeName = intl.Intl.canonicalizedLocale(locale.toString());

  final String localeName;

  static AppLocalizations of(BuildContext context) {
    return Localizations.of<AppLocalizations>(context, AppLocalizations)!;
  }

  static const LocalizationsDelegate<AppLocalizations> delegate =
      _AppLocalizationsDelegate();

  /// A list of this localizations delegate along with the default localizations
  /// delegates.
  ///
  /// Returns a list of localizations delegates containing this delegate along with
  /// GlobalMaterialLocalizations.delegate, GlobalCupertinoLocalizations.delegate,
  /// and GlobalWidgetsLocalizations.delegate.
  ///
  /// Additional delegates can be added by appending to this list in
  /// MaterialApp. This list does not have to be used at all if a custom list
  /// of delegates is preferred or required.
  static const List<LocalizationsDelegate<dynamic>> localizationsDelegates =
      <LocalizationsDelegate<dynamic>>[
        delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
      ];

  /// A list of this localizations delegate's supported locales.
  static const List<Locale> supportedLocales = <Locale>[
    Locale('en'),
    Locale('zh'),
  ];

  /// Project name
  ///
  /// In en, this message translates to:
  /// **'HibiscusXR'**
  String get appTitle;

  /// No description provided for @navStatus.
  ///
  /// In en, this message translates to:
  /// **'Status'**
  String get navStatus;

  /// No description provided for @navRepos.
  ///
  /// In en, this message translates to:
  /// **'Repos'**
  String get navRepos;

  /// No description provided for @navScreenshots.
  ///
  /// In en, this message translates to:
  /// **'Screenshots'**
  String get navScreenshots;

  /// No description provided for @navDownload.
  ///
  /// In en, this message translates to:
  /// **'Download'**
  String get navDownload;

  /// No description provided for @navAbout.
  ///
  /// In en, this message translates to:
  /// **'About'**
  String get navAbout;

  /// No description provided for @navFaq.
  ///
  /// In en, this message translates to:
  /// **'FAQ'**
  String get navFaq;

  /// No description provided for @navDocs.
  ///
  /// In en, this message translates to:
  /// **'Docs'**
  String get navDocs;

  /// No description provided for @navMenu.
  ///
  /// In en, this message translates to:
  /// **'Menu'**
  String get navMenu;

  /// No description provided for @navClose.
  ///
  /// In en, this message translates to:
  /// **'Close'**
  String get navClose;

  /// No description provided for @themeSystem.
  ///
  /// In en, this message translates to:
  /// **'System'**
  String get themeSystem;

  /// No description provided for @themeLight.
  ///
  /// In en, this message translates to:
  /// **'Light'**
  String get themeLight;

  /// No description provided for @themeDark.
  ///
  /// In en, this message translates to:
  /// **'Dark'**
  String get themeDark;

  /// No description provided for @themeLabel.
  ///
  /// In en, this message translates to:
  /// **'Appearance'**
  String get themeLabel;

  /// No description provided for @languageLabel.
  ///
  /// In en, this message translates to:
  /// **'Language'**
  String get languageLabel;

  /// No description provided for @heroEyebrow.
  ///
  /// In en, this message translates to:
  /// **'Pico Neo 2 · LineageOS 17.1'**
  String get heroEyebrow;

  /// No description provided for @heroTitle.
  ///
  /// In en, this message translates to:
  /// **'Android back, in VR.'**
  String get heroTitle;

  /// No description provided for @heroSubtitle.
  ///
  /// In en, this message translates to:
  /// **'A full port of LineageOS 17.1 to the Pico Neo 2 headset.'**
  String get heroSubtitle;

  /// No description provided for @heroPrimary.
  ///
  /// In en, this message translates to:
  /// **'See the status'**
  String get heroPrimary;

  /// No description provided for @heroSecondary.
  ///
  /// In en, this message translates to:
  /// **'Read the docs'**
  String get heroSecondary;

  /// No description provided for @heroShotCaption.
  ///
  /// In en, this message translates to:
  /// **'The library window floating inside vrhome.'**
  String get heroShotCaption;

  /// No description provided for @shotMockTime.
  ///
  /// In en, this message translates to:
  /// **'3:52'**
  String get shotMockTime;

  /// No description provided for @shotMockSearch.
  ///
  /// In en, this message translates to:
  /// **'Search'**
  String get shotMockSearch;

  /// No description provided for @shotMockFilter.
  ///
  /// In en, this message translates to:
  /// **'All ({count})'**
  String shotMockFilter(int count);

  /// No description provided for @shotMockSort.
  ///
  /// In en, this message translates to:
  /// **'A-Z'**
  String get shotMockSort;

  /// No description provided for @shotMockSystem.
  ///
  /// In en, this message translates to:
  /// **'System'**
  String get shotMockSystem;

  /// No description provided for @shotAppCalendar.
  ///
  /// In en, this message translates to:
  /// **'Calendar'**
  String get shotAppCalendar;

  /// No description provided for @shotAppCamera.
  ///
  /// In en, this message translates to:
  /// **'Camera'**
  String get shotAppCamera;

  /// No description provided for @shotAppChrome.
  ///
  /// In en, this message translates to:
  /// **'Chrome'**
  String get shotAppChrome;

  /// No description provided for @shotAppClock.
  ///
  /// In en, this message translates to:
  /// **'Clock'**
  String get shotAppClock;

  /// No description provided for @shotAppContacts.
  ///
  /// In en, this message translates to:
  /// **'Contacts'**
  String get shotAppContacts;

  /// No description provided for @shotAppDrive.
  ///
  /// In en, this message translates to:
  /// **'Drive'**
  String get shotAppDrive;

  /// No description provided for @shotAppFiles.
  ///
  /// In en, this message translates to:
  /// **'Files'**
  String get shotAppFiles;

  /// No description provided for @shotAppGemini.
  ///
  /// In en, this message translates to:
  /// **'Gemini'**
  String get shotAppGemini;

  /// No description provided for @shotAppGlasses.
  ///
  /// In en, this message translates to:
  /// **'Glasses'**
  String get shotAppGlasses;

  /// No description provided for @statSoc.
  ///
  /// In en, this message translates to:
  /// **'Snapdragon 845'**
  String get statSoc;

  /// No description provided for @statSocLabel.
  ///
  /// In en, this message translates to:
  /// **'with Adreno 630'**
  String get statSocLabel;

  /// No description provided for @statPanel.
  ///
  /// In en, this message translates to:
  /// **'3840×2160 @ 72 Hz'**
  String get statPanel;

  /// No description provided for @statPanelLabel.
  ///
  /// In en, this message translates to:
  /// **'JDI 4K panel'**
  String get statPanelLabel;

  /// No description provided for @statNotes.
  ///
  /// In en, this message translates to:
  /// **'~300'**
  String get statNotes;

  /// No description provided for @statNotesLabel.
  ///
  /// In en, this message translates to:
  /// **'Research notes'**
  String get statNotesLabel;

  /// No description provided for @shotCollectionCaption.
  ///
  /// In en, this message translates to:
  /// **'Collections and groups, with counts.'**
  String get shotCollectionCaption;

  /// No description provided for @shotMenuCaption.
  ///
  /// In en, this message translates to:
  /// **'The tile menu on a long press.'**
  String get shotMenuCaption;

  /// No description provided for @homeWayOverlayTitle.
  ///
  /// In en, this message translates to:
  /// **'GSI plus overlay'**
  String get homeWayOverlayTitle;

  /// No description provided for @homeWayOverlayBody.
  ///
  /// In en, this message translates to:
  /// **'A LineageOS 17.1 GSI carries the system; a small overlay holds every fix we made. The vendor partition stays byte-identical to stock, so each problem gets solved on our side.'**
  String get homeWayOverlayBody;

  /// No description provided for @homeWayNotesTitle.
  ///
  /// In en, this message translates to:
  /// **'Research in the open'**
  String get homeWayNotesTitle;

  /// No description provided for @homeWayNotesBody.
  ///
  /// In en, this message translates to:
  /// **'Every dead end is written down. The notes repo holds around 300 numbered files of raw findings, from first boot to the last black frame.'**
  String get homeWayNotesBody;

  /// No description provided for @homeWayCta.
  ///
  /// In en, this message translates to:
  /// **'Browse the repositories'**
  String get homeWayCta;

  /// No description provided for @homeStatusEyebrow.
  ///
  /// In en, this message translates to:
  /// **'Where it stands'**
  String get homeStatusEyebrow;

  /// No description provided for @homeStatusTitle.
  ///
  /// In en, this message translates to:
  /// **'Boots. Shell runs. Picture\'s on.'**
  String get homeStatusTitle;

  /// No description provided for @homeStatusBody.
  ///
  /// In en, this message translates to:
  /// **'The port boots with audio, live head rotation and VRShell driving the real Pico compositor - and the VR display shows a real picture.'**
  String get homeStatusBody;

  /// No description provided for @homeStatusCta.
  ///
  /// In en, this message translates to:
  /// **'Full status'**
  String get homeStatusCta;

  /// No description provided for @homeOpenTitle.
  ///
  /// In en, this message translates to:
  /// **'Open all the way down.'**
  String get homeOpenTitle;

  /// No description provided for @homeOpenBody.
  ///
  /// In en, this message translates to:
  /// **'Everything we wrote is AGPL-3.0. Pico\'s binaries stay Pico\'s - pulled from your own device, mapped in a manifest, never redistributed.'**
  String get homeOpenBody;

  /// No description provided for @homeOpenSource.
  ///
  /// In en, this message translates to:
  /// **'Browse the repo'**
  String get homeOpenSource;

  /// No description provided for @homeDownloadTitle.
  ///
  /// In en, this message translates to:
  /// **'Flash it yourself.'**
  String get homeDownloadTitle;

  /// No description provided for @homeDownloadBody.
  ///
  /// In en, this message translates to:
  /// **'A full system image - GSI, our fixes and the complete Pico stack - sits in the out repository. One fastboot command puts it on the headset.'**
  String get homeDownloadBody;

  /// No description provided for @homeDownloadCta.
  ///
  /// In en, this message translates to:
  /// **'Get the image'**
  String get homeDownloadCta;

  /// No description provided for @statusTitle.
  ///
  /// In en, this message translates to:
  /// **'Status'**
  String get statusTitle;

  /// No description provided for @statusSubtitle.
  ///
  /// In en, this message translates to:
  /// **'The honest list. It moves as the port does.'**
  String get statusSubtitle;

  /// No description provided for @statusWorksTitle.
  ///
  /// In en, this message translates to:
  /// **'Working'**
  String get statusWorksTitle;

  /// No description provided for @statusWorks1.
  ///
  /// In en, this message translates to:
  /// **'Boots clean: audio, 3840×2160 landscape, suspend matching stock'**
  String get statusWorks1;

  /// No description provided for @statusWorks2.
  ///
  /// In en, this message translates to:
  /// **'pvrservice publishes live head rotation - valid unit quaternions'**
  String get statusWorks2;

  /// No description provided for @statusWorks3.
  ///
  /// In en, this message translates to:
  /// **'VRShell launches reliably and drives the real Pico compositor'**
  String get statusWorks3;

  /// No description provided for @statusWorks4.
  ///
  /// In en, this message translates to:
  /// **'airservice, virtual_input and pn2_qvrd all start'**
  String get statusWorks4;

  /// No description provided for @statusWorks5.
  ///
  /// In en, this message translates to:
  /// **'See-through calibration app installed, platform-signed, launching'**
  String get statusWorks5;

  /// No description provided for @statusWorks6.
  ///
  /// In en, this message translates to:
  /// **'2D Pico apps render - VRUserCenter, Pico Store'**
  String get statusWorks6;

  /// No description provided for @statusWorks7.
  ///
  /// In en, this message translates to:
  /// **'Optional wireless adb via persist.pn2.adbwifi'**
  String get statusWorks7;

  /// No description provided for @statusWorks8.
  ///
  /// In en, this message translates to:
  /// **'VR display shows a real picture - tracking state reaches apps'**
  String get statusWorks8;

  /// No description provided for @statusBrokenTitle.
  ///
  /// In en, this message translates to:
  /// **'Not yet'**
  String get statusBrokenTitle;

  /// No description provided for @statusBroken1.
  ///
  /// In en, this message translates to:
  /// **'Passthrough imagery - libgui calls deleted in Android 10'**
  String get statusBroken1;

  /// No description provided for @statusBroken2.
  ///
  /// In en, this message translates to:
  /// **'CVService controllers - crashes on a wifi broadcast, disabled'**
  String get statusBroken2;

  /// No description provided for @statusBroken3.
  ///
  /// In en, this message translates to:
  /// **'Provision setup wizard - crashes in its language picker'**
  String get statusBroken3;

  /// No description provided for @statusBlockerTitle.
  ///
  /// In en, this message translates to:
  /// **'No bug between here and a picture.'**
  String get statusBlockerTitle;

  /// No description provided for @statusBlockerBody.
  ///
  /// In en, this message translates to:
  /// **'pvrservice hands out good rotation and app-side tracking state now arrives valid. Poses pass the compositor\'s unit-quaternion check and every frame lands on the display.'**
  String get statusBlockerBody;

  /// No description provided for @statusBlockerCta.
  ///
  /// In en, this message translates to:
  /// **'Read the internals docs'**
  String get statusBlockerCta;

  /// No description provided for @reposTitle.
  ///
  /// In en, this message translates to:
  /// **'Repositories'**
  String get reposTitle;

  /// No description provided for @reposSubtitle.
  ///
  /// In en, this message translates to:
  /// **'One tree per component in the HibiscusXR monorepo.'**
  String get reposSubtitle;

  /// No description provided for @reposSoftwareTitle.
  ///
  /// In en, this message translates to:
  /// **'Software'**
  String get reposSoftwareTitle;

  /// No description provided for @reposSoftwareBody.
  ///
  /// In en, this message translates to:
  /// **'Things people run or read.'**
  String get reposSoftwareBody;

  /// No description provided for @reposSourceTitle.
  ///
  /// In en, this message translates to:
  /// **'Port source'**
  String get reposSourceTitle;

  /// No description provided for @reposSourceBody.
  ///
  /// In en, this message translates to:
  /// **'Device tree, fixes, tooling and the research log - all our own work.'**
  String get reposSourceBody;

  /// No description provided for @reposDumpsTitle.
  ///
  /// In en, this message translates to:
  /// **'Dumps & staging'**
  String get reposDumpsTitle;

  /// No description provided for @reposDumpsBody.
  ///
  /// In en, this message translates to:
  /// **'Binaries pulled from hardware and mid-pipeline trees, kept for research. Proprietary Pico files belong to Pico and are never redistributed.'**
  String get reposDumpsBody;

  /// No description provided for @reposCount.
  ///
  /// In en, this message translates to:
  /// **'{count} trees'**
  String reposCount(int count);

  /// No description provided for @repoVrhome.
  ///
  /// In en, this message translates to:
  /// **'Open VR home: 2D apps as floating windows, Pico VR apps fullscreen.'**
  String get repoVrhome;

  /// No description provided for @repoLibrary.
  ///
  /// In en, this message translates to:
  /// **'Flutter app grid inside vrhome - search, pin, group, launch.'**
  String get repoLibrary;

  /// No description provided for @repoVrdemo.
  ///
  /// In en, this message translates to:
  /// **'Minimal native VR test app (pn2vr) used for compositor bring-up.'**
  String get repoVrdemo;

  /// No description provided for @repoDocs.
  ///
  /// In en, this message translates to:
  /// **'The MkDocs documentation site - guides, internals, repo map.'**
  String get repoDocs;

  /// No description provided for @repoWebsite.
  ///
  /// In en, this message translates to:
  /// **'This site.'**
  String get repoWebsite;

  /// No description provided for @repoTools.
  ///
  /// In en, this message translates to:
  /// **'Every script for the port, sorted by job: recon, build, flash.'**
  String get repoTools;

  /// No description provided for @repoAndroid.
  ///
  /// In en, this message translates to:
  /// **'LineageOS device tree device/pico/A7B10, read from stock firmware.'**
  String get repoAndroid;

  /// No description provided for @repoOverlay.
  ///
  /// In en, this message translates to:
  /// **'Files laid over the GSI: init rc fixes, patched libs, blob manifest.'**
  String get repoOverlay;

  /// No description provided for @repoVendorPatch.
  ///
  /// In en, this message translates to:
  /// **'Vendor-side init and vintf patch files.'**
  String get repoVendorPatch;

  /// No description provided for @repoShim.
  ///
  /// In en, this message translates to:
  /// **'Source for the ABI shims bridging 8.1 binaries to Android 10.'**
  String get repoShim;

  /// No description provided for @repoKeylayout.
  ///
  /// In en, this message translates to:
  /// **'Input keylayout files for the headset\'s buttons.'**
  String get repoKeylayout;

  /// No description provided for @repoLens.
  ///
  /// In en, this message translates to:
  /// **'Lens, distortion and svrapi configs from /vendor/etc/qvr.'**
  String get repoLens;

  /// No description provided for @repoPersistCalib.
  ///
  /// In en, this message translates to:
  /// **'Calibration files that live on /persist - camera, lens.'**
  String get repoPersistCalib;

  /// No description provided for @repoNotes.
  ///
  /// In en, this message translates to:
  /// **'The research log - around 300 numbered files of raw findings.'**
  String get repoNotes;

  /// No description provided for @repoExtracted.
  ///
  /// In en, this message translates to:
  /// **'Decompiled boot images, dtbs, props and VR binaries.'**
  String get repoExtracted;

  /// No description provided for @repoPvrDex.
  ///
  /// In en, this message translates to:
  /// **'Deodexed dex code of the PVR system apps.'**
  String get repoPvrDex;

  /// No description provided for @repoPvrStack.
  ///
  /// In en, this message translates to:
  /// **'PVR service binaries, libraries and configs pulled from stock.'**
  String get repoPvrStack;

  /// No description provided for @repoFullstage.
  ///
  /// In en, this message translates to:
  /// **'Staging tree mirroring /system for the full image build.'**
  String get repoFullstage;

  /// No description provided for @repoImages.
  ///
  /// In en, this message translates to:
  /// **'Stock PUI 4.1.3 OTA, rebuilt images and a LUN0 snapshot.'**
  String get repoImages;

  /// No description provided for @repoBackupNonEye.
  ///
  /// In en, this message translates to:
  /// **'Full partition backup of the non-Eye unit - the rollback source.'**
  String get repoBackupNonEye;

  /// No description provided for @repoGsi.
  ///
  /// In en, this message translates to:
  /// **'The LineageOS 17.1 GSI base and its raw ext4 conversion.'**
  String get repoGsi;

  /// No description provided for @repoPvrApps.
  ///
  /// In en, this message translates to:
  /// **'All PVR system apps pulled from stock, apk + oat.'**
  String get repoPvrApps;

  /// No description provided for @repoPvrApplibs.
  ///
  /// In en, this message translates to:
  /// **'App-private lib/ dirs that sit beside each system apk.'**
  String get repoPvrApplibs;

  /// No description provided for @repoPvrAppsDexed.
  ///
  /// In en, this message translates to:
  /// **'Deodex stage of the PVR repack pipeline.'**
  String get repoPvrAppsDexed;

  /// No description provided for @repoPvrAppsSigned.
  ///
  /// In en, this message translates to:
  /// **'Re-sign stage of the repack pipeline.'**
  String get repoPvrAppsSigned;

  /// No description provided for @repoPvrAppsInjected.
  ///
  /// In en, this message translates to:
  /// **'Native-lib injection stage of the repack pipeline.'**
  String get repoPvrAppsInjected;

  /// No description provided for @repoPvrAppsFinal.
  ///
  /// In en, this message translates to:
  /// **'Final repacked and signed PVR apps.'**
  String get repoPvrAppsFinal;

  /// No description provided for @repoOemApps.
  ///
  /// In en, this message translates to:
  /// **'Raw /oem partition apps - PVRLauncher, PVRHome and friends.'**
  String get repoOemApps;

  /// No description provided for @repoOemDex.
  ///
  /// In en, this message translates to:
  /// **'Deodex stage for the /oem apps.'**
  String get repoOemDex;

  /// No description provided for @repoOemInjected.
  ///
  /// In en, this message translates to:
  /// **'/oem apps with native libs injected before signing.'**
  String get repoOemInjected;

  /// No description provided for @repoOemFinal.
  ///
  /// In en, this message translates to:
  /// **'Final repacked and signed /oem apps.'**
  String get repoOemFinal;

  /// No description provided for @repoSeethrough.
  ///
  /// In en, this message translates to:
  /// **'The seethrough calibration app and its native libs.'**
  String get repoSeethrough;

  /// No description provided for @repoSensorpatch.
  ///
  /// In en, this message translates to:
  /// **'Binary-patch work area for libsensorservice.'**
  String get repoSensorpatch;

  /// No description provided for @repoAirsvc.
  ///
  /// In en, this message translates to:
  /// **'Stock airservice and virtual_input daemons plus rc files.'**
  String get repoAirsvc;

  /// No description provided for @repoFan.
  ///
  /// In en, this message translates to:
  /// **'Stock fancontrol and thermalserviced binaries.'**
  String get repoFan;

  /// No description provided for @repoOverlayPvr.
  ///
  /// In en, this message translates to:
  /// **'Pico\'s resource overlays and public.libraries.txt.'**
  String get repoOverlayPvr;

  /// No description provided for @repoCdsp.
  ///
  /// In en, this message translates to:
  /// **'Qualcomm CDSP RPC libraries from stock vendor.'**
  String get repoCdsp;

  /// No description provided for @repoRfsa.
  ///
  /// In en, this message translates to:
  /// **'Hexagon DSP skel libs and rfsa filesystem pieces.'**
  String get repoRfsa;

  /// No description provided for @repoQvr.
  ///
  /// In en, this message translates to:
  /// **'QVR service client libraries, both ABIs.'**
  String get repoQvr;

  /// No description provided for @repoQvrlibs.
  ///
  /// In en, this message translates to:
  /// **'QVR vendor libraries, including the Tobii eye-core stubs.'**
  String get repoQvrlibs;

  /// No description provided for @repoNdiFirmware.
  ///
  /// In en, this message translates to:
  /// **'NDI eye-tracker firmware and w25q flasher ELFs.'**
  String get repoNdiFirmware;

  /// No description provided for @repoDeadunit.
  ///
  /// In en, this message translates to:
  /// **'SPI NOR dumps from a dead unit\'s eye board.'**
  String get repoDeadunit;

  /// No description provided for @repoEyeunit.
  ///
  /// In en, this message translates to:
  /// **'SPI flash dumps from a working eye-tracking unit.'**
  String get repoEyeunit;

  /// No description provided for @repoBuild.
  ///
  /// In en, this message translates to:
  /// **'Locally generated signing keys - real keys are never committed.'**
  String get repoBuild;

  /// No description provided for @repoOut.
  ///
  /// In en, this message translates to:
  /// **'Built images: system-pn2-full.img and the compiled shims.'**
  String get repoOut;

  /// No description provided for @repoRef.
  ///
  /// In en, this message translates to:
  /// **'Local clone of alvr-pico-legacy kept for reference.'**
  String get repoRef;

  /// No description provided for @screenshotsTitle.
  ///
  /// In en, this message translates to:
  /// **'Screenshots'**
  String get screenshotsTitle;

  /// No description provided for @screenshotsSubtitle.
  ///
  /// In en, this message translates to:
  /// **'vrhome and its library window, on real hardware.'**
  String get screenshotsSubtitle;

  /// No description provided for @shotGridCaption.
  ///
  /// In en, this message translates to:
  /// **'The app grid inside vrhome.'**
  String get shotGridCaption;

  /// No description provided for @downloadTitle.
  ///
  /// In en, this message translates to:
  /// **'Download'**
  String get downloadTitle;

  /// No description provided for @downloadSubtitle.
  ///
  /// In en, this message translates to:
  /// **'Flash it yourself - but read the warning first.'**
  String get downloadSubtitle;

  /// No description provided for @downloadAlphaTitle.
  ///
  /// In en, this message translates to:
  /// **'Alpha software'**
  String get downloadAlphaTitle;

  /// No description provided for @downloadAlphaBody.
  ///
  /// In en, this message translates to:
  /// **'This is very early alpha. These are testing builds, not production releases. Things will break and features are missing. Only flash if you know what you\'re doing and want to help test.'**
  String get downloadAlphaBody;

  /// No description provided for @downloadWarnTitle.
  ///
  /// In en, this message translates to:
  /// **'Flashing risk'**
  String get downloadWarnTitle;

  /// No description provided for @downloadWarnBody.
  ///
  /// In en, this message translates to:
  /// **'Only ever flash the system partition. Writing anything in the bootloader chain below the anti-rollback fuse is a permanent hard-brick on sdm845.'**
  String get downloadWarnBody;

  /// No description provided for @downloadBackupTitle.
  ///
  /// In en, this message translates to:
  /// **'Back up first'**
  String get downloadBackupTitle;

  /// No description provided for @downloadBackupBody.
  ///
  /// In en, this message translates to:
  /// **'Flashing replaces your system partition for good. Before anything else, take a full backup of the stock system - if something goes wrong, that dump is your only way back.'**
  String get downloadBackupBody;

  /// No description provided for @downloadBackupStep1.
  ///
  /// In en, this message translates to:
  /// **'Boot stock and get rooted adb: adb root'**
  String get downloadBackupStep1;

  /// No description provided for @downloadBackupStep2.
  ///
  /// In en, this message translates to:
  /// **'Dump every partition with dd over adb shell - the tools repo has a backup script that does it end to end'**
  String get downloadBackupStep2;

  /// No description provided for @downloadBackupStep3.
  ///
  /// In en, this message translates to:
  /// **'Pull the dump to your computer and keep it somewhere safe'**
  String get downloadBackupStep3;

  /// No description provided for @downloadBackupConfirm.
  ///
  /// In en, this message translates to:
  /// **'I created a full backup of my headset'**
  String get downloadBackupConfirm;

  /// No description provided for @downloadLockedHint.
  ///
  /// In en, this message translates to:
  /// **'Confirm your backup above to reveal'**
  String get downloadLockedHint;

  /// No description provided for @downloadImageTitle.
  ///
  /// In en, this message translates to:
  /// **'The full system image'**
  String get downloadImageTitle;

  /// No description provided for @downloadImageBody.
  ///
  /// In en, this message translates to:
  /// **'system-pn2-full.img - 3.6 GB, ext4, fsck-clean. The LineageOS GSI plus our fixes plus the complete Pico stack, ready to flash.'**
  String get downloadImageBody;

  /// No description provided for @downloadImageCta.
  ///
  /// In en, this message translates to:
  /// **'Open the out repo'**
  String get downloadImageCta;

  /// No description provided for @downloadStepsTitle.
  ///
  /// In en, this message translates to:
  /// **'Flashing'**
  String get downloadStepsTitle;

  /// No description provided for @downloadStep1.
  ///
  /// In en, this message translates to:
  /// **'adb reboot bootloader'**
  String get downloadStep1;

  /// No description provided for @downloadStep2.
  ///
  /// In en, this message translates to:
  /// **'fastboot oem pico unlock'**
  String get downloadStep2;

  /// No description provided for @downloadStep3.
  ///
  /// In en, this message translates to:
  /// **'fastboot -S 128M flash system system-pn2-full.img'**
  String get downloadStep3;

  /// No description provided for @downloadStep4.
  ///
  /// In en, this message translates to:
  /// **'fastboot reboot'**
  String get downloadStep4;

  /// No description provided for @downloadStepsNote.
  ///
  /// In en, this message translates to:
  /// **'The -S 128M chunk size is mandatory: larger chunks kill the USB link mid-flash. The tools repo has a script that handles both quirks for you.'**
  String get downloadStepsNote;

  /// No description provided for @downloadReqTitle.
  ///
  /// In en, this message translates to:
  /// **'What you need'**
  String get downloadReqTitle;

  /// No description provided for @downloadReq1.
  ///
  /// In en, this message translates to:
  /// **'A Pico Neo 2 (A7B10) - Eye and non-Eye SKUs both work'**
  String get downloadReq1;

  /// No description provided for @downloadReq2.
  ///
  /// In en, this message translates to:
  /// **'Rooted stock firmware and an unlockable bootloader'**
  String get downloadReq2;

  /// No description provided for @downloadReq3.
  ///
  /// In en, this message translates to:
  /// **'A Linux host with adb and fastboot'**
  String get downloadReq3;

  /// No description provided for @issuesButton.
  ///
  /// In en, this message translates to:
  /// **'Report an issue'**
  String get issuesButton;

  /// No description provided for @issuesSubtitle.
  ///
  /// In en, this message translates to:
  /// **'Bugs, feature requests, help - all welcome.'**
  String get issuesSubtitle;

  /// No description provided for @buildsTitle.
  ///
  /// In en, this message translates to:
  /// **'Available builds'**
  String get buildsTitle;

  /// No description provided for @buildsSubtitle.
  ///
  /// In en, this message translates to:
  /// **'Pulled live from the dist pipeline. Alpha and beta are prereleases.'**
  String get buildsSubtitle;

  /// No description provided for @buildsChannelRelease.
  ///
  /// In en, this message translates to:
  /// **'Release'**
  String get buildsChannelRelease;

  /// No description provided for @buildsChannelBeta.
  ///
  /// In en, this message translates to:
  /// **'Beta'**
  String get buildsChannelBeta;

  /// No description provided for @buildsChannelAlpha.
  ///
  /// In en, this message translates to:
  /// **'Alpha'**
  String get buildsChannelAlpha;

  /// No description provided for @buildsEmpty.
  ///
  /// In en, this message translates to:
  /// **'No builds in this channel yet.'**
  String get buildsEmpty;

  /// No description provided for @buildsError.
  ///
  /// In en, this message translates to:
  /// **'Couldn\'t load builds. Try again later.'**
  String get buildsError;

  /// No description provided for @buildsRetry.
  ///
  /// In en, this message translates to:
  /// **'Retry'**
  String get buildsRetry;

  /// No description provided for @buildsFullImage.
  ///
  /// In en, this message translates to:
  /// **'Full image'**
  String get buildsFullImage;

  /// No description provided for @buildsCleanImage.
  ///
  /// In en, this message translates to:
  /// **'Clean image'**
  String get buildsCleanImage;

  /// No description provided for @buildsLogs.
  ///
  /// In en, this message translates to:
  /// **'Build logs'**
  String get buildsLogs;

  /// No description provided for @buildsChecksums.
  ///
  /// In en, this message translates to:
  /// **'Checksums'**
  String get buildsChecksums;

  /// No description provided for @buildsViewRelease.
  ///
  /// In en, this message translates to:
  /// **'View on GitLab'**
  String get buildsViewRelease;

  /// No description provided for @buildsLoading.
  ///
  /// In en, this message translates to:
  /// **'Loading builds…'**
  String get buildsLoading;

  /// No description provided for @faqTitle.
  ///
  /// In en, this message translates to:
  /// **'FAQ'**
  String get faqTitle;

  /// No description provided for @faqSubtitle.
  ///
  /// In en, this message translates to:
  /// **'Short answers, no marketing.'**
  String get faqSubtitle;

  /// No description provided for @faqQ1.
  ///
  /// In en, this message translates to:
  /// **'Does the port actually work?'**
  String get faqQ1;

  /// No description provided for @faqA1.
  ///
  /// In en, this message translates to:
  /// **'It boots, VRShell runs, head tracking is live and the VR display shows a real picture. The status page keeps the current list.'**
  String get faqA1;

  /// No description provided for @faqQ2.
  ///
  /// In en, this message translates to:
  /// **'Is it safe to flash?'**
  String get faqQ2;

  /// No description provided for @faqA2.
  ///
  /// In en, this message translates to:
  /// **'There is real risk. Only the system partition gets flashed - writing a bootloader image older than the anti-rollback fuse allows hard-bricks sdm845 permanently. Read the docs flashing guide first.'**
  String get faqA2;

  /// No description provided for @faqQ3.
  ///
  /// In en, this message translates to:
  /// **'Which headset does it run on?'**
  String get faqQ3;

  /// No description provided for @faqA3.
  ///
  /// In en, this message translates to:
  /// **'The Pico Neo 2 (A7B10 / PICOA7B10). Both the Eye and non-Eye SKUs work; eye tracking is extra work on top.'**
  String get faqA3;

  /// No description provided for @faqQ4.
  ///
  /// In en, this message translates to:
  /// **'Where do Pico\'s proprietary files come from?'**
  String get faqQ4;

  /// No description provided for @faqA4.
  ///
  /// In en, this message translates to:
  /// **'From your own device or its stock OTA. The overlay repo carries a manifest of every blob needed - path, size, sha256 prefix, purpose - and none of them are committed to source repos.'**
  String get faqA4;

  /// No description provided for @faqQ5.
  ///
  /// In en, this message translates to:
  /// **'What is vrhome?'**
  String get faqQ5;

  /// No description provided for @faqA5.
  ///
  /// In en, this message translates to:
  /// **'Our own VR home environment. Stock VRShell needs the closed Pico compositor; vrhome is a NativeActivity that puts 2D apps on floating panels and still launches real VR apps fullscreen.'**
  String get faqA5;

  /// No description provided for @faqQ6.
  ///
  /// In en, this message translates to:
  /// **'What is the license?'**
  String get faqQ6;

  /// No description provided for @faqA6.
  ///
  /// In en, this message translates to:
  /// **'Everything we wrote is AGPL-3.0. Dumped Pico and vendor binaries remain property of their owners and live in dump repos for research only.'**
  String get faqA6;

  /// No description provided for @aboutTitle.
  ///
  /// In en, this message translates to:
  /// **'About'**
  String get aboutTitle;

  /// No description provided for @aboutSubtitle.
  ///
  /// In en, this message translates to:
  /// **'One port, one headset, every step in the open.'**
  String get aboutSubtitle;

  /// No description provided for @aboutWhatTitle.
  ///
  /// In en, this message translates to:
  /// **'What it is'**
  String get aboutWhatTitle;

  /// No description provided for @aboutWhatBody.
  ///
  /// In en, this message translates to:
  /// **'Hibiscus runs LineageOS 17.1 - Android 10 via a phh GSI - on the Pico Neo 2, a headset that shipped with Android 8.1 and a heavily proprietary VR stack.'**
  String get aboutWhatBody;

  /// No description provided for @aboutHowTitle.
  ///
  /// In en, this message translates to:
  /// **'How it works'**
  String get aboutHowTitle;

  /// No description provided for @aboutHowBody.
  ///
  /// In en, this message translates to:
  /// **'GSI plus overlay plus your own Pico stack. The vendor partition is never touched, so every compatibility problem - the vold deadlock, the missing sound card, the ABI breaks - gets fixed on the system side with init rules and shim libraries.'**
  String get aboutHowBody;

  /// No description provided for @aboutGroupTitle.
  ///
  /// In en, this message translates to:
  /// **'The monorepo'**
  String get aboutGroupTitle;

  /// No description provided for @aboutGroupBody.
  ///
  /// In en, this message translates to:
  /// **'Everything lives in one repo on GitLab, one directory per component: the device tree, the shims, the research notes, the dumps, the VR home and this site.'**
  String get aboutGroupBody;

  /// No description provided for @aboutLicenseTitle.
  ///
  /// In en, this message translates to:
  /// **'License'**
  String get aboutLicenseTitle;

  /// No description provided for @aboutLicenseBody.
  ///
  /// In en, this message translates to:
  /// **'All original work is AGPL-3.0. Proprietary Pico and vendor binaries in dump repos belong to their owners and are never redistributed. The Inter typeface bundled on this site is under the SIL Open Font License 1.1.'**
  String get aboutLicenseBody;

  /// No description provided for @aboutRepoCta.
  ///
  /// In en, this message translates to:
  /// **'GitLab repo'**
  String get aboutRepoCta;

  /// No description provided for @aboutDocsCta.
  ///
  /// In en, this message translates to:
  /// **'Project docs'**
  String get aboutDocsCta;

  /// No description provided for @aboutNotesCta.
  ///
  /// In en, this message translates to:
  /// **'Research notes'**
  String get aboutNotesCta;

  /// No description provided for @footerSite.
  ///
  /// In en, this message translates to:
  /// **'Site'**
  String get footerSite;

  /// No description provided for @footerProject.
  ///
  /// In en, this message translates to:
  /// **'Project'**
  String get footerProject;

  /// No description provided for @footerCopyright.
  ///
  /// In en, this message translates to:
  /// **'Copyright {year} HibiscusXR'**
  String footerCopyright(int year);

  /// No description provided for @footerLicense.
  ///
  /// In en, this message translates to:
  /// **'Licensed under the AGPL-3.0'**
  String get footerLicense;

  /// No description provided for @footerBuiltWith.
  ///
  /// In en, this message translates to:
  /// **'Built with Flutter'**
  String get footerBuiltWith;

  /// No description provided for @notFoundTitle.
  ///
  /// In en, this message translates to:
  /// **'Page not found'**
  String get notFoundTitle;

  /// No description provided for @notFoundBody.
  ///
  /// In en, this message translates to:
  /// **'The page you are looking for does not exist.'**
  String get notFoundBody;

  /// No description provided for @notFoundCta.
  ///
  /// In en, this message translates to:
  /// **'Back to HibiscusXR'**
  String get notFoundCta;
}

class _AppLocalizationsDelegate
    extends LocalizationsDelegate<AppLocalizations> {
  const _AppLocalizationsDelegate();

  @override
  Future<AppLocalizations> load(Locale locale) {
    return SynchronousFuture<AppLocalizations>(lookupAppLocalizations(locale));
  }

  @override
  bool isSupported(Locale locale) =>
      <String>['en', 'zh'].contains(locale.languageCode);

  @override
  bool shouldReload(_AppLocalizationsDelegate old) => false;
}

AppLocalizations lookupAppLocalizations(Locale locale) {
  // Lookup logic when only language code is specified.
  switch (locale.languageCode) {
    case 'en':
      return AppLocalizationsEn();
    case 'zh':
      return AppLocalizationsZh();
  }

  throw FlutterError(
    'AppLocalizations.delegate failed to load unsupported locale "$locale". This is likely '
    'an issue with the localizations generation tool. Please file an issue '
    'on GitHub with a reproducible sample app and the gen-l10n configuration '
    'that was used.',
  );
}
