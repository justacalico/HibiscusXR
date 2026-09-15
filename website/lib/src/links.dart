/// External URLs the site points at. Centralised so pages stay string-free.
abstract final class Links {
  static const group = 'https://gitlab.com/neosalsa';
  static const docs = 'https://gitlab.com/neosalsa/websites/docs';
  static const vrhome = 'https://gitlab.com/neosalsa/applications/vrhome';
  static const library = 'https://gitlab.com/neosalsa/applications/library';
  static const out = 'https://gitlab.com/neosalsa/out';
  static const notes = 'https://gitlab.com/neosalsa/notes';

  /// Canonical GitLab URL for a repo path under the group.
  static String repo(String path) => '$group/$path';
}
