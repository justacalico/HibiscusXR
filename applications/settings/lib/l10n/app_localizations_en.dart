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
  String get sectionCamera => 'Camera';

  @override
  String get sectionLanguage => 'Language and Region';

  @override
  String get sectionTime => 'Time';

  @override
  String get sectionKeyboard => 'Keyboard';

  @override
  String get sectionHeadsetTracking => 'Headset Tracking';

  @override
  String get sectionBackup => 'Backup';

  @override
  String get sectionDeveloper => 'Developer';

  @override
  String get sectionSoftwareUpdate => 'Software Update';

  @override
  String get sectionAbout => 'About';

  @override
  String get sectionTips => 'Tips and Support';

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
  String get itemSeethrough => 'Seethrough';

  @override
  String get itemSeethroughDesc => 'Show the real world through the cameras';

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
  String get itemTrackingToggle => 'Headset Tracking';

  @override
  String get itemTrackingToggleDesc =>
      'Document your actual movement and position in a play area';

  @override
  String get itemTrackingFrequency => 'Tracking Frequency';

  @override
  String get itemTrackingFrequencyDesc =>
      'Troubleshoot tracking problems by selecting the power frequency of outlets in your region';

  @override
  String get itemBoundary => 'Boundary';

  @override
  String get itemBoundaryDesc => 'Show the play-area boundary while you move';

  @override
  String get itemResetView => 'Reset view';

  @override
  String get itemResetViewDesc => 'Recenter the headset orientation';

  @override
  String get itemBackupNow => 'Back up now';

  @override
  String get itemBackupNowDesc => 'Back up app data and settings';

  @override
  String get itemDevOptions => 'Developer options';

  @override
  String get itemDevOptionsDesc => 'Open the system developer settings';

  @override
  String get itemUpdateCheck => 'Check for updates';

  @override
  String get itemUpdateCheckDesc => 'Look for a newer system image';

  @override
  String get itemBuildNumber => 'Build';

  @override
  String get itemBuildNumberDesc => 'The build this system image was made from';

  @override
  String get itemModelName => 'Model';

  @override
  String get itemModelNameDesc => 'The hardware model of this headset';

  @override
  String get itemAndroidVersion => 'Android version';

  @override
  String get itemAndroidVersionDesc => 'The Android release this system runs';

  @override
  String get itemAboutBrand => 'HibiscusXR';

  @override
  String get itemAboutBrandDesc => 'The system software on this headset';

  @override
  String get itemTipsBody => 'Getting around';

  @override
  String get itemTipsBodyDesc =>
      'Use the dock to switch apps, the quick panel for radios and volume, and this page for everything else.';

  @override
  String get valueAuto => 'Auto';

  @override
  String get value60hz => '60 Hz';

  @override
  String get value50hz => '50 Hz';

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
}
