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

  /// No description provided for @sectionEnvironment.
  ///
  /// In en, this message translates to:
  /// **'Home environment'**
  String get sectionEnvironment;

  /// No description provided for @sectionSound.
  ///
  /// In en, this message translates to:
  /// **'Sound'**
  String get sectionSound;

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

  /// No description provided for @sectionDeveloper.
  ///
  /// In en, this message translates to:
  /// **'Developer'**
  String get sectionDeveloper;

  /// No description provided for @sectionAbout.
  ///
  /// In en, this message translates to:
  /// **'About'**
  String get sectionAbout;

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

  /// No description provided for @itemWifiList.
  ///
  /// In en, this message translates to:
  /// **'Available networks'**
  String get itemWifiList;

  /// No description provided for @itemWifiListDesc.
  ///
  /// In en, this message translates to:
  /// **'Tap a network to connect or manage it'**
  String get itemWifiListDesc;

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

  /// No description provided for @itemBtList.
  ///
  /// In en, this message translates to:
  /// **'Devices'**
  String get itemBtList;

  /// No description provided for @itemBtListDesc.
  ///
  /// In en, this message translates to:
  /// **'Bonded and nearby Bluetooth devices'**
  String get itemBtListDesc;

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

  /// No description provided for @itemTheme.
  ///
  /// In en, this message translates to:
  /// **'Theme'**
  String get itemTheme;

  /// No description provided for @itemThemeDesc.
  ///
  /// In en, this message translates to:
  /// **'Colors for the panels and the headset HUD'**
  String get itemThemeDesc;

  /// No description provided for @themeDark.
  ///
  /// In en, this message translates to:
  /// **'Dark'**
  String get themeDark;

  /// No description provided for @themeLight.
  ///
  /// In en, this message translates to:
  /// **'Light'**
  String get themeLight;

  /// No description provided for @themeOled.
  ///
  /// In en, this message translates to:
  /// **'OLED'**
  String get themeOled;

  /// No description provided for @itemIpd.
  ///
  /// In en, this message translates to:
  /// **'Eye spacing (IPD)'**
  String get itemIpd;

  /// No description provided for @itemIpdDesc.
  ///
  /// In en, this message translates to:
  /// **'Match the stereo eye separation to the distance between your pupils. The Neo 2 lenses are fixed, so this shifts the rendered views'**
  String get itemIpdDesc;

  /// No description provided for @itemDeviceMode.
  ///
  /// In en, this message translates to:
  /// **'Device mode'**
  String get itemDeviceMode;

  /// No description provided for @itemDeviceModeDesc.
  ///
  /// In en, this message translates to:
  /// **'3DoF tracks head rotation only. 6DoF also tracks position through the tracking cameras'**
  String get itemDeviceModeDesc;

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

  /// No description provided for @itemEnvList.
  ///
  /// In en, this message translates to:
  /// **'Home environments'**
  String get itemEnvList;

  /// No description provided for @itemEnvListDesc.
  ///
  /// In en, this message translates to:
  /// **'Pick what surrounds you at home. Push .zip files to /data/local/tmp/hibiscus/envs over adb'**
  String get itemEnvListDesc;

  /// No description provided for @itemHomeEnv.
  ///
  /// In en, this message translates to:
  /// **'Active environment'**
  String get itemHomeEnv;

  /// No description provided for @itemHomeEnvDesc.
  ///
  /// In en, this message translates to:
  /// **'The environment the home shell loads'**
  String get itemHomeEnvDesc;

  /// No description provided for @envPassthrough.
  ///
  /// In en, this message translates to:
  /// **'Passthrough'**
  String get envPassthrough;

  /// No description provided for @envPassthroughDesc.
  ///
  /// In en, this message translates to:
  /// **'Live view from the tracking cameras'**
  String get envPassthroughDesc;

  /// No description provided for @envBuiltin.
  ///
  /// In en, this message translates to:
  /// **'Built-in'**
  String get envBuiltin;

  /// No description provided for @envBuiltinDesc.
  ///
  /// In en, this message translates to:
  /// **'The default sky dome and floor grid'**
  String get envBuiltinDesc;

  /// No description provided for @envEmpty.
  ///
  /// In en, this message translates to:
  /// **'No environments installed'**
  String get envEmpty;

  /// No description provided for @envMissingMap.
  ///
  /// In en, this message translates to:
  /// **'No map.obj inside'**
  String get envMissingMap;

  /// No description provided for @envBadName.
  ///
  /// In en, this message translates to:
  /// **'Filename is not a valid environment id'**
  String get envBadName;

  /// No description provided for @envUnreadable.
  ///
  /// In en, this message translates to:
  /// **'Not a readable zip'**
  String get envUnreadable;

  /// No description provided for @envRemoveTitle.
  ///
  /// In en, this message translates to:
  /// **'Remove {name}?'**
  String envRemoveTitle(String name);

  /// No description provided for @envRemoveBody.
  ///
  /// In en, this message translates to:
  /// **'The zip file is deleted from the headset.'**
  String get envRemoveBody;

  /// No description provided for @envRemove.
  ///
  /// In en, this message translates to:
  /// **'Remove'**
  String get envRemove;

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

  /// No description provided for @languageEnglish.
  ///
  /// In en, this message translates to:
  /// **'English'**
  String get languageEnglish;

  /// No description provided for @languageChineseSimplified.
  ///
  /// In en, this message translates to:
  /// **'简体中文'**
  String get languageChineseSimplified;

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

  /// No description provided for @itemImeList.
  ///
  /// In en, this message translates to:
  /// **'On-screen keyboard'**
  String get itemImeList;

  /// No description provided for @itemImeListDesc.
  ///
  /// In en, this message translates to:
  /// **'Choose the active input method'**
  String get itemImeListDesc;

  /// No description provided for @itemAdbToggle.
  ///
  /// In en, this message translates to:
  /// **'USB debugging'**
  String get itemAdbToggle;

  /// No description provided for @itemAdbToggleDesc.
  ///
  /// In en, this message translates to:
  /// **'Allow a computer to debug this headset over USB'**
  String get itemAdbToggleDesc;

  /// No description provided for @itemStayAwake.
  ///
  /// In en, this message translates to:
  /// **'Stay awake'**
  String get itemStayAwake;

  /// No description provided for @itemStayAwakeDesc.
  ///
  /// In en, this message translates to:
  /// **'Never sleep while charging'**
  String get itemStayAwakeDesc;

  /// No description provided for @itemShowTouches.
  ///
  /// In en, this message translates to:
  /// **'Show touches'**
  String get itemShowTouches;

  /// No description provided for @itemShowTouchesDesc.
  ///
  /// In en, this message translates to:
  /// **'Flash a dot where the screen is touched'**
  String get itemShowTouchesDesc;

  /// No description provided for @itemDebugHud.
  ///
  /// In en, this message translates to:
  /// **'Debug HUD'**
  String get itemDebugHud;

  /// No description provided for @itemDebugHudDesc.
  ///
  /// In en, this message translates to:
  /// **'Keep the debug status line visible over any app'**
  String get itemDebugHudDesc;

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

  /// No description provided for @itemHibiscusVersion.
  ///
  /// In en, this message translates to:
  /// **'Hibiscus version'**
  String get itemHibiscusVersion;

  /// No description provided for @itemHibiscusVersionDesc.
  ///
  /// In en, this message translates to:
  /// **'The Hibiscus build installed on this headset'**
  String get itemHibiscusVersionDesc;

  /// No description provided for @itemAboutBrand.
  ///
  /// In en, this message translates to:
  /// **'HibiscusXR'**
  String get itemAboutBrand;

  /// No description provided for @itemAboutBrandDesc.
  ///
  /// In en, this message translates to:
  /// **'The system software on this headset'**
  String get itemAboutBrandDesc;

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

  /// No description provided for @value3dof.
  ///
  /// In en, this message translates to:
  /// **'3DoF'**
  String get value3dof;

  /// No description provided for @value6dof.
  ///
  /// In en, this message translates to:
  /// **'6DoF'**
  String get value6dof;

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

  /// No description provided for @controllerScanning.
  ///
  /// In en, this message translates to:
  /// **'Scanning for controllers…'**
  String get controllerScanning;

  /// No description provided for @controllerScanIdle.
  ///
  /// In en, this message translates to:
  /// **'Not scanning'**
  String get controllerScanIdle;

  /// No description provided for @wifiScanning.
  ///
  /// In en, this message translates to:
  /// **'Scanning for networks…'**
  String get wifiScanning;

  /// No description provided for @wifiScanIdle.
  ///
  /// In en, this message translates to:
  /// **'Tap refresh to scan'**
  String get wifiScanIdle;

  /// No description provided for @wifiEmpty.
  ///
  /// In en, this message translates to:
  /// **'No networks found'**
  String get wifiEmpty;

  /// No description provided for @wifiSecured.
  ///
  /// In en, this message translates to:
  /// **'Secured'**
  String get wifiSecured;

  /// No description provided for @wifiOpen.
  ///
  /// In en, this message translates to:
  /// **'Open'**
  String get wifiOpen;

  /// No description provided for @wifiSaved.
  ///
  /// In en, this message translates to:
  /// **'Saved'**
  String get wifiSaved;

  /// No description provided for @wifiConnected.
  ///
  /// In en, this message translates to:
  /// **'Connected'**
  String get wifiConnected;

  /// No description provided for @wifiRescan.
  ///
  /// In en, this message translates to:
  /// **'Refresh'**
  String get wifiRescan;

  /// No description provided for @wifiJoinTitle.
  ///
  /// In en, this message translates to:
  /// **'Connect to {ssid}'**
  String wifiJoinTitle(String ssid);

  /// No description provided for @wifiPassword.
  ///
  /// In en, this message translates to:
  /// **'Password'**
  String get wifiPassword;

  /// No description provided for @wifiConnect.
  ///
  /// In en, this message translates to:
  /// **'Connect'**
  String get wifiConnect;

  /// No description provided for @wifiForget.
  ///
  /// In en, this message translates to:
  /// **'Forget'**
  String get wifiForget;

  /// No description provided for @btScanning.
  ///
  /// In en, this message translates to:
  /// **'Searching for devices…'**
  String get btScanning;

  /// No description provided for @btScanIdle.
  ///
  /// In en, this message translates to:
  /// **'Tap refresh to search'**
  String get btScanIdle;

  /// No description provided for @btEmpty.
  ///
  /// In en, this message translates to:
  /// **'No devices found'**
  String get btEmpty;

  /// No description provided for @btRescan.
  ///
  /// In en, this message translates to:
  /// **'Refresh'**
  String get btRescan;

  /// No description provided for @btPair.
  ///
  /// In en, this message translates to:
  /// **'Pair'**
  String get btPair;

  /// No description provided for @btPaired.
  ///
  /// In en, this message translates to:
  /// **'Paired'**
  String get btPaired;

  /// No description provided for @btConnected.
  ///
  /// In en, this message translates to:
  /// **'Connected'**
  String get btConnected;

  /// No description provided for @btForget.
  ///
  /// In en, this message translates to:
  /// **'Forget'**
  String get btForget;

  /// No description provided for @imeActive.
  ///
  /// In en, this message translates to:
  /// **'In use'**
  String get imeActive;

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

  /// No description provided for @sliderPercent.
  ///
  /// In en, this message translates to:
  /// **'{percent}%'**
  String sliderPercent(int percent);

  /// No description provided for @ipdMm.
  ///
  /// In en, this message translates to:
  /// **'{mm} mm'**
  String ipdMm(String mm);

  /// No description provided for @uiOnlyModeTitle.
  ///
  /// In en, this message translates to:
  /// **'UI-only mode'**
  String get uiOnlyModeTitle;

  /// No description provided for @uiOnlyModeBody.
  ///
  /// In en, this message translates to:
  /// **'This build runs without a system backend, so the page is a preview. Changes made here won\'t reach the device.'**
  String get uiOnlyModeBody;

  /// No description provided for @rebootRequiredTitle.
  ///
  /// In en, this message translates to:
  /// **'Reboot required'**
  String get rebootRequiredTitle;

  /// No description provided for @rebootRequiredBody.
  ///
  /// In en, this message translates to:
  /// **'To change this setting you have to reboot the headset.'**
  String get rebootRequiredBody;

  /// No description provided for @rebootConfirm.
  ///
  /// In en, this message translates to:
  /// **'Reboot'**
  String get rebootConfirm;
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
