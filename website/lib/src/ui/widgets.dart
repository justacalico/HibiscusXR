import 'dart:ui' show ImageFilter;

import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart' show RenderAbstractViewport;

import '../theme.dart';

/// Frosted surface used by the nav bar and the mobile menu.
class Frosted extends StatelessWidget {
  const Frosted({super.key, required this.child, this.opacity = 0.72});

  final Widget child;
  final double opacity;

  @override
  Widget build(BuildContext context) {
    final base = context.colors.surface;
    return ClipRect(
      child: BackdropFilter(
        filter: ImageFilter.blur(sigmaX: 20, sigmaY: 20),
        child: ColoredBox(
          color: base.withValues(alpha: opacity),
          child: child,
        ),
      ),
    );
  }
}

/// Apple's blue pill button.
class PillButton extends StatefulWidget {
  const PillButton({
    super.key,
    required this.label,
    required this.onPressed,
    this.small = false,
  });

  final String label;
  final VoidCallback onPressed;
  final bool small;

  @override
  State<PillButton> createState() => _PillButtonState();
}

class _PillButtonState extends State<PillButton> {
  bool _hover = false;
  bool _focus = false;

  @override
  Widget build(BuildContext context) {
    final accent = context.colors.primary;
    final bg = _hover
        ? Color.lerp(accent, Colors.white, 0.08)!
        : accent;
    return FocusableActionDetector(
      onShowHoverHighlight: (v) => setState(() => _hover = v),
      onShowFocusHighlight: (v) => setState(() => _focus = v),
      mouseCursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: widget.onPressed,
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 150),
          curve: Curves.easeOut,
          padding: EdgeInsets.symmetric(
            horizontal: widget.small ? 16 : 22,
            vertical: widget.small ? 8 : 12,
          ),
          decoration: BoxDecoration(
            color: bg,
            borderRadius: BorderRadius.circular(980),
            border: _focus
                ? Border.all(color: context.colors.onSurface, width: 2)
                : null,
          ),
          child: Text(
            widget.label,
            style: context.text.labelLarge!.copyWith(
              color: Colors.white,
              fontSize: widget.small ? 15 : 17,
            ),
          ),
        ),
      ),
    );
  }
}

/// "Learn more >" accent link, Apple's inline CTA voice.
class ChevronLink extends StatefulWidget {
  const ChevronLink({
    super.key,
    required this.label,
    required this.onPressed,
    this.large = true,
  });

  final String label;
  final VoidCallback onPressed;
  final bool large;

  @override
  State<ChevronLink> createState() => _ChevronLinkState();
}

class _ChevronLinkState extends State<ChevronLink> {
  bool _hover = false;

  @override
  Widget build(BuildContext context) {
    final color = context.colors.primary;
    final size = widget.large ? 19.0 : 15.0;
    return FocusableActionDetector(
      onShowHoverHighlight: (v) => setState(() => _hover = v),
      mouseCursor: SystemMouseCursors.click,
      child: GestureDetector(
        onTap: widget.onPressed,
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              widget.label,
              style: context.text.labelLarge!.copyWith(
                color: color,
                fontSize: size,
                decoration:
                    _hover ? TextDecoration.underline : TextDecoration.none,
                decorationColor: color,
              ),
            ),
            Icon(Icons.chevron_right, color: color, size: size + 2),
          ],
        ),
      ),
    );
  }
}

/// Small uppercase eyebrow line above a section title.
class Eyebrow extends StatelessWidget {
  const Eyebrow(this.label, {super.key, this.center = false});

  final String label;
  final bool center;

  @override
  Widget build(BuildContext context) {
    return Text(
      label.toUpperCase(),
      textAlign: center ? TextAlign.center : TextAlign.start,
      style: context.text.labelSmall!.copyWith(
        letterSpacing: 1.4,
        fontWeight: FontWeight.w600,
        color: context.colors.secondary,
      ),
    );
  }
}

/// A full-width horizontal band with centred, width-capped content.
class Band extends StatelessWidget {
  const Band({
    super.key,
    required this.child,
    this.color,
    this.padding = const EdgeInsets.symmetric(vertical: 96),
    this.width = Layout.content,
  });

  final Widget child;
  final Color? color;
  final EdgeInsets padding;
  final double width;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      color: color,
      padding: Layout.pagePadding(context).add(padding),
      child: Center(
        child: ConstrainedBox(
          constraints: BoxConstraints(maxWidth: width),
          child: child,
        ),
      ),
    );
  }
}

/// Fades and lifts its child into place the first time it scrolls
/// into view. Collapses to a plain child when animations are off.
class Reveal extends StatefulWidget {
  const Reveal({super.key, required this.child, this.delay = Duration.zero});

  final Widget child;
  final Duration delay;

  @override
  State<Reveal> createState() => _RevealState();
}

class _RevealState extends State<Reveal> with SingleTickerProviderStateMixin {
  late final AnimationController _controller = AnimationController(
    vsync: this,
    duration: const Duration(milliseconds: 700),
  );
  ScrollPosition? _position;
  bool _shown = false;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    _position?.removeListener(_check);
    _position = Scrollable.maybeOf(context)?.position;
    _position?.addListener(_check);
    WidgetsBinding.instance.addPostFrameCallback((_) => _check());
  }

  @override
  void dispose() {
    _position?.removeListener(_check);
    _controller.dispose();
    super.dispose();
  }

  void _check() {
    if (_shown || !mounted) return;
    if (MediaQuery.maybeOf(context) == null) return;
    final box = context.findRenderObject();
    if (box is! RenderBox || !box.hasSize) return;
    final viewport = RenderAbstractViewport.maybeOf(box);
    if (viewport == null) return;
    final reveal = viewport.getOffsetToReveal(box, 0).offset;
    final scroll = _position?.pixels ?? 0;
    final screen = MediaQuery.sizeOf(context).height;
    if (reveal - scroll < screen * 0.9) {
      _shown = true;
      _position?.removeListener(_check);
      if (MediaQuery.of(context).disableAnimations) {
        _controller.value = 1;
      } else {
        Future.delayed(widget.delay, () {
          if (mounted) _controller.forward();
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _controller,
      builder: (context, child) {
        final t = Curves.easeOut.transform(_controller.value);
        return Opacity(
          opacity: t,
          child: Transform.translate(
            offset: Offset(0, 24 * (1 - t)),
            child: child,
          ),
        );
      },
      child: widget.child,
    );
  }
}

/// A screenshot with a hairline border and a caption under it.
class ShotCard extends StatelessWidget {
  const ShotCard({
    super.key,
    required this.asset,
    required this.caption,
    this.height,
  });

  final String asset;
  final String caption;
  final double? height;

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.center,
      children: [
        Container(
          height: height,
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(18),
            border: Border.all(color: context.colors.outline, width: 1),
          ),
          clipBehavior: Clip.antiAlias,
          child: Image.asset(
            asset,
            fit: BoxFit.cover,
            alignment: Alignment.topCenter,
          ),
        ),
        const SizedBox(height: 16),
        Text(
          caption,
          textAlign: TextAlign.center,
          style: context.text.bodyMedium!.copyWith(fontSize: 14),
        ),
      ],
    );
  }
}
