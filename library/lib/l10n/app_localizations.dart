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

  /// Title shown in the library window title bar
  ///
  /// In en, this message translates to:
  /// **'App Library'**
  String get appTitle;

  /// Placeholder inside the search field
  ///
  /// In en, this message translates to:
  /// **'Search'**
  String get searchHint;

  /// Tooltip for the button that clears the search field
  ///
  /// In en, this message translates to:
  /// **'Clear search'**
  String get clearSearch;

  /// Collection filter showing every app
  ///
  /// In en, this message translates to:
  /// **'All'**
  String get collectionAll;

  /// Collection filter label with the number of apps
  ///
  /// In en, this message translates to:
  /// **'All ({count})'**
  String collectionAllCount(int count);

  /// Collection or group label with the number of apps
  ///
  /// In en, this message translates to:
  /// **'{label} ({count})'**
  String collectionCount(String label, int count);

  /// Collection filter showing only pinned apps
  ///
  /// In en, this message translates to:
  /// **'Pinned'**
  String get collectionPinned;

  /// Collection filter showing user-installed apps
  ///
  /// In en, this message translates to:
  /// **'Apps'**
  String get collectionUserApps;

  /// Collection filter showing system apps
  ///
  /// In en, this message translates to:
  /// **'System'**
  String get collectionSystemApps;

  /// Section header above user groups in the collection menu
  ///
  /// In en, this message translates to:
  /// **'Groups'**
  String get collectionGroups;

  /// Sort option for the manually arranged order
  ///
  /// In en, this message translates to:
  /// **'Custom order'**
  String get sortCustom;

  /// Sort option, alphabetical ascending
  ///
  /// In en, this message translates to:
  /// **'A-Z'**
  String get sortNameAsc;

  /// Sort option, alphabetical descending
  ///
  /// In en, this message translates to:
  /// **'Z-A'**
  String get sortNameDesc;

  /// Sort option, newest installs first
  ///
  /// In en, this message translates to:
  /// **'Recently installed'**
  String get sortNewest;

  /// Sort option, newest updates first
  ///
  /// In en, this message translates to:
  /// **'Recently updated'**
  String get sortUpdated;

  /// Context menu action that launches the app
  ///
  /// In en, this message translates to:
  /// **'Open'**
  String get openApp;

  /// Context menu action that pins the app to the top
  ///
  /// In en, this message translates to:
  /// **'Pin'**
  String get pinApp;

  /// Context menu action that removes the pin
  ///
  /// In en, this message translates to:
  /// **'Unpin'**
  String get unpinApp;

  /// Context menu action that uninstalls the app
  ///
  /// In en, this message translates to:
  /// **'Uninstall'**
  String get uninstallApp;

  /// Context menu action that opens the system app info page
  ///
  /// In en, this message translates to:
  /// **'App info'**
  String get appInfo;

  /// Context menu action that adds the app to a group
  ///
  /// In en, this message translates to:
  /// **'Add to group'**
  String get addToGroup;

  /// Context menu action that removes the app from the current group
  ///
  /// In en, this message translates to:
  /// **'Remove from group'**
  String get removeFromGroup;

  /// Button and menu item that creates a group
  ///
  /// In en, this message translates to:
  /// **'New group'**
  String get newGroup;

  /// Menu item that renames the active group
  ///
  /// In en, this message translates to:
  /// **'Rename group'**
  String get renameGroup;

  /// Menu item that deletes the active group
  ///
  /// In en, this message translates to:
  /// **'Delete group'**
  String get deleteGroup;

  /// Confirmation text when deleting a group
  ///
  /// In en, this message translates to:
  /// **'Delete \"{name}\"? The apps stay installed.'**
  String deleteGroupConfirm(String name);

  /// Placeholder inside the group name text field
  ///
  /// In en, this message translates to:
  /// **'Group name'**
  String get groupNameHint;

  /// Shown in the group picker when no group exists
  ///
  /// In en, this message translates to:
  /// **'No groups yet'**
  String get noGroupsYet;

  /// Action that installs an APK file picked by the user
  ///
  /// In en, this message translates to:
  /// **'Install app'**
  String get installApp;

  /// Shown when the device reports no launchable apps
  ///
  /// In en, this message translates to:
  /// **'No apps installed'**
  String get emptyLibrary;

  /// Shown when the search query matches nothing
  ///
  /// In en, this message translates to:
  /// **'No results for \"{query}\"'**
  String emptySearch(String query);

  /// Shown when the active group has no members
  ///
  /// In en, this message translates to:
  /// **'No apps in this group'**
  String get emptyGroup;

  /// Shown while the app list is being fetched
  ///
  /// In en, this message translates to:
  /// **'Loading apps…'**
  String get loadingApps;

  /// Shown when the platform bridge fails to list apps
  ///
  /// In en, this message translates to:
  /// **'Couldn\'t load apps'**
  String get loadFailed;

  /// Snackbar when launching an app fails
  ///
  /// In en, this message translates to:
  /// **'Couldn\'t open {app}'**
  String launchFailed(String app);

  /// Snackbar when uninstalling an app fails
  ///
  /// In en, this message translates to:
  /// **'Couldn\'t uninstall {app}'**
  String uninstallFailed(String app);

  /// Button that retries loading the app list
  ///
  /// In en, this message translates to:
  /// **'Retry'**
  String get retry;

  /// Generic cancel button
  ///
  /// In en, this message translates to:
  /// **'Cancel'**
  String get cancel;

  /// Generic create button
  ///
  /// In en, this message translates to:
  /// **'Create'**
  String get create;

  /// Generic save button
  ///
  /// In en, this message translates to:
  /// **'Save'**
  String get save;

  /// Generic delete button
  ///
  /// In en, this message translates to:
  /// **'Delete'**
  String get delete;

  /// App count label in the title area
  ///
  /// In en, this message translates to:
  /// **'{count, plural, =0{No apps} =1{1 app} other{{count} apps}}'**
  String appCount(int count);

  /// Badge on tiles for preinstalled system apps
  ///
  /// In en, this message translates to:
  /// **'System'**
  String get systemBadge;

  /// Context menu action opening the app details dialog
  ///
  /// In en, this message translates to:
  /// **'Details'**
  String get details;

  /// Title of the app details dialog
  ///
  /// In en, this message translates to:
  /// **'About {app}'**
  String detailsTitle(String app);

  /// Label for the package name row in the details dialog
  ///
  /// In en, this message translates to:
  /// **'Package'**
  String get detailsPackage;

  /// Label for the version row in the details dialog
  ///
  /// In en, this message translates to:
  /// **'Version'**
  String get detailsVersion;

  /// Label for the install date row in the details dialog
  ///
  /// In en, this message translates to:
  /// **'Installed'**
  String get detailsInstalled;

  /// Label for the update date row in the details dialog
  ///
  /// In en, this message translates to:
  /// **'Updated'**
  String get detailsUpdated;

  /// Shown for a missing value in the details dialog
  ///
  /// In en, this message translates to:
  /// **'—'**
  String get notAvailable;
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
