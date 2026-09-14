// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'App Library';

  @override
  String get searchHint => 'Search';

  @override
  String get clearSearch => 'Clear search';

  @override
  String get collectionAll => 'All';

  @override
  String collectionAllCount(int count) {
    return 'All ($count)';
  }

  @override
  String collectionCount(String label, int count) {
    return '$label ($count)';
  }

  @override
  String get collectionPinned => 'Pinned';

  @override
  String get collectionUserApps => 'Apps';

  @override
  String get collectionSystemApps => 'System';

  @override
  String get collectionGroups => 'Groups';

  @override
  String get sortCustom => 'Custom order';

  @override
  String get sortNameAsc => 'A-Z';

  @override
  String get sortNameDesc => 'Z-A';

  @override
  String get sortNewest => 'Recently installed';

  @override
  String get sortUpdated => 'Recently updated';

  @override
  String get openApp => 'Open';

  @override
  String get pinApp => 'Pin';

  @override
  String get unpinApp => 'Unpin';

  @override
  String get uninstallApp => 'Uninstall';

  @override
  String get appInfo => 'App info';

  @override
  String get addToGroup => 'Add to group';

  @override
  String get removeFromGroup => 'Remove from group';

  @override
  String get newGroup => 'New group';

  @override
  String get renameGroup => 'Rename group';

  @override
  String get deleteGroup => 'Delete group';

  @override
  String deleteGroupConfirm(String name) {
    return 'Delete \"$name\"? The apps stay installed.';
  }

  @override
  String get groupNameHint => 'Group name';

  @override
  String get noGroupsYet => 'No groups yet';

  @override
  String get installApp => 'Install app';

  @override
  String get emptyLibrary => 'No apps installed';

  @override
  String emptySearch(String query) {
    return 'No results for \"$query\"';
  }

  @override
  String get emptyGroup => 'No apps in this group';

  @override
  String get loadingApps => 'Loading apps…';

  @override
  String get loadFailed => 'Couldn\'t load apps';

  @override
  String launchFailed(String app) {
    return 'Couldn\'t open $app';
  }

  @override
  String uninstallFailed(String app) {
    return 'Couldn\'t uninstall $app';
  }

  @override
  String get retry => 'Retry';

  @override
  String get cancel => 'Cancel';

  @override
  String get create => 'Create';

  @override
  String get save => 'Save';

  @override
  String get delete => 'Delete';

  @override
  String appCount(int count) {
    String _temp0 = intl.Intl.pluralLogic(
      count,
      locale: localeName,
      other: '$count apps',
      one: '1 app',
      zero: 'No apps',
    );
    return '$_temp0';
  }

  @override
  String get systemBadge => 'System';

  @override
  String get details => 'Details';

  @override
  String detailsTitle(String app) {
    return 'About $app';
  }

  @override
  String get detailsPackage => 'Package';

  @override
  String get detailsVersion => 'Version';

  @override
  String get detailsInstalled => 'Installed';

  @override
  String get detailsUpdated => 'Updated';

  @override
  String get notAvailable => '—';
}
