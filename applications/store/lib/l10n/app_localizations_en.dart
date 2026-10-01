// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'Store';

  @override
  String get searchHint => 'Search apps';

  @override
  String get categoryAll => 'All';

  @override
  String get sortUpdated => 'Recently updated';

  @override
  String get sortName => 'Name A-Z';

  @override
  String appsCount(int count) {
    String _temp0 = intl.Intl.pluralLogic(
      count,
      locale: localeName,
      other: '$count apps',
      one: '1 app',
    );
    return '$_temp0';
  }

  @override
  String get install => 'Install';

  @override
  String get installing => 'Installing';

  @override
  String get downloading => 'Downloading';

  @override
  String get open => 'Open';

  @override
  String get retry => 'Retry';

  @override
  String get installedBadge => 'Installed';

  @override
  String get installFailed => 'Install failed';

  @override
  String get installPrompted => 'Confirm the install in the system dialog';

  @override
  String versionRow(String version, int code) {
    return '$version ($code)';
  }

  @override
  String get loadErrorTitle => 'Couldn\'t load the catalog';

  @override
  String get loadErrorBody =>
      'Check the repository address and the network connection.';

  @override
  String get emptyResults => 'No apps match your search';

  @override
  String get selectAppPrompt => 'Pick an app to see details';

  @override
  String get repoMenu => 'Repository';

  @override
  String get repoDialogTitle => 'Repository';

  @override
  String get repoDialogBody =>
      'Catalog and packages come from this F-Droid compatible repository.';

  @override
  String get repoUrlLabel => 'Repository URL';

  @override
  String get repoUrlInvalid => 'Enter an http or https URL';

  @override
  String get repoReset => 'Reset to default';

  @override
  String get cancel => 'Cancel';

  @override
  String get save => 'Save';

  @override
  String get sectionDescription => 'About';

  @override
  String get sectionScreenshots => 'Screenshots';

  @override
  String get sectionVersions => 'Versions';

  @override
  String get metaVersion => 'Version';

  @override
  String get metaSize => 'Size';

  @override
  String get metaLicense => 'License';

  @override
  String get metaUpdated => 'Updated';

  @override
  String get metaMinSdk => 'Min SDK';

  @override
  String byAuthor(String author) {
    return 'by $author';
  }
}
