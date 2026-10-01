// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'HBSUP';

  @override
  String get appSubtitle => 'Hibiscus Backup';

  @override
  String get unsupportedHostTitle => 'Unsupported host OS';

  @override
  String get unsupportedHostBody =>
      'Windows is not a supported host OS for HBSUP. Things may not work - use Linux or macOS if you run into problems.';

  @override
  String get unsupportedHostDismiss => 'Dismiss';

  @override
  String get connectTitle => 'Connect a headset';

  @override
  String get connectHint =>
      'HBSUP needs rooted adb. Boot the stock system, run adb root, then plug the headset in.';

  @override
  String get connectRefresh => 'Refresh';

  @override
  String get connectNoDevices =>
      'No adb devices. Plug a headset in, or bring wireless adb up first.';

  @override
  String get connectUnknown => 'Unknown model';

  @override
  String get connectButton => 'Connect';

  @override
  String get disconnect => 'Disconnect';

  @override
  String get backupDestTitle => 'Backup folder';

  @override
  String get backupDestPick => 'Choose folder';

  @override
  String get backupDestNone => 'No folder picked yet';

  @override
  String backupFreeSpace(String free) {
    return '$free free';
  }

  @override
  String get backupFreeUnknown => 'Free space unknown';

  @override
  String get backupPartsTitle => 'Partitions';

  @override
  String get backupPartsLoading => 'Reading the partition table...';

  @override
  String get backupPartsEmpty =>
      'No partitions found. Is adb rooted on the headset?';

  @override
  String backupTotalSize(String size) {
    return 'Full dump: $size';
  }

  @override
  String get backupSpaceOk => 'Enough free space for the full dump';

  @override
  String backupSpaceShort(String needed, String free) {
    return 'Not enough space - the dump needs $needed, only $free free';
  }

  @override
  String get backupStart => 'Start backup';

  @override
  String get backupRunning => 'Backing up...';

  @override
  String get backupCancel => 'Cancel';

  @override
  String get backupDone =>
      'Backup finished. Keep this folder somewhere safe - it is your way back to stock.';

  @override
  String backupFailed(String error) {
    return 'Backup failed: $error';
  }

  @override
  String get backupCancelled => 'Backup cancelled';

  @override
  String backupProgress(int done, int total) {
    return '$done of $total partitions';
  }

  @override
  String get backupLogTitle => 'Log';
}
