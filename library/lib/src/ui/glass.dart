import 'dart:ui';

import 'package:flutter/material.dart';

import 'theme.dart';

/// Frosted-glass surface: translucent fill, hairline stroke, soft shadow.
/// [blur] enables a real BackdropFilter - use sparingly, it is GPU-costly
/// per instance, so tiles use [blur]=false and only chrome blurs.
class Glass extends StatelessWidget {
  const Glass({
    super.key,
    required this.child,
    this.radius = LibraryTheme.pillRadius,
    this.padding,
    this.blur = false,
    this.fill = LibraryTheme.glassFill,
    this.stroke = LibraryTheme.glassStroke,
    this.circle = false,
  });

  final Widget child;
  final double radius;
  final EdgeInsetsGeometry? padding;
  final bool blur;
  final Color fill;
  final Color stroke;
  final bool circle;

  @override
  Widget build(BuildContext context) {
    final decoration = BoxDecoration(
      color: fill,
      shape: circle ? BoxShape.circle : BoxShape.rectangle,
      borderRadius:
          circle ? null : BorderRadius.circular(radius),
      border: Border.all(color: stroke, width: 1),
      boxShadow: const [
        BoxShadow(
          color: Color(0x33000000),
          blurRadius: 20,
          offset: Offset(0, 8),
        ),
      ],
    );

    Widget content = Container(
      decoration: decoration,
      padding: padding,
      child: child,
    );

    if (blur) {
      content = ClipRRect(
        borderRadius:
            circle ? BorderRadius.circular(999) : BorderRadius.circular(radius),
        child: BackdropFilter(
          filter: ImageFilter.blur(sigmaX: 24, sigmaY: 24),
          child: content,
        ),
      );
    }
    return content;
  }
}
