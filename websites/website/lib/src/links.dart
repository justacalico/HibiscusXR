/// External URLs the site points at. Centralised so pages stay string-free.
abstract final class Links {
  static const repo = 'https://gitlab.com/neosalsa/HibiscusXR';
  static const docs = 'https://hibiscusxr-37c6a7.gitlab.io/docs/';
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
}
