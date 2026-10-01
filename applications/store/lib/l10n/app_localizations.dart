import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:intl/intl.dart' as intl;

import 'app_localizations_en.dart';

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
  static const List<Locale> supportedLocales = <Locale>[Locale('en')];

  /// No description provided for @appTitle.
  ///
  /// In en, this message translates to:
  /// **'Store'**
  String get appTitle;

  /// No description provided for @searchHint.
  ///
  /// In en, this message translates to:
  /// **'Search apps'**
  String get searchHint;

  /// No description provided for @categoryAll.
  ///
  /// In en, this message translates to:
  /// **'All'**
  String get categoryAll;

  /// No description provided for @sortUpdated.
  ///
  /// In en, this message translates to:
  /// **'Recently updated'**
  String get sortUpdated;

  /// No description provided for @sortName.
  ///
  /// In en, this message translates to:
  /// **'Name A-Z'**
  String get sortName;

  /// No description provided for @appsCount.
  ///
  /// In en, this message translates to:
  /// **'{count, plural, =1{1 app} other{{count} apps}}'**
  String appsCount(int count);

  /// No description provided for @install.
  ///
  /// In en, this message translates to:
  /// **'Install'**
  String get install;

  /// No description provided for @installing.
  ///
  /// In en, this message translates to:
  /// **'Installing'**
  String get installing;

  /// No description provided for @downloading.
  ///
  /// In en, this message translates to:
  /// **'Downloading'**
  String get downloading;

  /// No description provided for @open.
  ///
  /// In en, this message translates to:
  /// **'Open'**
  String get open;

  /// No description provided for @retry.
  ///
  /// In en, this message translates to:
  /// **'Retry'**
  String get retry;

  /// No description provided for @installedBadge.
  ///
  /// In en, this message translates to:
  /// **'Installed'**
  String get installedBadge;

  /// No description provided for @installFailed.
  ///
  /// In en, this message translates to:
  /// **'Install failed'**
  String get installFailed;

  /// No description provided for @installPrompted.
  ///
  /// In en, this message translates to:
  /// **'Confirm the install in the system dialog'**
  String get installPrompted;

  /// No description provided for @versionRow.
  ///
  /// In en, this message translates to:
  /// **'{version} ({code})'**
  String versionRow(String version, int code);

  /// No description provided for @loadErrorTitle.
  ///
  /// In en, this message translates to:
  /// **'Couldn\'t load the catalog'**
  String get loadErrorTitle;

  /// No description provided for @loadErrorBody.
  ///
  /// In en, this message translates to:
  /// **'Check the repository address and the network connection.'**
  String get loadErrorBody;

  /// No description provided for @emptyResults.
  ///
  /// In en, this message translates to:
  /// **'No apps match your search'**
  String get emptyResults;

  /// No description provided for @selectAppPrompt.
  ///
  /// In en, this message translates to:
  /// **'Pick an app to see details'**
  String get selectAppPrompt;

  /// No description provided for @repoMenu.
  ///
  /// In en, this message translates to:
  /// **'Repository'**
  String get repoMenu;

  /// No description provided for @repoDialogTitle.
  ///
  /// In en, this message translates to:
  /// **'Repository'**
  String get repoDialogTitle;

  /// No description provided for @repoDialogBody.
  ///
  /// In en, this message translates to:
  /// **'Catalog and packages come from this F-Droid compatible repository.'**
  String get repoDialogBody;

  /// No description provided for @repoUrlLabel.
  ///
  /// In en, this message translates to:
  /// **'Repository URL'**
  String get repoUrlLabel;

  /// No description provided for @repoUrlInvalid.
  ///
  /// In en, this message translates to:
  /// **'Enter an http or https URL'**
  String get repoUrlInvalid;

  /// No description provided for @repoReset.
  ///
  /// In en, this message translates to:
  /// **'Reset to default'**
  String get repoReset;

  /// No description provided for @cancel.
  ///
  /// In en, this message translates to:
  /// **'Cancel'**
  String get cancel;

  /// No description provided for @save.
  ///
  /// In en, this message translates to:
  /// **'Save'**
  String get save;

  /// No description provided for @sectionDescription.
  ///
  /// In en, this message translates to:
  /// **'About'**
  String get sectionDescription;

  /// No description provided for @sectionScreenshots.
  ///
  /// In en, this message translates to:
  /// **'Screenshots'**
  String get sectionScreenshots;

  /// No description provided for @sectionVersions.
  ///
  /// In en, this message translates to:
  /// **'Versions'**
  String get sectionVersions;

  /// No description provided for @metaVersion.
  ///
  /// In en, this message translates to:
  /// **'Version'**
  String get metaVersion;

  /// No description provided for @metaSize.
  ///
  /// In en, this message translates to:
  /// **'Size'**
  String get metaSize;

  /// No description provided for @metaLicense.
  ///
  /// In en, this message translates to:
  /// **'License'**
  String get metaLicense;

  /// No description provided for @metaUpdated.
  ///
  /// In en, this message translates to:
  /// **'Updated'**
  String get metaUpdated;

  /// No description provided for @metaMinSdk.
  ///
  /// In en, this message translates to:
  /// **'Min SDK'**
  String get metaMinSdk;

  /// No description provided for @byAuthor.
  ///
  /// In en, this message translates to:
  /// **'by {author}'**
  String byAuthor(String author);
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
      <String>['en'].contains(locale.languageCode);

  @override
  bool shouldReload(_AppLocalizationsDelegate old) => false;
}

AppLocalizations lookupAppLocalizations(Locale locale) {
  // Lookup logic when only language code is specified.
  switch (locale.languageCode) {
    case 'en':
      return AppLocalizationsEn();
  }

  throw FlutterError(
    'AppLocalizations.delegate failed to load unsupported locale "$locale". This is likely '
    'an issue with the localizations generation tool. Please file an issue '
    'on GitHub with a reproducible sample app and the gen-l10n configuration '
    'that was used.',
  );
}
