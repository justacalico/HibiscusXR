import 'dart:typed_data';
import 'dart:ui' as ui;

import 'package:flutter/material.dart';

/// The decoded art for one tile: icon bytes plus a tint averaged from its
/// opaque pixels, used to color the tile backdrop.
class TileArt {
  const TileArt(this.bytes, this.tint);

  final Uint8List? bytes;
  final Color? tint;
}

/// Averages the opaque pixels of a decoded icon. Decodes at a tiny size -
/// this only needs to be roughly right.
Future<Color?> dominantIconColor(Uint8List png) async {
  try {
    final codec = await ui.instantiateImageCodec(
      png,
      targetWidth: 24,
      targetHeight: 24,
    );
    final frame = await codec.getNextFrame();
    final data =
        await frame.image.toByteData(format: ui.ImageByteFormat.rawRgba);
    frame.image.dispose();
    codec.dispose();
    if (data == null) return null;
    final px = data.buffer.asUint8List();
    var r = 0, g = 0, b = 0, n = 0;
    for (var i = 0; i + 3 < px.length; i += 4) {
      if (px[i + 3] < 40) continue;
      r += px[i];
      g += px[i + 1];
      b += px[i + 2];
      n++;
    }
    if (n == 0) return null;
    return Color.fromARGB(255, r ~/ n, g ~/ n, b ~/ n);
  } catch (_) {
    return null;
  }
}

/// Dark tile backdrop derived from the icon tint: a soft vertical gradient
/// that stays in the dark-theme family.
List<Color> tileGradient(Color? tint) {
  if (tint == null) {
    return const [Color(0xFF28313D), Color(0xFF1E2630)];
  }
  final hsl = HSLColor.fromColor(tint);
  final top = hsl
      .withSaturation((hsl.saturation * 0.55).clamp(0.0, 1.0))
      .withLightness((hsl.lightness * 0.45 + 0.14).clamp(0.0, 0.38))
      .toColor();
  final bottom = hsl
      .withSaturation((hsl.saturation * 0.45).clamp(0.0, 1.0))
      .withLightness((hsl.lightness * 0.3 + 0.09).clamp(0.0, 0.3))
      .toColor();
  return [top, bottom];
}
