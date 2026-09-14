import 'package:flutter/material.dart';

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
            const NavBar(),
            Expanded(child: child),
          ],
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
