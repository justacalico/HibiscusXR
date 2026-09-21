import 'package:flutter/material.dart';

import '../battery.dart';
import 'theme.dart';

/// A real battery gauge: rounded outline, fill proportional to the
/// charge level, tip on the right. Tinted by [BatteryTint] bands.
class BatteryIcon extends StatelessWidget {
  const BatteryIcon({super.key, required this.level});

  final int level;

  static Color colorFor(BatteryTint tint) {
    switch (tint) {
      case BatteryTint.good:
        return const Color(0xFF3DD68C);
      case BatteryTint.normal:
        return PanelTheme.textPrimary;
      case BatteryTint.warn:
        return const Color(0xFFF5C542);
      case BatteryTint.critical:
        return const Color(0xFFFF5E5E);
    }
  }

  BatteryTint get tint => batteryTintFor(level);

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: const Size(24, 14),
      painter: _BatteryPainter(level: level / 100, color: colorFor(tint)),
    );
  }
}

class _BatteryPainter extends CustomPainter {
  const _BatteryPainter({required this.level, required this.color});

  final double level;
  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    const tipW = 2.0;
    const stroke = 1.6;
    const pad = 2.4;
    final bodyW = size.width - tipW - 1.5;

    final outline = Paint()
      ..color = color
      ..style = PaintingStyle.stroke
      ..strokeWidth = stroke;
    final body = RRect.fromRectAndRadius(
      Rect.fromLTWH(0, 0, bodyW, size.height),
      const Radius.circular(4),
    );
    canvas.drawRRect(body, outline);

    // fill proportional to charge, inset inside the outline
    final fillW = (bodyW - pad * 2) * level.clamp(0.0, 1.0);
    if (fillW > 0) {
      canvas.drawRRect(
        RRect.fromRectAndRadius(
          Rect.fromLTWH(pad, pad, fillW, size.height - pad * 2),
          const Radius.circular(2),
        ),
        Paint()..color = color,
      );
    }

    // tip nub
    canvas.drawRRect(
      RRect.fromRectAndRadius(
        Rect.fromLTWH(bodyW + 0.5, size.height / 2 - 2.5, tipW, 5),
        const Radius.circular(1),
      ),
      Paint()..color = color,
    );
  }

  @override
  bool shouldRepaint(_BatteryPainter old) =>
      old.level != level || old.color != color;
}
