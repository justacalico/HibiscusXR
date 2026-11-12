import 'package:flutter/material.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../theme.dart';
import 'footer.dart';
import 'nav_bar.dart';

/// Frame around every page: fixed frosted nav, scrolling body,
/// directory footer at the bottom of the scroll.
class SiteShell extends StatelessWidget {
  const SiteShell({super.key, required this.child});

  final Widget child;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SelectionArea(
        child: Column(
          children: [
            const SiteBanner(),
            const NavBar(),
            Expanded(child: child),
          ],
        ),
      ),
    );
  }
}

/// Announcement strip pinned above the nav bar.
class SiteBanner extends StatelessWidget {
  const SiteBanner({super.key});

  @override
  Widget build(BuildContext context) {
    return ColoredBox(
      color: context.colors.primary,
      child: Padding(
        padding: Layout.pagePadding(context)
            .add(const EdgeInsets.symmetric(vertical: 8)),
        child: Center(
          child: Text(
            AppLocalizations.of(context).siteBanner,
            textAlign: TextAlign.center,
            style: context.text.labelSmall!.copyWith(
              color: context.colors.onPrimary,
              fontSize: 13,
            ),
          ),
        ),
      ),
    );
  }
}

/// Scroll container for a page: page sections, then the footer.
class PageBody extends StatelessWidget {
  const PageBody({super.key, required this.children});

  final List<Widget> children;

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          ...children,
          const SiteFooter(),
        ],
      ),
    );
  }
}
