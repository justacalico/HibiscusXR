
/// Desktop apps that ship out of the monorepo's app release lanes.
enum SiteApp { cte, hbsup }

abstract final class SiteApps {
  /// Display name.
  static String label(SiteApp app) =>
      switch (app) { SiteApp.cte => 'HCTE', SiteApp.hbsup => 'HBSUP' };

  /// Release tag prefix for the app's lane (`cte-v*`, `hbsup-v*`).
  static String tagPrefix(SiteApp app) => '${app.name}-';

  /// Source directory inside the monorepo.
  static String treePath(SiteApp app) => 'applications/${app.name}';
}
