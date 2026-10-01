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

  /// No description provided for @appTitle.
  ///
  /// In en, this message translates to:
  /// **'HBSUP'**
  String get appTitle;

  /// No description provided for @appSubtitle.
  ///
  /// In en, this message translates to:
  /// **'Hibiscus Backup'**
  String get appSubtitle;

  /// No description provided for @unsupportedHostTitle.
  ///
  /// In en, this message translates to:
  /// **'Unsupported host OS'**
  String get unsupportedHostTitle;

  /// No description provided for @unsupportedHostBody.
  ///
  /// In en, this message translates to:
  /// **'Windows is not a supported host OS for HBSUP. Things may not work - use Linux or macOS if you run into problems.'**
  String get unsupportedHostBody;

  /// No description provided for @unsupportedHostDismiss.
  ///
  /// In en, this message translates to:
  /// **'Dismiss'**
  String get unsupportedHostDismiss;

  /// No description provided for @connectTitle.
  ///
  /// In en, this message translates to:
  /// **'Connect a headset'**
  String get connectTitle;

  /// No description provided for @connectHint.
  ///
  /// In en, this message translates to:
  /// **'HBSUP needs rooted adb. Boot the stock system, run adb root, then plug the headset in.'**
  String get connectHint;

  /// No description provided for @connectRefresh.
  ///
  /// In en, this message translates to:
  /// **'Refresh'**
  String get connectRefresh;

  /// No description provided for @connectScanning.
  ///
  /// In en, this message translates to:
  /// **'Scanning for devices...'**
  String get connectScanning;

  /// No description provided for @connectNoDevices.
  ///
  /// In en, this message translates to:
  /// **'No adb devices. Plug a headset in, or bring wireless adb up first.'**
  String get connectNoDevices;

  /// No description provided for @connectUnknown.
  ///
  /// In en, this message translates to:
  /// **'Unknown model'**
  String get connectUnknown;

  /// No description provided for @connectButton.
  ///
  /// In en, this message translates to:
  /// **'Connect'**
  String get connectButton;

  /// No description provided for @disconnect.
  ///
  /// In en, this message translates to:
  /// **'Disconnect'**
  String get disconnect;

  /// No description provided for @backupDestTitle.
  ///
  /// In en, this message translates to:
  /// **'Backup folder'**
  String get backupDestTitle;

  /// No description provided for @backupDestPick.
  ///
  /// In en, this message translates to:
  /// **'Choose folder'**
  String get backupDestPick;

  /// No description provided for @backupDestNone.
  ///
  /// In en, this message translates to:
  /// **'No folder picked yet'**
  String get backupDestNone;

  /// No description provided for @backupFreeSpace.
  ///
  /// In en, this message translates to:
  /// **'{free} free'**
  String backupFreeSpace(String free);

  /// No description provided for @backupFreeUnknown.
  ///
  /// In en, this message translates to:
  /// **'Free space unknown'**
  String get backupFreeUnknown;

  /// No description provided for @backupPartsTitle.
  ///
  /// In en, this message translates to:
  /// **'Partitions'**
  String get backupPartsTitle;

  /// No description provided for @backupPartsLoading.
  ///
  /// In en, this message translates to:
  /// **'Reading the partition table...'**
  String get backupPartsLoading;

  /// No description provided for @backupPartsEmpty.
  ///
  /// In en, this message translates to:
  /// **'No partitions found. Is adb rooted on the headset?'**
  String get backupPartsEmpty;

  /// No description provided for @backupTotalSize.
  ///
  /// In en, this message translates to:
  /// **'Full dump: {size}'**
  String backupTotalSize(String size);

  /// No description provided for @backupSpaceOk.
  ///
  /// In en, this message translates to:
  /// **'Enough free space for the full dump'**
  String get backupSpaceOk;

  /// No description provided for @backupSpaceShort.
  ///
  /// In en, this message translates to:
  /// **'Not enough space - the dump needs {needed}, only {free} free'**
  String backupSpaceShort(String needed, String free);

  /// No description provided for @backupStart.
  ///
  /// In en, this message translates to:
  /// **'Start backup'**
  String get backupStart;

  /// No description provided for @backupRunning.
  ///
  /// In en, this message translates to:
  /// **'Backing up...'**
  String get backupRunning;

  /// No description provided for @backupCancel.
  ///
  /// In en, this message translates to:
  /// **'Cancel'**
  String get backupCancel;

  /// No description provided for @backupDone.
  ///
  /// In en, this message translates to:
  /// **'Backup finished. Keep this folder somewhere safe - it is your way back to stock.'**
  String get backupDone;

  /// No description provided for @backupFailed.
  ///
  /// In en, this message translates to:
  /// **'Backup failed: {error}'**
  String backupFailed(String error);

  /// No description provided for @backupCancelled.
  ///
  /// In en, this message translates to:
  /// **'Backup cancelled'**
  String get backupCancelled;

  /// No description provided for @backupProgress.
  ///
  /// In en, this message translates to:
  /// **'{done} of {total} partitions'**
  String backupProgress(int done, int total);

  /// No description provided for @backupLogTitle.
  ///
  /// In en, this message translates to:
  /// **'Log'**
  String get backupLogTitle;
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
