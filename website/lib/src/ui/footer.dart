import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../links.dart';
import '../routes.dart';
import '../theme.dart';
import 'widgets.dart';

class _Link {
  const _Link(this.label, this.target);
  final String label;
  final String target;
  bool get external => target.startsWith('http');
}

/// Apple-style directory footer: link columns over a legal row.
class SiteFooter extends StatelessWidget {
  const SiteFooter({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final product = <_Link>[
      _Link(l10n.navFeatures, Routes.features),
      _Link(l10n.navScreenshots, Routes.screenshots),
      _Link(l10n.navFaq, Routes.faq),
      _Link(l10n.navDownload, Routes.download),
    ];
    final project = <_Link>[
      _Link(l10n.navAbout, Routes.about),
      _Link(l10n.navDocs, Links.docs),
      _Link(l10n.aboutGroupCta, Links.group),
      _Link(l10n.aboutRepoCta, Links.repo),
    ];
    final year = DateTime.now().year;

    return Container(
      width: double.infinity,
      decoration: BoxDecoration(
        color: context.colors.surfaceContainerHighest,
        border: Border(
          top: BorderSide(color: context.colors.outline, width: 0.5),
        ),
      ),
      padding: Layout.pagePadding(context).add(
        const EdgeInsets.symmetric(vertical: 32),
      ),
      child: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: Layout.content),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Wrap(
                spacing: 64,
                runSpacing: 24,
                children: [
                  _FooterColumn(title: l10n.footerProduct, links: product),
                  _FooterColumn(title: l10n.footerProject, links: project),
                ],
              ),
              const SizedBox(height: 24),
              Divider(color: context.colors.outline, height: 1),
              const SizedBox(height: 16),
              Wrap(
                spacing: 24,
                runSpacing: 8,
                alignment: WrapAlignment.spaceBetween,
                crossAxisAlignment: WrapCrossAlignment.center,
                children: [
                  Text(
                    l10n.footerCopyright(year),
                    style: context.text.labelSmall,
                  ),
                  Text(
                    '${l10n.footerLicense} · ${l10n.footerBuiltWith}',
                    style: context.text.labelSmall,
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _FooterColumn extends StatelessWidget {
  const _FooterColumn({required this.title, required this.links});

  final String title;
  final List<_Link> links;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: 200,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            title,
            style: context.text.labelSmall!.copyWith(
              color: context.colors.onSurface,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(height: 10),
          for (final link in links) _FooterLink(link: link),
        ],
      ),
    );
  }
}

class _FooterLink extends StatefulWidget {
  const _FooterLink({required this.link});

  final _Link link;

  @override
  State<_FooterLink> createState() => _FooterLinkState();
}

class _FooterLinkState extends State<_FooterLink> {
  bool _hover = false;
  bool _focus = false;

  @override
  Widget build(BuildContext context) {
    final underline = _hover || _focus;
    return FocusableActionDetector(
      onShowHoverHighlight: (v) => setState(() => _hover = v),
      onShowFocusHighlight: (v) => setState(() => _focus = v),
      actions: activateActions(() => widget.link.external
          ? launchUrl(Uri.parse(widget.link.target))
          : context.go(widget.link.target)),
      mouseCursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: () => widget.link.external
            ? launchUrl(Uri.parse(widget.link.target))
            : context.go(widget.link.target),
        child: Padding(
          padding: const EdgeInsets.symmetric(vertical: 5),
          child: Text(
            widget.link.label,
            style: context.text.labelSmall!.copyWith(
              decoration:
                  underline ? TextDecoration.underline : TextDecoration.none,
            ),
          ),
        ),
      ),
    );
  }
}
