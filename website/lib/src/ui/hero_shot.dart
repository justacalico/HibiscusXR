import 'package:flutter/material.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../theme.dart';

/// The hero image: the Neosalsa mark on a dark stage with an accent glow
/// and a floor shadow. Painted instead of a screenshot so it stays sharp
/// at any size and matches the site accent.
class HeroShot extends StatelessWidget {
  const HeroShot({super.key});

  static const double aspect = 1.6;

  @override
  Widget build(BuildContext context) {
    return Semantics(
      image: true,
      label: AppLocalizations.of(context).heroShotCaption,
      child: const ExcludeSemantics(
        child: AspectRatio(
          aspectRatio: aspect,
          child: CustomPaint(painter: NeosalsaMarkPainter()),
        ),
      ),
    );
  }
}

/// Draws the brand mark - the headset glyph inside its rounded square -
/// scaled to fit whatever width the slot gives it. All geometry is
/// authored on a 880x550 canvas and normalised by the paint size.
class NeosalsaMarkPainter extends CustomPainter {
  const NeosalsaMarkPainter();

  @override
  void paint(Canvas canvas, Size size) {
    canvas.scale(size.width / 880, size.height / 550);
    final stage = Paint()..isAntiAlias = true;

    canvas.drawRect(
      const Rect.fromLTWH(0, 0, 880, 550),
      stage..color = AppColors.shotSurface,
    );

    canvas.drawCircle(
      const Offset(440, 230),
      340,
      Paint()
        ..shader = RadialGradient(
          colors: [
            AppColors.accentOnDark.withValues(alpha: 0.16),
            AppColors.accentOnDark.withValues(alpha: 0),
          ],
        ).createShader(
            Rect.fromCircle(center: const Offset(440, 230), radius: 340)),
    );

    canvas.drawOval(
      Rect.fromCenter(center: const Offset(440, 476), width: 380, height: 42),
      Paint()
        ..color = AppColors.darkPaper.withValues(alpha: 0.55)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 22),
    );

    final icon = RRect.fromRectAndRadius(
      Rect.fromCenter(center: const Offset(440, 262), width: 340, height: 340),
      const Radius.circular(62),
    );
    canvas.drawRRect(icon, stage..color = AppColors.shotIcon);
    canvas.drawRRect(
      icon,
      Paint()
        ..isAntiAlias = true
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1.5
        ..color = AppColors.shotDisc.withValues(alpha: 0.08),
    );

    const barCenter = Offset(440, 251);
    canvas.drawRRect(
      RRect.fromRectAndRadius(
        Rect.fromCenter(center: barCenter, width: 196, height: 86),
        const Radius.circular(37),
      ),
      stage..color = AppColors.shotDisc,
    );
    canvas.drawCircle(const Offset(403, 249), 19.5,
        stage..color = AppColors.shotIcon);
    canvas.drawCircle(const Offset(477, 249), 19.5,
        stage..color = AppColors.accentOnDark);
    canvas.drawCircle(const Offset(437, 292), 25,
        stage..color = AppColors.shotIcon);
  }

  @override
  bool shouldRepaint(NeosalsaMarkPainter old) => false;
}
