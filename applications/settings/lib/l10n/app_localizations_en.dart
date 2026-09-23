// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'Settings';

  @override
  String get sectionWifi => 'Wi-Fi';

  @override
  String get sectionBluetooth => 'Bluetooth';

  @override
  String get sectionControllers => 'Controllers';

  @override
  String get sectionDisplay => 'Display';

  @override
  String get sectionSound => 'Sound';

  @override
  String get sectionLanguage => 'Language and Region';

  @override
  String get sectionTime => 'Time';

  @override
  String get sectionKeyboard => 'Keyboard';

  @override
  String get sectionDeveloper => 'Developer';

  @override
  String get sectionAbout => 'About';

  @override
  String get itemWifiToggle => 'Wi-Fi';

  @override
  String get itemWifiToggleDesc => 'Connect to a wireless network';

  @override
  String get itemWifiSsid => 'Network';

  @override
  String get itemWifiSsidDesc => 'The network this headset is connected to';

  @override
  String get itemWifiSettings => 'Manage networks';

  @override
  String get itemWifiSettingsDesc => 'Saved networks and advanced options';

  @override
  String get itemBluetoothToggle => 'Bluetooth';

  @override
  String get itemBluetoothToggleDesc => 'Pair controllers and accessories';

  @override
  String get itemBluetoothSettings => 'Manage devices';

  @override
  String get itemBluetoothSettingsDesc => 'Pairing and connected devices';

  @override
  String get itemControllerPair => 'Scan for controllers';

  @override
  String get itemControllerPairDesc =>
      'Starts the link scan, then hold the pairing button on each controller until it rumbles';

  @override
  String get itemControllerLeft => 'Left controller';

  @override
  String get itemControllerLeftDesc => 'Link state and battery level';

  @override
  String get itemControllerRight => 'Right controller';

  @override
  String get itemControllerRightDesc => 'Link state and battery level';

  @override
  String get itemControllerUnbind => 'Unpair all';

  @override
  String get itemControllerUnbindDesc => 'Forget both paired controllers';

  @override
  String get itemBrightness => 'Brightness';

  @override
  String get itemBrightnessDesc => 'Adjust the screen brightness';

  @override
  String get itemNightMode => 'Night mode';

  @override
  String get itemNightModeDesc => 'Warm the display colors after dark';

  @override
  String get itemVolume => 'Volume';

  @override
  String get itemVolumeDesc => 'Adjust the media volume';

  @override
  String get itemMicMute => 'Microphone';

  @override
  String get itemMicMuteDesc => 'Allow apps to use the microphone';

  @override
  String get itemLanguagePicker => 'Language';

  @override
  String get itemLanguagePickerDesc => 'Change the system language';

  @override
  String get itemTimeZone => 'Date and time';

  @override
  String get itemTimeZoneDesc => 'Set the time zone and clock format';

  @override
  String get itemKeyboardPicker => 'Keyboard';

  @override
  String get itemKeyboardPickerDesc => 'Choose the active input method';

  @override
  String get itemAdbToggle => 'USB debugging';

  @override
  String get itemAdbToggleDesc =>
      'Allow a computer to debug this headset over USB';

  @override
  String get itemStayAwake => 'Stay awake';

  @override
  String get itemStayAwakeDesc => 'Never sleep while charging';

  @override
  String get itemShowTouches => 'Show touches';

  @override
  String get itemShowTouchesDesc => 'Flash a dot where the screen is touched';

  @override
  String get itemModelName => 'Model';

  @override
  String get itemModelNameDesc => 'The hardware model of this headset';

  @override
  String get itemAndroidVersion => 'Android version';

  @override
  String get itemAndroidVersionDesc => 'The Android release this system runs';

  @override
  String get itemHibiscusVersion => 'Hibiscus version';

  @override
  String get itemHibiscusVersionDesc =>
      'The Hibiscus build installed on this headset';

  @override
  String get itemAboutBrand => 'HibiscusXR';

  @override
  String get itemAboutBrandDesc => 'The system software on this headset';

  @override
  String get valueNotConnected => 'Not connected';

  @override
  String get valueUnknown => 'Unknown';

  @override
  String get controllerConnected => 'Connected';

  @override
  String get controllerDisconnected => 'Disconnected';

  @override
  String get controllerPairing => 'Pairing…';

  @override
  String get controllerScanning => 'Scanning for controllers…';

  @override
  String get controllerScanIdle => 'Not scanning';

  @override
  String controllerBattery(int level) {
    return 'Battery $level of 5';
  }

  @override
  String get controllerCharging => 'Charging';

  @override
  String sliderPercent(int percent) {
    return '$percent%';
  }

  @override
  String get uiOnlyModeTitle => 'UI-only mode';

  @override
  String get uiOnlyModeBody =>
      'This build runs without a system backend, so the page is a preview. Changes made here won\'t reach the device.';
}
