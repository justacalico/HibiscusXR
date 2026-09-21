// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'Quick Settings';

  @override
  String get settings => 'Settings';

  @override
  String get tileWifi => 'Wi-Fi';

  @override
  String get tileBluetooth => 'Bluetooth';

  @override
  String get tileSeethrough => 'Seethrough';

  @override
  String get tileBoundary => 'Boundary';

  @override
  String get tileMicrophone => 'Microphone';

  @override
  String get tileNightMode => 'Night mode';

  @override
  String get tileDoNotDisturb => 'Do Not Disturb';

  @override
  String get tileAirplaneMode => 'Airplane mode';

  @override
  String get tileBatterySaver => 'Battery saver';

  @override
  String get actionResetView => 'Reset view';

  @override
  String get actionReportProblem => 'Report problem';

  @override
  String get actionAboutDevice => 'About device';

  @override
  String get sliderVolume => 'Volume';

  @override
  String get sliderBrightness => 'Brightness';

  @override
  String get stateOn => 'On';

  @override
  String get stateOff => 'Off';

  @override
  String get stateNotConnected => 'Not Connected';

  @override
  String get notifTitle => 'Notifications';

  @override
  String get notifEmpty => 'No notifications';

  @override
  String get notifDismiss => 'Dismiss';

  @override
  String get notifClearAll => 'Clear all';

  @override
  String batteryPercent(int level) {
    return '$level%';
  }
}
