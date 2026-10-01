/// All in-site paths. Pure constants so tests can walk the whole table.
abstract final class Routes {
  static const home = '/';
  static const status = '/status';
  static const repositories = '/repositories';
  static const screenshots = '/screenshots';
  static const download = '/download';
  static const cte = '/cte';
  static const faq = '/faq';
  static const about = '/about';
  static const flashdocs = '/flashdocs';

  /// Device page under the flashing docs, e.g. /flashdocs/pico-neo-2.
  static String flashdocsDevice(String device) => '$flashdocs/$device';

  /// One host-OS guide under a device, e.g. /flashdocs/pico-neo-2/linux.
  static String flashdocsGuide(String device, String system) =>
      '$flashdocs/$device/$system';
}

/// One nav destination: where it goes and which l10n getter names it.
class Destination {
  const Destination(this.path, this.label);

  /// In-site path, or an absolute URL for external links.
  final String path;
  final String Function(dynamic l10n) label;

  bool get isExternal => path.startsWith('http');
}
