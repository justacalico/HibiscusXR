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
  /// **'Settings'**
  String get appTitle;

  /// No description provided for @sectionWifi.
  ///
  /// In en, this message translates to:
  /// **'Wi-Fi'**
  String get sectionWifi;

  /// No description provided for @sectionBluetooth.
  ///
  /// In en, this message translates to:
  /// **'Bluetooth'**
  String get sectionBluetooth;

  /// No description provided for @sectionControllers.
  ///
  /// In en, this message translates to:
  /// **'Controllers'**
  String get sectionControllers;

  /// No description provided for @sectionDisplay.
  ///
  /// In en, this message translates to:
  /// **'Display'**
  String get sectionDisplay;

  /// No description provided for @sectionSound.
  ///
  /// In en, this message translates to:
  /// **'Sound'**
  String get sectionSound;

  /// No description provided for @sectionCamera.
  ///
  /// In en, this message translates to:
  /// **'Camera'**
  String get sectionCamera;

  /// No description provided for @sectionLanguage.
  ///
  /// In en, this message translates to:
  /// **'Language and Region'**
  String get sectionLanguage;

  /// No description provided for @sectionTime.
  ///
  /// In en, this message translates to:
  /// **'Time'**
  String get sectionTime;

  /// No description provided for @sectionKeyboard.
  ///
  /// In en, this message translates to:
  /// **'Keyboard'**
  String get sectionKeyboard;

  /// No description provided for @sectionHeadsetTracking.
  ///
  /// In en, this message translates to:
  /// **'Headset Tracking'**
  String get sectionHeadsetTracking;

  /// No description provided for @sectionBackup.
  ///
  /// In en, this message translates to:
  /// **'Backup'**
  String get sectionBackup;

  /// No description provided for @sectionDeveloper.
  ///
  /// In en, this message translates to:
  /// **'Developer'**
  String get sectionDeveloper;

  /// No description provided for @sectionSoftwareUpdate.
  ///
  /// In en, this message translates to:
  /// **'Software Update'**
  String get sectionSoftwareUpdate;

  /// No description provided for @sectionAbout.
  ///
  /// In en, this message translates to:
  /// **'About'**
  String get sectionAbout;

  /// No description provided for @sectionTips.
  ///
  /// In en, this message translates to:
  /// **'Tips and Support'**
  String get sectionTips;

  /// No description provided for @itemWifiToggle.
  ///
  /// In en, this message translates to:
  /// **'Wi-Fi'**
  String get itemWifiToggle;

  /// No description provided for @itemWifiToggleDesc.
  ///
  /// In en, this message translates to:
  /// **'Connect to a wireless network'**
  String get itemWifiToggleDesc;

  /// No description provided for @itemWifiSsid.
  ///
  /// In en, this message translates to:
  /// **'Network'**
  String get itemWifiSsid;

  /// No description provided for @itemWifiSsidDesc.
  ///
  /// In en, this message translates to:
  /// **'The network this headset is connected to'**
  String get itemWifiSsidDesc;

  /// No description provided for @itemWifiSettings.
  ///
  /// In en, this message translates to:
  /// **'Manage networks'**
  String get itemWifiSettings;

  /// No description provided for @itemWifiSettingsDesc.
  ///
  /// In en, this message translates to:
  /// **'Saved networks and advanced options'**
  String get itemWifiSettingsDesc;

  /// No description provided for @itemBluetoothToggle.
  ///
  /// In en, this message translates to:
  /// **'Bluetooth'**
  String get itemBluetoothToggle;

  /// No description provided for @itemBluetoothToggleDesc.
  ///
  /// In en, this message translates to:
  /// **'Pair controllers and accessories'**
  String get itemBluetoothToggleDesc;

  /// No description provided for @itemBluetoothSettings.
  ///
  /// In en, this message translates to:
  /// **'Manage devices'**
  String get itemBluetoothSettings;

  /// No description provided for @itemBluetoothSettingsDesc.
  ///
  /// In en, this message translates to:
  /// **'Pairing and connected devices'**
  String get itemBluetoothSettingsDesc;

  /// No description provided for @itemControllerPair.
  ///
  /// In en, this message translates to:
  /// **'Scan for controllers'**
  String get itemControllerPair;

  /// No description provided for @itemControllerPairDesc.
  ///
  /// In en, this message translates to:
  /// **'Starts the link scan, then hold the pairing button on each controller until it rumbles'**
  String get itemControllerPairDesc;

  /// No description provided for @itemControllerLeft.
  ///
  /// In en, this message translates to:
  /// **'Left controller'**
  String get itemControllerLeft;

  /// No description provided for @itemControllerLeftDesc.
  ///
  /// In en, this message translates to:
  /// **'Link state and battery level'**
  String get itemControllerLeftDesc;

  /// No description provided for @itemControllerRight.
  ///
  /// In en, this message translates to:
  /// **'Right controller'**
  String get itemControllerRight;

  /// No description provided for @itemControllerRightDesc.
  ///
  /// In en, this message translates to:
  /// **'Link state and battery level'**
  String get itemControllerRightDesc;

  /// No description provided for @itemControllerMain.
  ///
  /// In en, this message translates to:
  /// **'Main hand'**
  String get itemControllerMain;

  /// No description provided for @itemControllerMainDesc.
  ///
  /// In en, this message translates to:
  /// **'The controller used for pointing and system gestures'**
  String get itemControllerMainDesc;

  /// No description provided for @itemControllerUnbind.
  ///
  /// In en, this message translates to:
  /// **'Unpair all'**
  String get itemControllerUnbind;

  /// No description provided for @itemControllerUnbindDesc.
  ///
  /// In en, this message translates to:
  /// **'Forget both paired controllers'**
  String get itemControllerUnbindDesc;

  /// No description provided for @itemBrightness.
  ///
  /// In en, this message translates to:
  /// **'Brightness'**
  String get itemBrightness;

  /// No description provided for @itemBrightnessDesc.
  ///
  /// In en, this message translates to:
  /// **'Adjust the screen brightness'**
  String get itemBrightnessDesc;

  /// No description provided for @itemNightMode.
  ///
  /// In en, this message translates to:
  /// **'Night mode'**
  String get itemNightMode;

  /// No description provided for @itemNightModeDesc.
  ///
  /// In en, this message translates to:
  /// **'Warm the display colors after dark'**
  String get itemNightModeDesc;

  /// No description provided for @itemVolume.
  ///
  /// In en, this message translates to:
  /// **'Volume'**
  String get itemVolume;

  /// No description provided for @itemVolumeDesc.
  ///
  /// In en, this message translates to:
  /// **'Adjust the media volume'**
  String get itemVolumeDesc;

  /// No description provided for @itemMicMute.
  ///
  /// In en, this message translates to:
  /// **'Microphone'**
  String get itemMicMute;

  /// No description provided for @itemMicMuteDesc.
  ///
  /// In en, this message translates to:
  /// **'Allow apps to use the microphone'**
  String get itemMicMuteDesc;

  /// No description provided for @itemSeethrough.
  ///
  /// In en, this message translates to:
  /// **'Seethrough'**
  String get itemSeethrough;

  /// No description provided for @itemSeethroughDesc.
  ///
  /// In en, this message translates to:
  /// **'Show the real world through the cameras'**
  String get itemSeethroughDesc;

  /// No description provided for @itemLanguagePicker.
  ///
  /// In en, this message translates to:
  /// **'Language'**
  String get itemLanguagePicker;

  /// No description provided for @itemLanguagePickerDesc.
  ///
  /// In en, this message translates to:
  /// **'Change the system language'**
  String get itemLanguagePickerDesc;

  /// No description provided for @itemTimeZone.
  ///
  /// In en, this message translates to:
  /// **'Date and time'**
  String get itemTimeZone;

  /// No description provided for @itemTimeZoneDesc.
  ///
  /// In en, this message translates to:
  /// **'Set the time zone and clock format'**
  String get itemTimeZoneDesc;

  /// No description provided for @itemKeyboardPicker.
  ///
  /// In en, this message translates to:
  /// **'Keyboard'**
  String get itemKeyboardPicker;

  /// No description provided for @itemKeyboardPickerDesc.
  ///
  /// In en, this message translates to:
  /// **'Choose the active input method'**
  String get itemKeyboardPickerDesc;

  /// No description provided for @itemTrackingToggle.
  ///
  /// In en, this message translates to:
  /// **'Headset Tracking'**
  String get itemTrackingToggle;

  /// No description provided for @itemTrackingToggleDesc.
  ///
  /// In en, this message translates to:
  /// **'Document your actual movement and position in a play area'**
  String get itemTrackingToggleDesc;

  /// No description provided for @itemTrackingFrequency.
  ///
  /// In en, this message translates to:
  /// **'Tracking Frequency'**
  String get itemTrackingFrequency;

  /// No description provided for @itemTrackingFrequencyDesc.
  ///
  /// In en, this message translates to:
  /// **'Troubleshoot tracking problems by selecting the power frequency of outlets in your region'**
  String get itemTrackingFrequencyDesc;

  /// No description provided for @itemBoundary.
  ///
  /// In en, this message translates to:
  /// **'Boundary'**
  String get itemBoundary;

  /// No description provided for @itemBoundaryDesc.
  ///
  /// In en, this message translates to:
  /// **'Show the play-area boundary while you move'**
  String get itemBoundaryDesc;

  /// No description provided for @itemResetView.
  ///
  /// In en, this message translates to:
  /// **'Reset view'**
  String get itemResetView;

  /// No description provided for @itemResetViewDesc.
  ///
  /// In en, this message translates to:
  /// **'Recenter the headset orientation'**
  String get itemResetViewDesc;

  /// No description provided for @itemBackupNow.
  ///
  /// In en, this message translates to:
  /// **'Back up now'**
  String get itemBackupNow;

  /// No description provided for @itemBackupNowDesc.
  ///
  /// In en, this message translates to:
  /// **'Back up app data and settings'**
  String get itemBackupNowDesc;

  /// No description provided for @itemDevOptions.
  ///
  /// In en, this message translates to:
  /// **'Developer options'**
  String get itemDevOptions;

  /// No description provided for @itemDevOptionsDesc.
  ///
  /// In en, this message translates to:
  /// **'Open the system developer settings'**
  String get itemDevOptionsDesc;

  /// No description provided for @itemUpdateCheck.
  ///
  /// In en, this message translates to:
  /// **'Check for updates'**
  String get itemUpdateCheck;

  /// No description provided for @itemUpdateCheckDesc.
  ///
  /// In en, this message translates to:
  /// **'Look for a newer system image'**
  String get itemUpdateCheckDesc;

  /// No description provided for @itemBuildNumber.
  ///
  /// In en, this message translates to:
  /// **'Build'**
  String get itemBuildNumber;

  /// No description provided for @itemBuildNumberDesc.
  ///
  /// In en, this message translates to:
  /// **'The build this system image was made from'**
  String get itemBuildNumberDesc;

  /// No description provided for @itemModelName.
  ///
  /// In en, this message translates to:
  /// **'Model'**
  String get itemModelName;

  /// No description provided for @itemModelNameDesc.
  ///
  /// In en, this message translates to:
  /// **'The hardware model of this headset'**
  String get itemModelNameDesc;

  /// No description provided for @itemAndroidVersion.
  ///
  /// In en, this message translates to:
  /// **'Android version'**
  String get itemAndroidVersion;

  /// No description provided for @itemAndroidVersionDesc.
  ///
  /// In en, this message translates to:
  /// **'The Android release this system runs'**
  String get itemAndroidVersionDesc;

  /// No description provided for @itemAboutOpen.
  ///
  /// In en, this message translates to:
  /// **'More'**
  String get itemAboutOpen;

  /// No description provided for @itemAboutOpenDesc.
  ///
  /// In en, this message translates to:
  /// **'Open the full system information page'**
  String get itemAboutOpenDesc;

  /// No description provided for @itemTipsBody.
  ///
  /// In en, this message translates to:
  /// **'Getting around'**
  String get itemTipsBody;

  /// No description provided for @itemTipsBodyDesc.
  ///
  /// In en, this message translates to:
  /// **'Use the dock to switch apps, the quick panel for radios and volume, and this page for everything else.'**
  String get itemTipsBodyDesc;

  /// No description provided for @valueAuto.
  ///
  /// In en, this message translates to:
  /// **'Auto'**
  String get valueAuto;

  /// No description provided for @value60hz.
  ///
  /// In en, this message translates to:
  /// **'60 Hz'**
  String get value60hz;

  /// No description provided for @value50hz.
  ///
  /// In en, this message translates to:
  /// **'50 Hz'**
  String get value50hz;

  /// No description provided for @valueNotConnected.
  ///
  /// In en, this message translates to:
  /// **'Not connected'**
  String get valueNotConnected;

  /// No description provided for @valueUnknown.
  ///
  /// In en, this message translates to:
  /// **'Unknown'**
  String get valueUnknown;

  /// No description provided for @valueLeft.
  ///
  /// In en, this message translates to:
  /// **'Left'**
  String get valueLeft;

  /// No description provided for @valueRight.
  ///
  /// In en, this message translates to:
  /// **'Right'**
  String get valueRight;

  /// No description provided for @controllerConnected.
  ///
  /// In en, this message translates to:
  /// **'Connected'**
  String get controllerConnected;

  /// No description provided for @controllerDisconnected.
  ///
  /// In en, this message translates to:
  /// **'Disconnected'**
  String get controllerDisconnected;

  /// No description provided for @controllerPairing.
  ///
  /// In en, this message translates to:
  /// **'Pairing…'**
  String get controllerPairing;

  /// No description provided for @controllerBattery.
  ///
  /// In en, this message translates to:
  /// **'Battery {level} of 5'**
  String controllerBattery(int level);

  /// No description provided for @controllerCharging.
  ///
  /// In en, this message translates to:
  /// **'Charging'**
  String get controllerCharging;
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
