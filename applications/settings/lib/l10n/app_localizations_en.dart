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
  String get sectionEnvironment => 'Home environment';

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
  String get itemWifiList => 'Available networks';

  @override
  String get itemWifiListDesc => 'Tap a network to connect or manage it';

  @override
  String get itemBluetoothToggle => 'Bluetooth';

  @override
  String get itemBluetoothToggleDesc => 'Pair controllers and accessories';

  @override
  String get itemBtList => 'Devices';

  @override
  String get itemBtListDesc => 'Bonded and nearby Bluetooth devices';

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
  String get itemTheme => 'Theme';

  @override
  String get itemThemeDesc => 'Colors for the panels and the headset HUD';

  @override
  String get themeDark => 'Dark';

  @override
  String get themeLight => 'Light';

  @override
  String get themeOled => 'OLED';

  @override
  String get itemIpd => 'Eye spacing (IPD)';

  @override
  String get itemIpdDesc =>
      'Match the stereo eye separation to the distance between your pupils. The Neo 2 lenses are fixed, so this shifts the rendered views';

  @override
  String get itemDeviceMode => 'Device mode';

  @override
  String get itemDeviceModeDesc =>
      '3DoF tracks head rotation only. 6DoF also tracks position through the tracking cameras';

  @override
  String get itemNightMode => 'Night mode';

  @override
  String get itemNightModeDesc => 'Warm the display colors after dark';

  @override
  String get itemEnvList => 'Home environments';

  @override
  String get itemEnvListDesc =>
      'Pick what surrounds you at home. Push .zip files to /data/local/tmp/hibiscus/envs over adb';

  @override
  String get itemHomeEnv => 'Active environment';

  @override
  String get itemHomeEnvDesc => 'The environment the home shell loads';

  @override
  String get envPassthrough => 'Passthrough';

  @override
  String get envPassthroughDesc => 'Live view from the tracking cameras';

  @override
  String get envBuiltin => 'Built-in';

  @override
  String get envBuiltinDesc => 'The default sky dome and floor grid';

  @override
  String get envEmpty => 'No environments installed';

  @override
  String get envMissingMap => 'No map.obj inside';

  @override
  String get envBadName => 'Filename is not a valid environment id';

  @override
  String get envUnreadable => 'Not a readable zip';

  @override
  String envRemoveTitle(String name) {
    return 'Remove $name?';
  }

  @override
  String get envRemoveBody => 'The zip file is deleted from the headset.';

  @override
  String get envRemove => 'Remove';

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
  String get languageEnglish => 'English';

  @override
  String get languageChineseSimplified => '简体中文';

  @override
  String get itemTimeZone => 'Date and time';

  @override
  String get itemTimeZoneDesc => 'Set the time zone and clock format';

  @override
  String get itemImeList => 'On-screen keyboard';

  @override
  String get itemImeListDesc => 'Choose the active input method';

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
  String get itemDebugHud => 'Debug HUD';

  @override
  String get itemDebugHudDesc =>
      'Keep the debug status line visible over any app';

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
  String get value3dof => '3DoF';

  @override
  String get value6dof => '6DoF';

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
  String get wifiScanning => 'Scanning for networks…';

  @override
  String get wifiScanIdle => 'Tap refresh to scan';

  @override
  String get wifiEmpty => 'No networks found';

  @override
  String get wifiSecured => 'Secured';

  @override
  String get wifiOpen => 'Open';

  @override
  String get wifiSaved => 'Saved';

  @override
  String get wifiConnected => 'Connected';

  @override
  String get wifiRescan => 'Refresh';

  @override
  String wifiJoinTitle(String ssid) {
    return 'Connect to $ssid';
  }

  @override
  String get wifiPassword => 'Password';

  @override
  String get wifiConnect => 'Connect';

  @override
  String get wifiForget => 'Forget';

  @override
  String get btScanning => 'Searching for devices…';

  @override
  String get btScanIdle => 'Tap refresh to search';

  @override
  String get btEmpty => 'No devices found';

  @override
  String get btRescan => 'Refresh';

  @override
  String get btPair => 'Pair';

  @override
  String get btPaired => 'Paired';

  @override
  String get btConnected => 'Connected';

  @override
  String get btForget => 'Forget';

  @override
  String get imeActive => 'In use';

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
  String ipdMm(String mm) {
    return '$mm mm';
  }

  @override
  String get uiOnlyModeTitle => 'UI-only mode';

  @override
  String get uiOnlyModeBody =>
      'This build runs without a system backend, so the page is a preview. Changes made here won\'t reach the device.';

  @override
  String get rebootRequiredTitle => 'Reboot required';

  @override
  String get rebootRequiredBody =>
      'To change this setting you have to reboot the headset.';

  @override
  String get rebootConfirm => 'Reboot';
}
