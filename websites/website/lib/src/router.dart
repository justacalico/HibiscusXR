import 'package:flutter/widgets.dart';
import 'package:go_router/go_router.dart';

import 'flashdocs.dart';
import 'routes.dart';
import 'apps.dart';
import 'ui/pages/about_page.dart';
import 'ui/pages/app_downloads_page.dart';
import 'ui/pages/cte_page.dart';
import 'ui/pages/downloads_page.dart';
import 'ui/pages/faq_page.dart';
import 'ui/pages/flashdocs/device_page.dart';
import 'ui/pages/flashdocs/guide_page.dart';
import 'ui/pages/flashdocs/home_page.dart';
import 'ui/pages/home_page.dart';
import 'ui/pages/not_found_page.dart';
import 'ui/pages/repositories_page.dart';
import 'ui/pages/screenshots_page.dart';
import 'ui/pages/status_page.dart';
import 'ui/shell.dart';

GoRouter buildRouter() => GoRouter(
      initialLocation: Routes.home,
      errorPageBuilder: (context, state) => CustomTransitionPage<void>(
        key: state.pageKey,
        child: const SiteShell(child: NotFoundPage()),
        transitionDuration: const Duration(milliseconds: 200),
        transitionsBuilder: (context, animation, _, child) =>
            FadeTransition(opacity: animation, child: child),
      ),
      routes: [
        ShellRoute(
          builder: (context, state, child) => SiteShell(child: child),
          routes: [
            GoRoute(
              path: Routes.home,
              pageBuilder: _fade(const HomePage()),
            ),
            GoRoute(
              path: Routes.status,
              pageBuilder: _fade(const StatusPage()),
            ),
            GoRoute(
              path: Routes.repositories,
              pageBuilder: _fade(const RepositoriesPage()),
            ),
            GoRoute(
              path: Routes.screenshots,
              pageBuilder: _fade(const ScreenshotsPage()),
            ),
            GoRoute(
              path: Routes.download,
              pageBuilder: _fade(const DownloadsPage()),
            ),
            GoRoute(
              path: Routes.cte,
              pageBuilder: _fade(const CtePage()),
            ),
            GoRoute(
              path: Routes.cteDownload,
              pageBuilder: _fade(
                  const AppDownloadsPage(app: SiteApp.cte)),
            ),
            GoRoute(
              path: Routes.hbsupDownload,
              pageBuilder: _fade(
                  const AppDownloadsPage(app: SiteApp.hbsup)),
            ),
            GoRoute(
              path: Routes.faq,
              pageBuilder: _fade(const FaqPage()),
            ),
            GoRoute(
              path: Routes.about,
              pageBuilder: _fade(const AboutPage()),
            ),
            GoRoute(
              path: Routes.flashdocs,
              pageBuilder: _fade(const FlashDocsHomePage()),
            ),
            GoRoute(
              path: '${Routes.flashdocs}/:device',
              pageBuilder: (context, state) =>
                  _flashdocsDevice(state.pathParameters['device'])(
                      context, state),
            ),
            GoRoute(
              path: '${Routes.flashdocs}/:device/:system',
              pageBuilder: (context, state) => _flashdocsGuide(
                state.pathParameters['device'],
                state.pathParameters['system'],
              )(context, state),
            ),
          ],
        ),
      ],
    );

/// Plain fade transition between pages - no hero-style pushes on a site.
CustomTransitionPage<void> Function(BuildContext, GoRouterState) _fade(
        Widget child) =>
    (context, state) => CustomTransitionPage<void>(
          key: state.pageKey,
          child: child,
          transitionDuration: const Duration(milliseconds: 200),
          transitionsBuilder: (context, animation, _, child) =>
              FadeTransition(opacity: animation, child: child),
        );

/// Page builder for /flashdocs/:device - NotFoundPage on an unknown slug.
CustomTransitionPage<void> Function(BuildContext, GoRouterState)
    _flashdocsDevice(String? slug) {
  final device = flashDocDevice(slug);
  return _fade(device == null
      ? const NotFoundPage()
      : FlashDocsDevicePage(device: device));
}

/// Page builder for /flashdocs/:device/:system - NotFoundPage when either
/// slug is unknown.
CustomTransitionPage<void> Function(BuildContext, GoRouterState)
    _flashdocsGuide(String? deviceSlug, String? systemSlug) {
  final device = flashDocDevice(deviceSlug);
  final system =
      device == null ? null : flashDocSystem(device, systemSlug);
  return _fade(device == null || system == null
      ? const NotFoundPage()
      : FlashDocsGuidePage(device: device, system: system));
}
