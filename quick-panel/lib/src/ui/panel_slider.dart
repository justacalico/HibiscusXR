import 'package:flutter/material.dart';

import 'theme.dart';

/// Quest-style slider: a pill track, accent fill, and a round knob with
/// the icon inside flush with the fill edge. Drag or tap to set.
class PanelSlider extends StatelessWidget {
  const PanelSlider({
    super.key,
    required this.value,
    required this.icon,
    required this.label,
    required this.onChanged,
  });

  final double value;
  final IconData icon;
  final String label;
  final ValueChanged<double> onChanged;

  static const height = 44.0;
  static const knob = height;

  void _setFromDx(double dx, double width) {
    onChanged((dx / width).clamp(0.0, 1.0));
  }

  @override
  Widget build(BuildContext context) {
    return Semantics(
      label: label,
      slider: true,
      value: '${(value * 100).round()}%',
      child: LayoutBuilder(
        builder: (context, constraints) {
          final w = constraints.maxWidth;
          final knobX = value.clamp(0.0, 1.0) * (w - knob);
          return GestureDetector(
            behavior: HitTestBehavior.opaque,
            onTapDown: (d) => _setFromDx(d.localPosition.dx, w),
            onHorizontalDragUpdate: (d) => _setFromDx(d.localPosition.dx, w),
            child: SizedBox(
              height: height,
              child: Stack(
                alignment: Alignment.centerLeft,
                children: [
                  Container(
                    height: height,
                    decoration: BoxDecoration(
                      color: PanelTheme.surface,
                      borderRadius: BorderRadius.circular(height / 2),
                    ),
                  ),
                  ClipRRect(
                    borderRadius: BorderRadius.circular(height / 2),
                    child: Container(
                      height: height,
                      width: knobX + knob / 2,
                      color: PanelTheme.accent,
                    ),
                  ),
                  Positioned(
                    left: knobX,
                    child: Container(
                      width: knob,
                      height: knob,
                      decoration: const BoxDecoration(
                        color: PanelTheme.panel,
                        shape: BoxShape.circle,
                      ),
                      child: Icon(icon, size: 20, color: PanelTheme.textPrimary),
                    ),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    );
  }
}
