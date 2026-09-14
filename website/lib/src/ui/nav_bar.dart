import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../links.dart';
import '../routes.dart';
import '../settings.dart';
import '../theme.dart';
import 'widgets.dart';

/// Fixed top bar: wordmark left, links centre, theme + language + CTA right.
/// Collapses to a hamburger overlay under [Layout.mobileBreak].
class NavBar extends StatelessWidget implements PreferredSizeWidget {
  const NavBar({super.key});

  @override
  Size get preferredSize => const Size.fromHeight(Layout.navHeight);

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Frosted(
      child: Container(
        height: Layout.navHeight,
        decoration: BoxDecoration(
          border: Border(
            bottom: BorderSide(color: context.colors.outline, width: 0.5),
          ),
        ),
        padding: Layout.pagePadding(context),
        child: Center(
          child: ConstrainedBox(
            constraints: const BoxConstraints(maxWidth: Layout.content),
            child: Layout.isMobile(context)
                ? _MobileBar(l10n: l10n)
                : _DesktopBar(l10n: l10n),
          ),
        ),
      ),
    );
  }
}

class _Wordmark extends StatelessWidget {
  const _Wordmark();

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return InkWell(
      onTap: () => context.go(Routes.home),
      mouseCursor: SystemMouseCursors.click,
      child: Padding(
        padding: const EdgeInsets.symmetric(vertical: 8),
        child: Text(
          l10n.appTitle,
          style: context.text.titleMedium!.copyWith(
            fontFamily: 'InterDisplay',
            fontSize: 17,
            letterSpacing: -0.3,
          ),
        ),
      ),
    );
  }
}

List<Destination> _destinations(AppLocalizations l10n) => [
      Destination(Routes.features, (l) => l.navFeatures),
      Destination(Routes.screenshots, (l) => l.navScreenshots),
      Destination(Routes.faq, (l) => l.navFaq),
      Destination(Routes.about, (l) => l.navAbout),
      Destination(Links.docs, (l) => l.navDocs),
    ];

void _open(Destination d, BuildContext context) {
  if (d.isExternal) {
    launchUrl(Uri.parse(d.path));
  } else {
    context.go(d.path);
  }
}

/// Current path without requiring the context to sit under a RouteBase,
/// so it also works inside dialogs and the router error page.
String _currentPath(BuildContext context) =>
    GoRouter.maybeOf(context)
        ?.routerDelegate
        .currentConfiguration
        .uri
        .path ??
    '';

class _NavLink extends StatefulWidget {
  const _NavLink({required this.destination, required this.label});

  final Destination destination;
  final String label;

  @override
  State<_NavLink> createState() => _NavLinkState();
}

class _NavLinkState extends State<_NavLink> {
  bool _hover = false;
  bool _focus = false;

  @override
  Widget build(BuildContext context) {
    final current = _currentPath(context) == widget.destination.path;
    final color = current
        ? context.colors.onSurface
        : context.colors.secondary;
    final active = _hover || _focus;
    return FocusableActionDetector(
      onShowHoverHighlight: (v) => setState(() => _hover = v),
      onShowFocusHighlight: (v) => setState(() => _focus = v),
      actions: activateActions(() => _open(widget.destination, context)),
      mouseCursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: () => _open(widget.destination, context),
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
          child: Text(
            widget.label,
            style: context.text.labelSmall!.copyWith(
              color: active ? context.colors.onSurface : color,
              fontWeight: current ? FontWeight.w500 : FontWeight.w400,
              decoration:
                  _focus ? TextDecoration.underline : TextDecoration.none,
            ),
          ),
        ),
      ),
    );
  }
}

class _DesktopBar extends StatelessWidget {
  const _DesktopBar({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        const _Wordmark(),
        const Spacer(),
        for (final d in _destinations(l10n))
          _NavLink(destination: d, label: d.label(l10n)),
        const Spacer(),
        const _ThemeMenu(),
        const SizedBox(width: 4),
        const _LanguageMenu(),
        const SizedBox(width: 12),
        PillButton(
          label: l10n.navDownload,
          small: true,
          onPressed: () => context.go(Routes.download),
        ),
      ],
    );
  }
}

class _MobileBar extends StatelessWidget {
  const _MobileBar({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        const _Wordmark(),
        const Spacer(),
        const _ThemeMenu(),
        const _LanguageMenu(),
        IconButton(
          tooltip: l10n.navMenu,
          icon: const Icon(Icons.menu, size: 20),
          color: context.colors.onSurface,
          onPressed: () => showGeneralDialog(
            context: context,
            barrierDismissible: true,
            barrierLabel: l10n.navClose,
            barrierColor: Colors.transparent,
            transitionDuration: const Duration(milliseconds: 200),
            pageBuilder: (context, _, _) => const _MobileMenu(),
          ),
        ),
      ],
    );
  }
}

/// Full-screen frosted overlay menu, Apple-mobile style.
class _MobileMenu extends StatelessWidget {
  const _MobileMenu();

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final current = _currentPath(context);
    final destinations = [
      Destination(Routes.home, (l) => l.appTitle),
      ..._destinations(l10n),
      Destination(Routes.download, (l) => l.navDownload),
    ];
    return Material(
      type: MaterialType.transparency,
      child: Frosted(
        opacity: 0.94,
        child: SafeArea(
          child: Padding(
            padding: Layout.pagePadding(context),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                SizedBox(
                  height: Layout.navHeight,
                  child: Row(
                    children: [
                      const _Wordmark(),
                      const Spacer(),
                      IconButton(
                        tooltip: l10n.navClose,
                        icon: const Icon(Icons.close, size: 20),
                        color: context.colors.onSurface,
                        onPressed: () => Navigator.of(context).pop(),
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 24),
                for (final d in destinations)
                  _MobileMenuItem(
                    destination: d,
                    label: d.label(l10n),
                    current: current == d.path,
                  ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _MobileMenuItem extends StatelessWidget {
  const _MobileMenuItem({
    required this.destination,
    required this.label,
    required this.current,
  });

  final Destination destination;
  final String label;
  final bool current;

  @override
  Widget build(BuildContext context) {
    return InkWell(
      onTap: () {
        Navigator.of(context).pop();
        _open(destination, context);
      },
      child: Container(
        width: double.infinity,
        padding: const EdgeInsets.symmetric(vertical: 14),
        decoration: BoxDecoration(
          border: Border(
            bottom: BorderSide(color: context.colors.outline, width: 0.5),
          ),
        ),
        child: Text(
          label,
          style: context.text.titleLarge!.copyWith(
            fontSize: 22,
            color: current
                ? context.colors.primary
                : context.colors.onSurface,
          ),
        ),
      ),
    );
  }
}

class _ThemeMenu extends StatelessWidget {
  const _ThemeMenu();

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final settings = AppSettingsScope.of(context);
    return PopupMenuButton<ThemeMode>(
      tooltip: l10n.themeLabel,
      initialValue: settings.themeMode,
      onSelected: settings.setThemeMode,
      color: context.colors.surface,
      itemBuilder: (context) => [
        for (final (mode, label) in [
          (ThemeMode.system, l10n.themeSystem),
          (ThemeMode.light, l10n.themeLight),
          (ThemeMode.dark, l10n.themeDark),
        ])
          PopupMenuItem(
            value: mode,
            child: Row(
              children: [
                Expanded(
                  child: Text(
                    label,
                    style: context.text.bodyMedium!
                        .copyWith(color: context.colors.onSurface),
                  ),
                ),
                if (settings.themeMode == mode)
                  Icon(Icons.check, size: 16, color: context.colors.primary),
              ],
            ),
          ),
      ],
      icon: Icon(
        switch (settings.themeMode) {
          ThemeMode.light => Icons.light_mode_outlined,
          ThemeMode.dark => Icons.dark_mode_outlined,
          ThemeMode.system => Icons.contrast,
        },
        size: 18,
        color: context.colors.secondary,
      ),
    );
  }
}

class _LanguageMenu extends StatelessWidget {
  const _LanguageMenu();

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final settings = AppSettingsScope.of(context);
    return PopupMenuButton<String>(
      tooltip: l10n.languageLabel,
      initialValue: settings.locale?.languageCode ?? 'system',
      onSelected: (code) =>
          settings.setLocale(code == 'system' ? null : Locale(code)),
      color: context.colors.surface,
      itemBuilder: (context) => [
        for (final (code, label) in [
          ('system', l10n.themeSystem),
          ('en', 'English'),
          ('zh', '中文'),
        ])
          PopupMenuItem(
            value: code,
            child: Row(
              children: [
                Expanded(
                  child: Text(
                    label,
                    style: context.text.bodyMedium!
                        .copyWith(color: context.colors.onSurface),
                  ),
                ),
                if ((settings.locale?.languageCode ?? 'system') == code)
                  Icon(Icons.check, size: 16, color: context.colors.primary),
              ],
            ),
          ),
      ],
      icon: Icon(Icons.language, size: 18, color: context.colors.secondary),
    );
  }
}
