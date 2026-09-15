import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../../links.dart';
import '../../theme.dart';
import '../shell.dart';
import '../widgets.dart';

class AboutPage extends StatelessWidget {
  const AboutPage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return PageBody(
      children: [
        PageHead(title: l10n.aboutTitle, subtitle: l10n.aboutSubtitle),
        Band(
          width: Layout.text,
          padding: const EdgeInsets.symmetric(vertical: 72),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _Block(title: l10n.aboutWhatTitle, body: l10n.aboutWhatBody),
              _Block(title: l10n.aboutHowTitle, body: l10n.aboutHowBody),
              _Block(title: l10n.aboutGroupTitle, body: l10n.aboutGroupBody),
              _Block(
                  title: l10n.aboutLicenseTitle, body: l10n.aboutLicenseBody),
              const SizedBox(height: 12),
              Wrap(
                spacing: 28,
                runSpacing: 12,
                children: [
                  ChevronLink(
                    label: l10n.aboutRepoCta,
                    onPressed: () => launchUrl(Uri.parse(Links.group)),
                  ),
                  ChevronLink(
                    label: l10n.aboutDocsCta,
                    onPressed: () => launchUrl(Uri.parse(Links.docs)),
                  ),
                  ChevronLink(
                    label: l10n.aboutNotesCta,
                    onPressed: () => launchUrl(Uri.parse(Links.notes)),
                  ),
                ],
              ),
            ],
          ),
        ),
      ],
    );
  }
}

class _Block extends StatelessWidget {
  const _Block({required this.title, required this.body});

  final String title;
  final String body;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 40),
      child: Reveal(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: context.text.titleLarge),
            const SizedBox(height: 10),
            Text(
              body,
              style: context.text.bodyMedium!.copyWith(fontSize: 17),
            ),
          ],
        ),
      ),
    );
  }
}
