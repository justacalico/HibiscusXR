/// External URLs the site points at. Centralised so pages stay string-free.
abstract final class Links {
  static const repo = 'https://gitlab.com/neosalsa/HibiscusXR';
  static const vrhome = '$repo/-/tree/main/applications/vrhome';
  static const library = '$repo/-/tree/main/applications/library';
  static const out = '$repo/-/tree/main/system/out';
  static const notes = '$repo/-/tree/main/research/notes';
  static const issues = '$repo/-/issues';
  static const newIssue = '$repo/-/issues/new';

  /// Releases live on the monorepo - assets are package-registry backed and
  /// never expire.
  static const projectId = '86728484';
  static String get releasesApi =>
      'https://gitlab.com/api/v4/projects/$projectId/releases';
  static String releasePage(String tag) => '$repo/-/releases/$tag';

  /// Canonical GitLab URL for a directory inside the monorepo.
  static String tree(String path) => '$repo/-/tree/main/$path';

  /// A file inside a generic package on the monorepo - direct download.
  static String packageFile(String package, String version, String file) =>
      'https://gitlab.com/api/v4/projects/$projectId/packages/generic'
      '/$package/$version/$file';

  /// Magisk-patched boot image for the Pico Neo 2 - roots stock so the
  /// backup step gets `adb root`.
  static String get neo2RootBoot => packageFile(
      'neo2-magisk-boot', '4.1.3', 'magisk_patched_pico_neo_2_boot.img');
}
