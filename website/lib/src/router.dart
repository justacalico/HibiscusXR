import 'package:flutter/widgets.dart';
import 'package:go_router/go_router.dart';

import 'routes.dart';
import 'ui/pages/about_page.dart';
import 'ui/pages/downloads_page.dart';
import 'ui/pages/features_page.dart';
import 'ui/pages/home_page.dart';
import 'ui/pages/not_found_page.dart';
import 'ui/pages/screenshots_page.dart';
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
              path: Routes.features,
              pageBuilder: _fade(const FeaturesPage()),
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
              path: Routes.about,
              pageBuilder: _fade(const AboutPage()),
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
