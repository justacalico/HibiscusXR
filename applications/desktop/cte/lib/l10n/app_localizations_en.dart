// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'HCTE';

  @override
  String get appSubtitle => 'Hibiscus Controller Testing Environment';

  @override
  String get navOverview => 'Overview';

  @override
  String get navDisplay => 'Display';

  @override
  String get navInstall => 'Install';

  @override
  String get navTracking => 'Tracking';

  @override
  String get navDebug => 'Debug';

  @override
  String get connectTitle => 'Connect to a headset';

  @override
  String get connectUsbSection => 'USB / adb devices';

  @override
  String get connectWirelessSection => 'Wireless';

  @override
  String get connectWirelessHint => 'Headset IP address';

  @override
  String get connectWirelessButton => 'Connect';

  @override
  String get connectRefresh => 'Refresh';

  @override
  String get connectScanning => 'Scanning for devices...';

  @override
  String get connectConnecting => 'Connecting...';

  @override
  String get connectNoDevices =>
      'No adb devices. Plug a headset in, or connect over Wi-Fi.';

  @override
  String get connectWirelessNote =>
      'Tries the on-device CTE service first, then wireless adb.';

  @override
  String get disconnect => 'Disconnect';

  @override
  String get overviewHeadset => 'Headset';

  @override
  String get overviewControllers => 'Controllers';

  @override
  String get overviewLink => 'Link';

  @override
  String get overviewBattery => 'Battery';

  @override
  String get overviewTrackingMode => 'Tracking';

  @override
  String get overviewAndroid => 'Android';

  @override
  String get overviewBuild => 'Build';

  @override
  String get overviewSerial => 'Serial';

  @override
  String get overviewAddress => 'Address';

  @override
  String get overviewHibiscus => 'Hibiscus';

  @override
  String get ctrlLeft => 'Left controller';

  @override
  String get ctrlRight => 'Right controller';

  @override
  String get ctrlConnected => 'connected';

  @override
  String get ctrlAbsent => 'not connected';

  @override
  String ctrlBattery(Object pct) {
    return '$pct%';
  }

  @override
  String get ctrlTracked => 'tracked';

  @override
  String get ctrlUntracked => 'no tracking';

  @override
  String get displayMirror => 'Mirror';

  @override
  String get displayStart => 'Start mirroring';

  @override
  String get displayStop => 'Stop';

  @override
  String get displayHint =>
      'The mirror streams as fast as the headset can capture the screen.';

  @override
  String displayStats(Object count, Object rate) {
    return '$rate fps · $count frames';
  }

  @override
  String get displayNoFrame => 'No frames yet';

  @override
  String get installTitle => 'Install an APK';

  @override
  String get installPick => 'Choose APK';

  @override
  String get installRun => 'Install';

  @override
  String get installRunning => 'Installing...';

  @override
  String get installDone => 'Finished';

  @override
  String get installDrop => 'Pick an .apk to push to the headset.';

  @override
  String get trackingTitle => 'Tracking';

  @override
  String get trackingHead => 'Head pose';

  @override
  String trackingRate(Object rate) {
    return '$rate samples/s';
  }

  @override
  String trackingMode(String mode) {
    String _temp0 = intl.Intl.selectLogic(mode, {
      'dof6': '6DoF',
      'dof3': '3DoF',
      'other': 'unknown',
    });
    return '$_temp0';
  }

  @override
  String get trackingIdle =>
      'No pose data - poses only flow while a VR app is running on the headset.';

  @override
  String get trackingPosition => 'Position';

  @override
  String get trackingOrientation => 'Orientation';

  @override
  String get trackingLog => 'Pose log';

  @override
  String get debugTitle => 'Debug';

  @override
  String get debugProps => 'Properties';

  @override
  String get debugFilter => 'Filter';

  @override
  String get debugLogcat => 'Logcat';

  @override
  String get debugEmpty => 'Connect a headset to read its state.';

  @override
  String get valueUnknown => 'unknown';
}
