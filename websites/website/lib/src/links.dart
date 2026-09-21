/// External URLs the site points at. Centralised so pages stay string-free.
abstract final class Links {
  static const group = 'https://gitlab.com/neosalsa';
  static const docs = 'https://docs-3fb651.gitlab.io/';
  static const vrhome = 'https://gitlab.com/neosalsa/applications/vrhome';
  static const library = 'https://gitlab.com/neosalsa/applications/library';
  static const out = 'https://gitlab.com/neosalsa/out';
  static const notes = 'https://gitlab.com/neosalsa/notes';
  static const issues = 'https://gitlab.com/neosalsa/issues';
  static const newIssue = 'https://gitlab.com/neosalsa/issues/-/issues/new';

  /// Dist project on GitLab - releases are public, assets never expire.
  static const distProjectId = '86495557';
  static String get releasesApi =>
      'https://gitlab.com/api/v4/projects/$distProjectId/releases';
  static String releasePage(String tag) =>
      'https://gitlab.com/neosalsa/dist/-/releases/$tag';

  /// Canonical GitLab URL for a repo path under the group.
  static String repo(String path) => '$group/$path';
}
