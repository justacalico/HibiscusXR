/// All in-site paths. Pure constants so tests can walk the whole table.
abstract final class Routes {
  static const home = '/';
  static const features = '/features';
  static const screenshots = '/screenshots';
  static const download = '/download';
  static const about = '/about';
}

/// One nav destination: where it goes and which l10n getter names it.
class Destination {
  const Destination(this.path, this.label);

  /// In-site path, or an absolute URL for external links.
  final String path;
  final String Function(dynamic l10n) label;

  bool get isExternal => path.startsWith('http');
}
