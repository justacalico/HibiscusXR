import 'dart:math' show pi;

import 'package:flutter/material.dart';

import 'package:pn2_website/l10n/app_localizations.dart';

import '../library_mock.dart';
import '../theme.dart';

/// The library window drawn in widgets instead of a screenshot asset.
/// Built at a fixed 880x550 canvas and scaled with FittedBox so every
/// detail keeps its proportions at any render width.
class LibraryShot extends StatelessWidget {
  const LibraryShot({super.key});

  static const double designWidth = 880;
  static const double designHeight = 550;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final tiles = libraryTiles(l10n);
    return Semantics(
      image: true,
      label: l10n.heroShotCaption,
      child: ExcludeSemantics(
        child: AspectRatio(
          aspectRatio: designWidth / designHeight,
          child: FittedBox(
            child: SizedBox(
              width: designWidth,
              height: designHeight,
              child: ColoredBox(
                color: AppColors.shotSurface,
                child: Column(
                  children: [
                    _StatusBar(l10n: l10n),
                    _Toolbar(l10n: l10n),
                    Expanded(
                      child: SingleChildScrollView(
                        physics: const NeverScrollableScrollPhysics(),
                        child: Padding(
                          padding: const EdgeInsets.fromLTRB(24, 12, 24, 0),
                          child: Column(
                            children: [
                              for (var row = 0; row < 3; row++)
                                Padding(
                                  padding: EdgeInsets.only(
                                      bottom: row == 2 ? 0 : 22),
                                  child: Row(
                                    children: [
                                      for (var col = 0; col < 3; col++)
                                        Expanded(
                                          child: Padding(
                                            padding: EdgeInsets.only(
                                                right: col == 2 ? 0 : 16),
                                            child:
                                                _Tile(tiles[row * 3 + col]),
                                          ),
                                        ),
                                    ],
                                  ),
                                ),
                            ],
                          ),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}

class _StatusBar extends StatelessWidget {
  const _StatusBar({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(28, 14, 28, 0),
      child: Row(
        children: [
          Text(
            l10n.shotMockTime,
            style: context.text.labelSmall!.copyWith(
              fontSize: 15,
              fontWeight: FontWeight.w500,
              color: AppColors.shotInk,
            ),
          ),
          const Spacer(),
          const Icon(Icons.signal_cellular_alt_rounded,
              size: 15, color: AppColors.shotInk),
          const SizedBox(width: 5),
          const Icon(Icons.wifi_rounded, size: 15, color: AppColors.shotInk),
          const SizedBox(width: 5),
          const Icon(Icons.battery_charging_full_rounded,
              size: 19, color: AppColors.ok),
        ],
      ),
    );
  }
}

class _Toolbar extends StatelessWidget {
  const _Toolbar({required this.l10n});

  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(24, 10, 24, 8),
      child: Row(
        children: [
          Container(
            width: 252,
            height: 44,
            padding: const EdgeInsets.symmetric(horizontal: 16),
            decoration: BoxDecoration(
              color: AppColors.shotWell,
              borderRadius: BorderRadius.circular(22),
            ),
            child: Row(
              children: [
                const Icon(Icons.search_rounded,
                    size: 19, color: AppColors.shotMuted),
                const SizedBox(width: 10),
                Text(
                  l10n.shotMockSearch,
                  style: context.text.labelSmall!.copyWith(
                    fontSize: 15,
                    color: AppColors.shotMuted,
                  ),
                ),
              ],
            ),
          ),
          const Spacer(),
          _WellChip(
            children: [
              const Icon(Icons.grid_view_rounded,
                  size: 15, color: AppColors.shotInk),
              const SizedBox(width: 7),
              Text(
                l10n.shotMockFilter(libraryTotal),
                style: context.text.labelSmall!
                    .copyWith(fontSize: 14, color: AppColors.shotInk),
              ),
              const SizedBox(width: 4),
              const Icon(Icons.keyboard_arrow_down_rounded,
                  size: 16, color: AppColors.shotMuted),
            ],
          ),
          const SizedBox(width: 10),
          _WellChip(
            children: [
              const Icon(Icons.sort_rounded,
                  size: 15, color: AppColors.shotInk),
              const SizedBox(width: 7),
              Text(
                l10n.shotMockSort,
                style: context.text.labelSmall!
                    .copyWith(fontSize: 14, color: AppColors.shotInk),
              ),
              const SizedBox(width: 4),
              const Icon(Icons.keyboard_arrow_down_rounded,
                  size: 16, color: AppColors.shotMuted),
            ],
          ),
          const SizedBox(width: 10),
          Container(
            width: 44,
            height: 44,
            decoration: const BoxDecoration(
              color: AppColors.shotWell,
              shape: BoxShape.circle,
            ),
            child: const Icon(Icons.add_rounded,
                size: 20, color: AppColors.shotInk),
          ),
        ],
      ),
    );
  }
}

class _WellChip extends StatelessWidget {
  const _WellChip({required this.children});

  final List<Widget> children;

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 44,
      padding: const EdgeInsets.symmetric(horizontal: 16),
      decoration: BoxDecoration(
        color: AppColors.shotWell,
        borderRadius: BorderRadius.circular(22),
      ),
      child: Row(mainAxisSize: MainAxisSize.min, children: children),
    );
  }
}

class _Tile extends StatelessWidget {
  const _Tile(this.tile);

  final LibraryTile tile;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Column(
      children: [
        Container(
          height: 128,
          decoration: BoxDecoration(
            color: tile.tint,
            borderRadius: BorderRadius.circular(18),
            border: tile.pinned
                ? Border.all(color: AppColors.shotAccent, width: 2)
                : null,
          ),
          child: Stack(
            children: [
              if (tile.pinned)
                Positioned(
                  top: 8,
                  left: 8,
                  child: Container(
                    width: 22,
                    height: 22,
                    decoration: const BoxDecoration(
                      color: AppColors.shotPill,
                      shape: BoxShape.circle,
                    ),
                    child: const Icon(Icons.push_pin_rounded,
                        size: 12, color: AppColors.shotInk),
                  ),
                ),
              Positioned(
                top: tile.pinned ? 36 : 10,
                left: 10,
                child: Container(
                  height: 20,
                  padding: const EdgeInsets.symmetric(horizontal: 8),
                  decoration: BoxDecoration(
                    color: AppColors.shotPill,
                    borderRadius: BorderRadius.circular(10),
                  ),
                  alignment: Alignment.center,
                  child: Text(
                    l10n.shotMockSystem,
                    style: context.text.labelSmall!
                        .copyWith(fontSize: 11, color: AppColors.shotInk),
                  ),
                ),
              ),
              const Positioned(
                top: 10,
                right: 10,
                child: Icon(Icons.more_vert_rounded,
                    size: 17, color: AppColors.shotInk),
              ),
              Center(
                child: CustomPaint(
                  size: const Size.square(76),
                  painter: LibraryMarkPainter(
                    tile.mark,
                    digits: context.text.labelSmall!.copyWith(
                      fontSize: 15,
                      fontWeight: FontWeight.w700,
                      color: AppColors.shotMarkInk,
                      height: 1,
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
        const SizedBox(height: 8),
        Text(
          tile.name,
          style: context.text.labelSmall!
              .copyWith(fontSize: 15, color: AppColors.shotInk),
        ),
      ],
    );
  }
}

/// Paints the white icon disc and the app glyph inside it.
class LibraryMarkPainter extends CustomPainter {
  const LibraryMarkPainter(this.mark, {required this.digits});

  final LibraryMark mark;
  final TextStyle digits;

  static final _fill = Paint()..isAntiAlias = true;
  static final _stroke = Paint()
    ..isAntiAlias = true
    ..style = PaintingStyle.stroke
    ..strokeCap = StrokeCap.round;

  @override
  void paint(Canvas canvas, Size size) {
    final c = size.center(Offset.zero);
    canvas.drawCircle(c, size.width / 2, _fill..color = AppColors.shotDisc);
    switch (mark) {
      case LibraryMark.calendar:
        _calendar(canvas, c);
      case LibraryMark.camera:
        _camera(canvas, c);
      case LibraryMark.chrome:
        _chrome(canvas, c);
      case LibraryMark.clock:
        _clock(canvas, c);
      case LibraryMark.contacts:
        _contacts(canvas, c);
      case LibraryMark.drive:
        _drive(canvas, c);
      case LibraryMark.files:
        _files(canvas, c);
      case LibraryMark.gemini:
        _gemini(canvas, c);
      case LibraryMark.glasses:
        _glasses(canvas, c);
    }
  }

  void _calendar(Canvas canvas, Offset c) {
    final page = Rect.fromCenter(
        center: c + const Offset(0, 1.5), width: 37, height: 35);
    final rrect = RRect.fromRectAndRadius(page, const Radius.circular(5));
    canvas.drawRRect(rrect, _fill..color = AppColors.shotPaper);
    canvas.drawRRect(rrect, _stroke..color = AppColors.shotMuted..strokeWidth = 1);
    canvas.drawRRect(
      RRect.fromRectAndCorners(
        Rect.fromLTRB(page.left, page.top, page.right, page.top + 10),
        topLeft: const Radius.circular(5),
        topRight: const Radius.circular(5),
      ),
      _fill..color = AppColors.shotGoogle,
    );
    final tp = TextPainter(
      text: TextSpan(text: '31', style: digits),
      textDirection: TextDirection.ltr,
    )..layout(maxWidth: page.width);
    tp.paint(
      canvas,
      Offset(c.dx - tp.width / 2, page.top + 10 + (page.height - 10 - tp.height) / 2),
    );
  }

  void _camera(Canvas canvas, Offset c) {
    final body = Rect.fromCenter(
        center: c + const Offset(0, 2), width: 42, height: 30);
    canvas.drawRRect(
      RRect.fromRectAndRadius(
        Rect.fromLTWH(c.dx - 8, body.top - 4, 14, 5),
        const Radius.circular(2),
      ),
      _fill..color = AppColors.shotGoogle,
    );
    canvas.drawRRect(
      RRect.fromRectAndRadius(body, const Radius.circular(8)),
      _fill..color = AppColors.shotGoogle,
    );
    canvas.drawCircle(c + const Offset(0, 2), 8, _fill..color = AppColors.shotPaper);
    canvas.drawCircle(c + const Offset(0, 2), 4, _fill..color = AppColors.shotGoogle);
    canvas.drawCircle(Offset(body.right - 7, body.top + 6), 1.8,
        _fill..color = AppColors.shotPaper);
  }

  void _chrome(Canvas canvas, Offset c) {
    const r = 21.0;
    final rect = Rect.fromCircle(center: c, radius: r);
    const wedges = [
      (AppColors.shotRed, -150.0),
      (AppColors.shotYellow, -30.0),
      (AppColors.shotGreen, 90.0),
    ];
    for (final (color, start) in wedges) {
      canvas.drawArc(
        rect,
        start * pi / 180,
        120 * pi / 180,
        true,
        _fill..color = color,
      );
    }
    canvas.drawCircle(c, 10.5, _fill..color = AppColors.shotPaper);
    canvas.drawCircle(c, 9, _fill..color = AppColors.shotGoogle);
  }

  void _clock(Canvas canvas, Offset c) {
    canvas.drawCircle(c, 16.5,
        _stroke..color = AppColors.shotGoogle..strokeWidth = 4.5);
    canvas.drawLine(c, c + const Offset(0, -9.5),
        _stroke..color = AppColors.shotGoogle..strokeWidth = 3.5);
    canvas.drawLine(c, c + const Offset(7.5, 4.5),
        _stroke..color = AppColors.shotGoogle..strokeWidth = 3.5);
    canvas.drawCircle(c, 2.5, _fill..color = AppColors.shotGoogle);
  }

  void _contacts(Canvas canvas, Offset c) {
    canvas.drawRRect(
      RRect.fromRectAndCorners(
        Rect.fromLTWH(c.dx - 17, c.dy + 1, 34, 18),
        topLeft: const Radius.circular(9),
        topRight: const Radius.circular(9),
      ),
      _fill..color = AppColors.shotGoogle,
    );
    canvas.drawCircle(c - const Offset(0, 8), 7.5,
        _fill..color = AppColors.shotGoogle);
  }

  void _drive(Canvas canvas, Offset c) {
    final top = c + const Offset(0, -20);
    final left = c + const Offset(-19, 14);
    final right = c + const Offset(19, 14);
    _strip(canvas, top, left, c, AppColors.shotGreen);
    _strip(canvas, top, right, c, AppColors.shotYellow);
    _strip(canvas, left, right, c, AppColors.shotGoogle);
  }

  /// One edge of the Drive triangle: a quad of fixed thickness pushed
  /// toward [inside] so the three strips leave a hollow middle.
  void _strip(Canvas canvas, Offset a, Offset b, Offset inside, Color color) {
    const t = 8.0;
    final dir = (b - a).normalize();
    var n = Offset(-dir.dy, dir.dx);
    if ((inside - a).dot(n) < 0) n = -n;
    canvas.drawPath(
      Path()
        ..moveTo(a.dx, a.dy)
        ..lineTo(b.dx, b.dy)
        ..lineTo(b.dx + n.dx * t, b.dy + n.dy * t)
        ..lineTo(a.dx + n.dx * t, a.dy + n.dy * t)
        ..close(),
      _fill..color = color,
    );
  }

  void _files(Canvas canvas, Offset c) {
    final body = Rect.fromCenter(
        center: c + const Offset(0, 4), width: 40, height: 30);
    canvas.drawRRect(
      RRect.fromRectAndRadius(
        Rect.fromLTWH(body.left, body.top - 4, 17, 7),
        const Radius.circular(3),
      ),
      _fill..color = AppColors.shotGoogle,
    );
    canvas.drawRRect(
      RRect.fromRectAndRadius(body, const Radius.circular(6)),
      _fill..color = AppColors.shotGoogle,
    );
    canvas.drawRRect(
      RRect.fromRectAndRadius(
        Rect.fromCenter(center: c + const Offset(0, 6), width: 19, height: 15),
        const Radius.circular(3),
      ),
      _fill..color = AppColors.shotPaper,
    );
  }

  void _gemini(Canvas canvas, Offset c) {
    const r = 20.0;
    const k = 2.6;
    final path = Path()
      ..moveTo(c.dx, c.dy - r)
      ..quadraticBezierTo(c.dx + k, c.dy - k, c.dx + r, c.dy)
      ..quadraticBezierTo(c.dx + k, c.dy + k, c.dx, c.dy + r)
      ..quadraticBezierTo(c.dx - k, c.dy + k, c.dx - r, c.dy)
      ..quadraticBezierTo(c.dx - k, c.dy - k, c.dx, c.dy - r)
      ..close();
    canvas.drawPath(
      path,
      Paint()
        ..isAntiAlias = true
        ..shader = LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: const [AppColors.shotAccent, AppColors.shotViolet],
        ).createShader(Rect.fromCircle(center: c, radius: r)),
    );
  }

  void _glasses(Canvas canvas, Offset c) {
    _stroke
      ..color = AppColors.shotMarkInk
      ..strokeWidth = 3;
    for (final dx in [-11.5, 11.5]) {
      canvas.drawRRect(
        RRect.fromRectAndRadius(
          Rect.fromCenter(center: c + Offset(dx, 3), width: 16, height: 11),
          const Radius.circular(4),
        ),
        _stroke,
      );
    }
    canvas.drawPath(
      Path()
        ..moveTo(c.dx - 4, c.dy - 1)
        ..quadraticBezierTo(c.dx, c.dy - 6, c.dx + 4, c.dy - 1),
      _stroke,
    );
    canvas.drawLine(c + const Offset(-19.5, -1), c + const Offset(-22, -5),
        _stroke..strokeWidth = 2.5);
    canvas.drawLine(c + const Offset(19.5, -1), c + const Offset(22, -5),
        _stroke..strokeWidth = 2.5);
  }

  @override
  bool shouldRepaint(LibraryMarkPainter old) =>
      old.mark != mark || old.digits != digits;
}

extension on Offset {
  Offset normalize() => this / distance;
  double dot(Offset o) => dx * o.dx + dy * o.dy;
}
