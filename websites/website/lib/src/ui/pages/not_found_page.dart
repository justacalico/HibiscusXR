import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../routes.dart';
import '../../theme.dart';
import '../shell.dart';
import '../widgets.dart';

class NotFoundPage extends StatelessWidget {
  const NotFoundPage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return PageBody(
      children: [
        Band(
          padding: const EdgeInsets.symmetric(vertical: 140),
          width: Layout.text,
          child: Column(
            children: [
              Text(
                l10n.notFoundTitle,
                textAlign: TextAlign.center,
                style: context.text.displayMedium!.copyWith(
                  fontSize: Layout.isMobile(context) ? 36 : 56,
                ),
              ),
              const SizedBox(height: 16),
              Text(
                l10n.notFoundBody,
                textAlign: TextAlign.center,
                style: context.text.bodyLarge!
                    .copyWith(color: context.colors.secondary),
              ),
              const SizedBox(height: 28),
              PillButton(
                label: l10n.notFoundCta,
                onPressed: () => context.go(Routes.home),
              ),
            ],
          ),
        ),
      ],
    );
  }
}
