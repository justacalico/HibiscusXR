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

  /// Product name
  ///
  /// In en, this message translates to:
  /// **'Library'**
  String get appTitle;

  /// Name of the parent porting project
  ///
  /// In en, this message translates to:
  /// **'PN2Lineage'**
  String get projectName;

  /// No description provided for @navFeatures.
  ///
  /// In en, this message translates to:
  /// **'Features'**
  String get navFeatures;

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
  /// **'For the Pico Neo 2 LineageOS port'**
  String get heroEyebrow;

  /// No description provided for @heroTitle.
  ///
  /// In en, this message translates to:
  /// **'Library'**
  String get heroTitle;

  /// No description provided for @heroSubtitle.
  ///
  /// In en, this message translates to:
  /// **'Every app on the headset, in one quiet grid. Search it, pin it, group it, launch it.'**
  String get heroSubtitle;

  /// No description provided for @heroPrimary.
  ///
  /// In en, this message translates to:
  /// **'See features'**
  String get heroPrimary;

  /// No description provided for @heroSecondary.
  ///
  /// In en, this message translates to:
  /// **'Read the docs'**
  String get heroSecondary;

  /// No description provided for @homeShowcaseEyebrow.
  ///
  /// In en, this message translates to:
  /// **'App grid'**
  String get homeShowcaseEyebrow;

  /// No description provided for @homeShowcaseTitle.
  ///
  /// In en, this message translates to:
  /// **'Everything installed, at a glance.'**
  String get homeShowcaseTitle;

  /// No description provided for @homeShowcaseBody.
  ///
  /// In en, this message translates to:
  /// **'Tiles tinted by each app\'s own icon colour, with labels, badges and a kebab menu on every tile. Made to sit inside the vrhome shell as the app window.'**
  String get homeShowcaseBody;

  /// No description provided for @homeFindTitle.
  ///
  /// In en, this message translates to:
  /// **'Find anything fast'**
  String get homeFindTitle;

  /// No description provided for @homeFindBody.
  ///
  /// In en, this message translates to:
  /// **'Search ignores accents and case, collections split apps from system packages, and four sort orders keep the grid the way you left it.'**
  String get homeFindBody;

  /// No description provided for @homeArrangeTitle.
  ///
  /// In en, this message translates to:
  /// **'Your order, kept'**
  String get homeArrangeTitle;

  /// No description provided for @homeArrangeBody.
  ///
  /// In en, this message translates to:
  /// **'Pin favourites to the top, long-press-drag tiles into place, and file apps into groups you name yourself.'**
  String get homeArrangeBody;

  /// No description provided for @homeControlTitle.
  ///
  /// In en, this message translates to:
  /// **'Controller first'**
  String get homeControlTitle;

  /// No description provided for @homeControlBody.
  ///
  /// In en, this message translates to:
  /// **'Arrow keys move focus, confirm launches, and the menu key opens the tile menu. Built for a controller, usable with anything.'**
  String get homeControlBody;

  /// No description provided for @homeOpenTitle.
  ///
  /// In en, this message translates to:
  /// **'Open all the way down.'**
  String get homeOpenTitle;

  /// No description provided for @homeOpenBody.
  ///
  /// In en, this message translates to:
  /// **'Library is free software under the AGPL-3.0. Every screen, every string, every commit lives in the open on GitLab.'**
  String get homeOpenBody;

  /// No description provided for @homeOpenSource.
  ///
  /// In en, this message translates to:
  /// **'Browse the source'**
  String get homeOpenSource;

  /// No description provided for @homeDownloadTitle.
  ///
  /// In en, this message translates to:
  /// **'Not shipping yet.'**
  String get homeDownloadTitle;

  /// No description provided for @homeDownloadBody.
  ///
  /// In en, this message translates to:
  /// **'There is no release build to install today. The port is still being assembled piece by piece - watch the repository or build from source if you want it early.'**
  String get homeDownloadBody;

  /// No description provided for @homeDownloadCta.
  ///
  /// In en, this message translates to:
  /// **'Download status'**
  String get homeDownloadCta;

  /// No description provided for @featuresTitle.
  ///
  /// In en, this message translates to:
  /// **'Features'**
  String get featuresTitle;

  /// No description provided for @featuresSubtitle.
  ///
  /// In en, this message translates to:
  /// **'A launcher-shaped tool, tuned for a headset.'**
  String get featuresSubtitle;

  /// No description provided for @featGridTitle.
  ///
  /// In en, this message translates to:
  /// **'A grid that reads like a shelf'**
  String get featGridTitle;

  /// No description provided for @featGridBody.
  ///
  /// In en, this message translates to:
  /// **'Every launchable app gets a tile: icon, label and a backdrop tinted from the icon\'s own dominant colour. New installs and removals show up live, no refresh needed.'**
  String get featGridBody;

  /// No description provided for @featFindTitle.
  ///
  /// In en, this message translates to:
  /// **'Find'**
  String get featFindTitle;

  /// No description provided for @featSearchTitle.
  ///
  /// In en, this message translates to:
  /// **'Search that forgives'**
  String get featSearchTitle;

  /// No description provided for @featSearchBody.
  ///
  /// In en, this message translates to:
  /// **'Matching ignores case and accents, so typing the way you remember a name is enough to find it.'**
  String get featSearchBody;

  /// No description provided for @featCollectionsTitle.
  ///
  /// In en, this message translates to:
  /// **'Collections with counts'**
  String get featCollectionsTitle;

  /// No description provided for @featCollectionsBody.
  ///
  /// In en, this message translates to:
  /// **'All, Pinned, Apps and System are always there. Groups you make join the same row, each with a live count.'**
  String get featCollectionsBody;

  /// No description provided for @featSortTitle.
  ///
  /// In en, this message translates to:
  /// **'Four sort orders'**
  String get featSortTitle;

  /// No description provided for @featSortBody.
  ///
  /// In en, this message translates to:
  /// **'A-Z, Z-A, recently installed and recently updated - or a custom order once you start dragging tiles.'**
  String get featSortBody;

  /// No description provided for @featArrangeTitle.
  ///
  /// In en, this message translates to:
  /// **'Arrange'**
  String get featArrangeTitle;

  /// No description provided for @featPinTitle.
  ///
  /// In en, this message translates to:
  /// **'Pin to the top'**
  String get featPinTitle;

  /// No description provided for @featPinBody.
  ///
  /// In en, this message translates to:
  /// **'Pinned apps lead the grid in every collection. Long-press-drag any tile to rearrange; the order survives restarts.'**
  String get featPinBody;

  /// No description provided for @featGroupsTitle.
  ///
  /// In en, this message translates to:
  /// **'Groups'**
  String get featGroupsTitle;

  /// No description provided for @featGroupsBody.
  ///
  /// In en, this message translates to:
  /// **'Create, rename and delete groups, then file apps into them from the tile menu. A group is just another collection.'**
  String get featGroupsBody;

  /// No description provided for @featControlTitle.
  ///
  /// In en, this message translates to:
  /// **'Control'**
  String get featControlTitle;

  /// No description provided for @featMenuTitle.
  ///
  /// In en, this message translates to:
  /// **'A menu on every tile'**
  String get featMenuTitle;

  /// No description provided for @featMenuBody.
  ///
  /// In en, this message translates to:
  /// **'Open, pin, add to group, app details and uninstall for user apps - one long press or menu key away.'**
  String get featMenuBody;

  /// No description provided for @featInstallTitle.
  ///
  /// In en, this message translates to:
  /// **'Install APKs in place'**
  String get featInstallTitle;

  /// No description provided for @featInstallBody.
  ///
  /// In en, this message translates to:
  /// **'The system file picker handles installs, so sideloading never leaves the window.'**
  String get featInstallBody;

  /// No description provided for @featNavigateTitle.
  ///
  /// In en, this message translates to:
  /// **'Navigate'**
  String get featNavigateTitle;

  /// No description provided for @featDpadTitle.
  ///
  /// In en, this message translates to:
  /// **'D-pad native'**
  String get featDpadTitle;

  /// No description provided for @featDpadBody.
  ///
  /// In en, this message translates to:
  /// **'Arrows move focus, confirm launches the focused app, and the menu key opens its tile menu. Focus wraps the way a TV launcher does.'**
  String get featDpadBody;

  /// No description provided for @featLiveTitle.
  ///
  /// In en, this message translates to:
  /// **'Live package updates'**
  String get featLiveTitle;

  /// No description provided for @featLiveBody.
  ///
  /// In en, this message translates to:
  /// **'Install or remove an app anywhere on the system and the grid updates while you watch.'**
  String get featLiveBody;

  /// No description provided for @featI18nTitle.
  ///
  /// In en, this message translates to:
  /// **'Every string external'**
  String get featI18nTitle;

  /// No description provided for @featI18nBody.
  ///
  /// In en, this message translates to:
  /// **'All UI text lives in ARB files, so translating the app means editing one file, not hunting through code.'**
  String get featI18nBody;

  /// No description provided for @screenshotsTitle.
  ///
  /// In en, this message translates to:
  /// **'Screenshots'**
  String get screenshotsTitle;

  /// No description provided for @screenshotsSubtitle.
  ///
  /// In en, this message translates to:
  /// **'The real app, running on the headset.'**
  String get screenshotsSubtitle;

  /// No description provided for @shotGridCaption.
  ///
  /// In en, this message translates to:
  /// **'The app grid inside vrhome.'**
  String get shotGridCaption;

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

  /// No description provided for @downloadTitle.
  ///
  /// In en, this message translates to:
  /// **'Download'**
  String get downloadTitle;

  /// No description provided for @downloadSubtitle.
  ///
  /// In en, this message translates to:
  /// **'Nothing to install yet.'**
  String get downloadSubtitle;

  /// No description provided for @downloadStatusTitle.
  ///
  /// In en, this message translates to:
  /// **'Not shipping yet'**
  String get downloadStatusTitle;

  /// No description provided for @downloadStatusBody.
  ///
  /// In en, this message translates to:
  /// **'Library ships with the PN2Lineage system image, and the image is still being built. When a flashable build exists it will land here first.'**
  String get downloadStatusBody;

  /// No description provided for @downloadStepsTitle.
  ///
  /// In en, this message translates to:
  /// **'Want it early?'**
  String get downloadStepsTitle;

  /// No description provided for @downloadStepSource.
  ///
  /// In en, this message translates to:
  /// **'Clone the repository and build the app yourself with the Flutter toolchain.'**
  String get downloadStepSource;

  /// No description provided for @downloadStepDocs.
  ///
  /// In en, this message translates to:
  /// **'Read the docs for the port\'s build pipeline and current state.'**
  String get downloadStepDocs;

  /// No description provided for @downloadStepWatch.
  ///
  /// In en, this message translates to:
  /// **'Watch the repository to get release news.'**
  String get downloadStepWatch;

  /// No description provided for @downloadSourceCta.
  ///
  /// In en, this message translates to:
  /// **'View source on GitLab'**
  String get downloadSourceCta;

  /// No description provided for @downloadDocsCta.
  ///
  /// In en, this message translates to:
  /// **'Open the docs'**
  String get downloadDocsCta;

  /// No description provided for @downloadNote.
  ///
  /// In en, this message translates to:
  /// **'Builds are signed for the headset\'s system image. Sideloaded copies work, but the library is meant to live inside vrhome.'**
  String get downloadNote;

  /// No description provided for @aboutTitle.
  ///
  /// In en, this message translates to:
  /// **'About'**
  String get aboutTitle;

  /// No description provided for @aboutSubtitle.
  ///
  /// In en, this message translates to:
  /// **'One small piece of a larger port.'**
  String get aboutSubtitle;

  /// No description provided for @aboutWhatTitle.
  ///
  /// In en, this message translates to:
  /// **'What it is'**
  String get aboutWhatTitle;

  /// No description provided for @aboutWhatBody.
  ///
  /// In en, this message translates to:
  /// **'Library is the app window of vrhome, the VR home environment for the Pico Neo 2 running LineageOS 17.1. It lists every launchable app on the headset and gets out of the way.'**
  String get aboutWhatBody;

  /// No description provided for @aboutProjectTitle.
  ///
  /// In en, this message translates to:
  /// **'The project'**
  String get aboutProjectTitle;

  /// No description provided for @aboutProjectBody.
  ///
  /// In en, this message translates to:
  /// **'PN2Lineage ports LineageOS 17.1 - Android 10 via a phh GSI - to the Pico Neo 2. Every piece of the port, from the GSI overlay to the VR stack, lives in its own repository under neosalsa on GitLab.'**
  String get aboutProjectBody;

  /// No description provided for @aboutTechTitle.
  ///
  /// In en, this message translates to:
  /// **'How it is built'**
  String get aboutTechTitle;

  /// No description provided for @aboutTechBody.
  ///
  /// In en, this message translates to:
  /// **'Flutter, with all decisions in pure Dart modules and platform glue kept thin. The Kotlin side only talks to PackageManager, rasterizes icons and fires intents.'**
  String get aboutTechBody;

  /// No description provided for @aboutLicenseTitle.
  ///
  /// In en, this message translates to:
  /// **'License'**
  String get aboutLicenseTitle;

  /// No description provided for @aboutLicenseBody.
  ///
  /// In en, this message translates to:
  /// **'Library is free software under the GNU Affero General Public License v3. The Inter typeface bundled on this site is under the SIL Open Font License 1.1.'**
  String get aboutLicenseBody;

  /// No description provided for @aboutRepoCta.
  ///
  /// In en, this message translates to:
  /// **'Library repository'**
  String get aboutRepoCta;

  /// No description provided for @aboutDocsCta.
  ///
  /// In en, this message translates to:
  /// **'Project docs'**
  String get aboutDocsCta;

  /// No description provided for @aboutGroupCta.
  ///
  /// In en, this message translates to:
  /// **'All neosalsa repos'**
  String get aboutGroupCta;

  /// No description provided for @footerProduct.
  ///
  /// In en, this message translates to:
  /// **'Library'**
  String get footerProduct;

  /// No description provided for @footerProject.
  ///
  /// In en, this message translates to:
  /// **'Project'**
  String get footerProject;

  /// No description provided for @footerCopyright.
  ///
  /// In en, this message translates to:
  /// **'Copyright {year} neosalsa'**
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
  /// **'Is there a build I can install?'**
  String get faqQ1;

  /// No description provided for @faqA1.
  ///
  /// In en, this message translates to:
  /// **'Not yet. Library ships inside the PN2Lineage system image, which is still being assembled. The download page tracks the current state.'**
  String get faqA1;

  /// No description provided for @faqQ2.
  ///
  /// In en, this message translates to:
  /// **'How do I try it today?'**
  String get faqQ2;

  /// No description provided for @faqA2.
  ///
  /// In en, this message translates to:
  /// **'Clone the repository and build it with the Flutter toolchain, or watch the repo for the first release.'**
  String get faqA2;

  /// No description provided for @faqQ3.
  ///
  /// In en, this message translates to:
  /// **'What is vrhome?'**
  String get faqQ3;

  /// No description provided for @faqA3.
  ///
  /// In en, this message translates to:
  /// **'The VR home environment Library lives inside. vrhome draws the window frames, the keyboard and the rest of the desktop around app windows like this one.'**
  String get faqA3;

  /// No description provided for @faqQ4.
  ///
  /// In en, this message translates to:
  /// **'Does it work without the headset?'**
  String get faqQ4;

  /// No description provided for @faqA4.
  ///
  /// In en, this message translates to:
  /// **'It is a normal Flutter app and runs anywhere Android does. The controller shortcuts and the window layout only really make sense inside vrhome on the Pico Neo 2.'**
  String get faqA4;

  /// No description provided for @faqQ5.
  ///
  /// In en, this message translates to:
  /// **'Can I sideload the APK?'**
  String get faqQ5;

  /// No description provided for @faqA5.
  ///
  /// In en, this message translates to:
  /// **'Yes, it installs like any other APK. What it cannot be alone is the home environment - that part belongs to the system image.'**
  String get faqA5;

  /// No description provided for @faqQ6.
  ///
  /// In en, this message translates to:
  /// **'Why AGPL?'**
  String get faqQ6;

  /// No description provided for @faqA6.
  ///
  /// In en, this message translates to:
  /// **'The whole port is free software. If you ship a modified Library over a network or on a device, your users get the source too.'**
  String get faqA6;

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
  /// **'Back to Library'**
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
