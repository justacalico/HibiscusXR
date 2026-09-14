// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'Library';

  @override
  String get projectName => 'PN2Lineage';

  @override
  String get navFeatures => 'Features';

  @override
  String get navScreenshots => 'Screenshots';

  @override
  String get navDownload => 'Download';

  @override
  String get navAbout => 'About';

  @override
  String get navFaq => 'FAQ';

  @override
  String get navDocs => 'Docs';

  @override
  String get navMenu => 'Menu';

  @override
  String get navClose => 'Close';

  @override
  String get themeSystem => 'System';

  @override
  String get themeLight => 'Light';

  @override
  String get themeDark => 'Dark';

  @override
  String get themeLabel => 'Appearance';

  @override
  String get languageLabel => 'Language';

  @override
  String get heroEyebrow => 'For the Pico Neo 2 LineageOS port';

  @override
  String get heroTitle => 'Library';

  @override
  String get heroSubtitle =>
      'Every app on the headset, in one quiet grid. Search it, pin it, group it, launch it.';

  @override
  String get heroPrimary => 'See features';

  @override
  String get heroSecondary => 'Read the docs';

  @override
  String get homeShowcaseEyebrow => 'App grid';

  @override
  String get homeShowcaseTitle => 'Everything installed, at a glance.';

  @override
  String get homeShowcaseBody =>
      'Tiles tinted by each app\'s own icon colour, with labels, badges and a kebab menu on every tile. Made to sit inside the vrhome shell as the app window.';

  @override
  String get homeFindTitle => 'Find anything fast';

  @override
  String get homeFindBody =>
      'Search ignores accents and case, collections split apps from system packages, and four sort orders keep the grid the way you left it.';

  @override
  String get homeArrangeTitle => 'Your order, kept';

  @override
  String get homeArrangeBody =>
      'Pin favourites to the top, long-press-drag tiles into place, and file apps into groups you name yourself.';

  @override
  String get homeControlTitle => 'Controller first';

  @override
  String get homeControlBody =>
      'Arrow keys move focus, confirm launches, and the menu key opens the tile menu. Built for a controller, usable with anything.';

  @override
  String get homeOpenTitle => 'Open all the way down.';

  @override
  String get homeOpenBody =>
      'Library is free software under the AGPL-3.0. Every screen, every string, every commit lives in the open on GitLab.';

  @override
  String get homeOpenSource => 'Browse the source';

  @override
  String get homeDownloadTitle => 'Not shipping yet.';

  @override
  String get homeDownloadBody =>
      'There is no release build to install today. The port is still being assembled piece by piece - watch the repository or build from source if you want it early.';

  @override
  String get homeDownloadCta => 'Download status';

  @override
  String get featuresTitle => 'Features';

  @override
  String get featuresSubtitle => 'A launcher-shaped tool, tuned for a headset.';

  @override
  String get featGridTitle => 'A grid that reads like a shelf';

  @override
  String get featGridBody =>
      'Every launchable app gets a tile: icon, label and a backdrop tinted from the icon\'s own dominant colour. New installs and removals show up live, no refresh needed.';

  @override
  String get featFindTitle => 'Find';

  @override
  String get featSearchTitle => 'Search that forgives';

  @override
  String get featSearchBody =>
      'Matching ignores case and accents, so typing the way you remember a name is enough to find it.';

  @override
  String get featCollectionsTitle => 'Collections with counts';

  @override
  String get featCollectionsBody =>
      'All, Pinned, Apps and System are always there. Groups you make join the same row, each with a live count.';

  @override
  String get featSortTitle => 'Four sort orders';

  @override
  String get featSortBody =>
      'A-Z, Z-A, recently installed and recently updated - or a custom order once you start dragging tiles.';

  @override
  String get featArrangeTitle => 'Arrange';

  @override
  String get featPinTitle => 'Pin to the top';

  @override
  String get featPinBody =>
      'Pinned apps lead the grid in every collection. Long-press-drag any tile to rearrange; the order survives restarts.';

  @override
  String get featGroupsTitle => 'Groups';

  @override
  String get featGroupsBody =>
      'Create, rename and delete groups, then file apps into them from the tile menu. A group is just another collection.';

  @override
  String get featControlTitle => 'Control';

  @override
  String get featMenuTitle => 'A menu on every tile';

  @override
  String get featMenuBody =>
      'Open, pin, add to group, app details and uninstall for user apps - one long press or menu key away.';

  @override
  String get featInstallTitle => 'Install APKs in place';

  @override
  String get featInstallBody =>
      'The system file picker handles installs, so sideloading never leaves the window.';

  @override
  String get featNavigateTitle => 'Navigate';

  @override
  String get featDpadTitle => 'D-pad native';

  @override
  String get featDpadBody =>
      'Arrows move focus, confirm launches the focused app, and the menu key opens its tile menu. Focus wraps the way a TV launcher does.';

  @override
  String get featLiveTitle => 'Live package updates';

  @override
  String get featLiveBody =>
      'Install or remove an app anywhere on the system and the grid updates while you watch.';

  @override
  String get featI18nTitle => 'Every string external';

  @override
  String get featI18nBody =>
      'All UI text lives in ARB files, so translating the app means editing one file, not hunting through code.';

  @override
  String get screenshotsTitle => 'Screenshots';

  @override
  String get screenshotsSubtitle => 'The real app, running on the headset.';

  @override
  String get shotGridCaption => 'The app grid inside vrhome.';

  @override
  String get shotCollectionCaption => 'Collections and groups, with counts.';

  @override
  String get shotMenuCaption => 'The tile menu on a long press.';

  @override
  String get downloadTitle => 'Download';

  @override
  String get downloadSubtitle => 'Nothing to install yet.';

  @override
  String get downloadStatusTitle => 'Not shipping yet';

  @override
  String get downloadStatusBody =>
      'Library ships with the PN2Lineage system image, and the image is still being built. When a flashable build exists it will land here first.';

  @override
  String get downloadStepsTitle => 'Want it early?';

  @override
  String get downloadStepSource =>
      'Clone the repository and build the app yourself with the Flutter toolchain.';

  @override
  String get downloadStepDocs =>
      'Read the docs for the port\'s build pipeline and current state.';

  @override
  String get downloadStepWatch => 'Watch the repository to get release news.';

  @override
  String get downloadSourceCta => 'View source on GitLab';

  @override
  String get downloadDocsCta => 'Open the docs';

  @override
  String get downloadNote =>
      'Builds are signed for the headset\'s system image. Sideloaded copies work, but the library is meant to live inside vrhome.';

  @override
  String get aboutTitle => 'About';

  @override
  String get aboutSubtitle => 'One small piece of a larger port.';

  @override
  String get aboutWhatTitle => 'What it is';

  @override
  String get aboutWhatBody =>
      'Library is the app window of vrhome, the VR home environment for the Pico Neo 2 running LineageOS 17.1. It lists every launchable app on the headset and gets out of the way.';

  @override
  String get aboutProjectTitle => 'The project';

  @override
  String get aboutProjectBody =>
      'PN2Lineage ports LineageOS 17.1 - Android 10 via a phh GSI - to the Pico Neo 2. Every piece of the port, from the GSI overlay to the VR stack, lives in its own repository under neosalsa on GitLab.';

  @override
  String get aboutTechTitle => 'How it is built';

  @override
  String get aboutTechBody =>
      'Flutter, with all decisions in pure Dart modules and platform glue kept thin. The Kotlin side only talks to PackageManager, rasterizes icons and fires intents.';

  @override
  String get aboutLicenseTitle => 'License';

  @override
  String get aboutLicenseBody =>
      'Library is free software under the GNU Affero General Public License v3. The Inter typeface bundled on this site is under the SIL Open Font License 1.1.';

  @override
  String get aboutRepoCta => 'Library repository';

  @override
  String get aboutDocsCta => 'Project docs';

  @override
  String get aboutGroupCta => 'All neosalsa repos';

  @override
  String get footerProduct => 'Library';

  @override
  String get footerProject => 'Project';

  @override
  String footerCopyright(int year) {
    return 'Copyright $year neosalsa';
  }

  @override
  String get footerLicense => 'Licensed under the AGPL-3.0';

  @override
  String get footerBuiltWith => 'Built with Flutter';

  @override
  String get faqTitle => 'FAQ';

  @override
  String get faqSubtitle => 'Short answers, no marketing.';

  @override
  String get faqQ1 => 'Is there a build I can install?';

  @override
  String get faqA1 =>
      'Not yet. Library ships inside the PN2Lineage system image, which is still being assembled. The download page tracks the current state.';

  @override
  String get faqQ2 => 'How do I try it today?';

  @override
  String get faqA2 =>
      'Clone the repository and build it with the Flutter toolchain, or watch the repo for the first release.';

  @override
  String get faqQ3 => 'What is vrhome?';

  @override
  String get faqA3 =>
      'The VR home environment Library lives inside. vrhome draws the window frames, the keyboard and the rest of the desktop around app windows like this one.';

  @override
  String get faqQ4 => 'Does it work without the headset?';

  @override
  String get faqA4 =>
      'It is a normal Flutter app and runs anywhere Android does. The controller shortcuts and the window layout only really make sense inside vrhome on the Pico Neo 2.';

  @override
  String get faqQ5 => 'Can I sideload the APK?';

  @override
  String get faqA5 =>
      'Yes, it installs like any other APK. What it cannot be alone is the home environment - that part belongs to the system image.';

  @override
  String get faqQ6 => 'Why AGPL?';

  @override
  String get faqA6 =>
      'The whole port is free software. If you ship a modified Library over a network or on a device, your users get the source too.';

  @override
  String get notFoundTitle => 'Page not found';

  @override
  String get notFoundBody => 'The page you are looking for does not exist.';

  @override
  String get notFoundCta => 'Back to Library';
}
