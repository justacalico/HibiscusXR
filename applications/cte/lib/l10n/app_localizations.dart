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
  /// **'HCTE'**
  String get appTitle;

  /// No description provided for @appSubtitle.
  ///
  /// In en, this message translates to:
  /// **'Hibiscus Controller Testing Environment'**
  String get appSubtitle;

  /// No description provided for @navOverview.
  ///
  /// In en, this message translates to:
  /// **'Overview'**
  String get navOverview;

  /// No description provided for @navDisplay.
  ///
  /// In en, this message translates to:
  /// **'Display'**
  String get navDisplay;

  /// No description provided for @navInstall.
  ///
  /// In en, this message translates to:
  /// **'Install'**
  String get navInstall;

  /// No description provided for @navTracking.
  ///
  /// In en, this message translates to:
  /// **'Tracking'**
  String get navTracking;

  /// No description provided for @navDebug.
  ///
  /// In en, this message translates to:
  /// **'Debug'**
  String get navDebug;

  /// No description provided for @connectTitle.
  ///
  /// In en, this message translates to:
  /// **'Connect to a headset'**
  String get connectTitle;

  /// No description provided for @connectUsbSection.
  ///
  /// In en, this message translates to:
  /// **'USB / adb devices'**
  String get connectUsbSection;

  /// No description provided for @connectWirelessSection.
  ///
  /// In en, this message translates to:
  /// **'Wireless'**
  String get connectWirelessSection;

  /// No description provided for @connectWirelessHint.
  ///
  /// In en, this message translates to:
  /// **'Headset IP address'**
  String get connectWirelessHint;

  /// No description provided for @connectWirelessButton.
  ///
  /// In en, this message translates to:
  /// **'Connect'**
  String get connectWirelessButton;

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

  /// No description provided for @connectConnecting.
  ///
  /// In en, this message translates to:
  /// **'Connecting...'**
  String get connectConnecting;

  /// No description provided for @connectNoDevices.
  ///
  /// In en, this message translates to:
  /// **'No adb devices. Plug a headset in, or connect over Wi-Fi.'**
  String get connectNoDevices;

  /// No description provided for @connectWirelessNote.
  ///
  /// In en, this message translates to:
  /// **'Tries the on-device CTE service first, then wireless adb.'**
  String get connectWirelessNote;

  /// No description provided for @disconnect.
  ///
  /// In en, this message translates to:
  /// **'Disconnect'**
  String get disconnect;

  /// No description provided for @overviewHeadset.
  ///
  /// In en, this message translates to:
  /// **'Headset'**
  String get overviewHeadset;

  /// No description provided for @overviewControllers.
  ///
  /// In en, this message translates to:
  /// **'Controllers'**
  String get overviewControllers;

  /// No description provided for @overviewLink.
  ///
  /// In en, this message translates to:
  /// **'Link'**
  String get overviewLink;

  /// No description provided for @overviewBattery.
  ///
  /// In en, this message translates to:
  /// **'Battery'**
  String get overviewBattery;

  /// No description provided for @overviewTrackingMode.
  ///
  /// In en, this message translates to:
  /// **'Tracking'**
  String get overviewTrackingMode;

  /// No description provided for @overviewAndroid.
  ///
  /// In en, this message translates to:
  /// **'Android'**
  String get overviewAndroid;

  /// No description provided for @overviewBuild.
  ///
  /// In en, this message translates to:
  /// **'Build'**
  String get overviewBuild;

  /// No description provided for @overviewSerial.
  ///
  /// In en, this message translates to:
  /// **'Serial'**
  String get overviewSerial;

  /// No description provided for @overviewAddress.
  ///
  /// In en, this message translates to:
  /// **'Address'**
  String get overviewAddress;

  /// No description provided for @overviewHibiscus.
  ///
  /// In en, this message translates to:
  /// **'Hibiscus'**
  String get overviewHibiscus;

  /// No description provided for @ctrlLeft.
  ///
  /// In en, this message translates to:
  /// **'Left controller'**
  String get ctrlLeft;

  /// No description provided for @ctrlRight.
  ///
  /// In en, this message translates to:
  /// **'Right controller'**
  String get ctrlRight;

  /// No description provided for @ctrlConnected.
  ///
  /// In en, this message translates to:
  /// **'connected'**
  String get ctrlConnected;

  /// No description provided for @ctrlAbsent.
  ///
  /// In en, this message translates to:
  /// **'not connected'**
  String get ctrlAbsent;

  /// No description provided for @ctrlBattery.
  ///
  /// In en, this message translates to:
  /// **'{pct}%'**
  String ctrlBattery(Object pct);

  /// No description provided for @ctrlTracked.
  ///
  /// In en, this message translates to:
  /// **'tracked'**
  String get ctrlTracked;

  /// No description provided for @ctrlUntracked.
  ///
  /// In en, this message translates to:
  /// **'no tracking'**
  String get ctrlUntracked;

  /// No description provided for @displayMirror.
  ///
  /// In en, this message translates to:
  /// **'Mirror'**
  String get displayMirror;

  /// No description provided for @displayStart.
  ///
  /// In en, this message translates to:
  /// **'Start mirroring'**
  String get displayStart;

  /// No description provided for @displayStop.
  ///
  /// In en, this message translates to:
  /// **'Stop'**
  String get displayStop;

  /// No description provided for @displayHint.
  ///
  /// In en, this message translates to:
  /// **'Screen frames stream at a few frames per second - enough for debugging, not for play.'**
  String get displayHint;

  /// No description provided for @displayNoFrame.
  ///
  /// In en, this message translates to:
  /// **'No frames yet'**
  String get displayNoFrame;

  /// No description provided for @installTitle.
  ///
  /// In en, this message translates to:
  /// **'Install an APK'**
  String get installTitle;

  /// No description provided for @installPick.
  ///
  /// In en, this message translates to:
  /// **'Choose APK'**
  String get installPick;

  /// No description provided for @installRun.
  ///
  /// In en, this message translates to:
  /// **'Install'**
  String get installRun;

  /// No description provided for @installRunning.
  ///
  /// In en, this message translates to:
  /// **'Installing...'**
  String get installRunning;

  /// No description provided for @installDone.
  ///
  /// In en, this message translates to:
  /// **'Finished'**
  String get installDone;

  /// No description provided for @installDrop.
  ///
  /// In en, this message translates to:
  /// **'Pick an .apk to push to the headset.'**
  String get installDrop;

  /// No description provided for @trackingTitle.
  ///
  /// In en, this message translates to:
  /// **'Tracking'**
  String get trackingTitle;

  /// No description provided for @trackingHead.
  ///
  /// In en, this message translates to:
  /// **'Head pose'**
  String get trackingHead;

  /// No description provided for @trackingRate.
  ///
  /// In en, this message translates to:
  /// **'{rate} samples/s'**
  String trackingRate(Object rate);

  /// No description provided for @trackingMode.
  ///
  /// In en, this message translates to:
  /// **'{mode, select, dof6{6DoF} dof3{3DoF} other{unknown}}'**
  String trackingMode(String mode);

  /// No description provided for @trackingIdle.
  ///
  /// In en, this message translates to:
  /// **'No pose data - poses only flow while a VR app is running on the headset.'**
  String get trackingIdle;

  /// No description provided for @trackingPosition.
  ///
  /// In en, this message translates to:
  /// **'Position'**
  String get trackingPosition;

  /// No description provided for @trackingOrientation.
  ///
  /// In en, this message translates to:
  /// **'Orientation'**
  String get trackingOrientation;

  /// No description provided for @trackingLog.
  ///
  /// In en, this message translates to:
  /// **'Pose log'**
  String get trackingLog;

  /// No description provided for @debugTitle.
  ///
  /// In en, this message translates to:
  /// **'Debug'**
  String get debugTitle;

  /// No description provided for @debugProps.
  ///
  /// In en, this message translates to:
  /// **'Properties'**
  String get debugProps;

  /// No description provided for @debugFilter.
  ///
  /// In en, this message translates to:
  /// **'Filter'**
  String get debugFilter;

  /// No description provided for @debugLogcat.
  ///
  /// In en, this message translates to:
  /// **'Logcat'**
  String get debugLogcat;

  /// No description provided for @debugEmpty.
  ///
  /// In en, this message translates to:
  /// **'Connect a headset to read its state.'**
  String get debugEmpty;

  /// No description provided for @valueUnknown.
  ///
  /// In en, this message translates to:
  /// **'unknown'**
  String get valueUnknown;
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
