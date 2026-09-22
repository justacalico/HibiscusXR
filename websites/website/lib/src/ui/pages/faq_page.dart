import 'package:flutter/material.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import '../../theme.dart';
import '../shell.dart';
import '../widgets.dart';

class FaqPage extends StatelessWidget {
  const FaqPage({super.key});

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final pairs = [
      (l10n.faqQ1, l10n.faqA1),
      (l10n.faqQ2, l10n.faqA2),
      (l10n.faqQ3, l10n.faqA3),
      (l10n.faqQ4, l10n.faqA4),
      (l10n.faqQ5, l10n.faqA5),
      (l10n.faqQ6, l10n.faqA6),
    ];
    return PageBody(
      children: [
        PageHead(title: l10n.faqTitle, subtitle: l10n.faqSubtitle),
        Band(
          width: Layout.text + 96,
          padding: const EdgeInsets.symmetric(vertical: 72),
          child: Column(
            children: [
              for (var i = 0; i < pairs.length; i++)
                Reveal(
                  delay: Duration(milliseconds: i * 40),
                  child: _FaqRow(question: pairs[i].$1, answer: pairs[i].$2),
                ),
            ],
          ),
        ),
      ],
    );
  }
}

class _FaqRow extends StatefulWidget {
  const _FaqRow({required this.question, required this.answer});

  final String question;
  final String answer;

  @override
  State<_FaqRow> createState() => _FaqRowState();
}

class _FaqRowState extends State<_FaqRow> {
  bool _open = false;
  bool _hover = false;

  @override
  Widget build(BuildContext context) {
    void toggle() => setState(() => _open = !_open);
    return FocusableActionDetector(
      onShowHoverHighlight: (v) => setState(() => _hover = v),
      actions: activateActions(toggle),
      mouseCursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: toggle,
        child: Container(
          decoration: BoxDecoration(
            border: Border(
              bottom: BorderSide(color: context.colors.outline, width: 0.5),
            ),
          ),
          padding: const EdgeInsets.symmetric(vertical: 22),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  Expanded(
                    child: Text(
                      widget.question,
                      style: context.text.titleMedium!.copyWith(
                        color: _hover
                            ? context.colors.primary
                            : context.colors.onSurface,
                      ),
                    ),
                  ),
                  AnimatedRotation(
                    turns: _open ? 0.25 : 0,
                    duration: const Duration(milliseconds: 200),
                    child: Icon(
                      Icons.chevron_right,
                      size: 20,
                      color: context.colors.secondary,
                    ),
                  ),
                ],
              ),
              AnimatedSize(
                duration: const Duration(milliseconds: 250),
                curve: Curves.easeInOut,
                alignment: Alignment.topCenter,
                child: _open
                    ? Padding(
                        padding: const EdgeInsets.only(top: 12),
                        child: Align(
                          alignment: Alignment.centerLeft,
                          child: Text(
                            widget.answer,
                            style: context.text.bodyMedium,
                          ),
                        ),
                      )
                    : const SizedBox(width: double.infinity, height: 0),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
