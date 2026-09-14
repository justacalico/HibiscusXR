import 'package:flutter/material.dart';

/// visionOS-flavoured theme: floating glass over a deep neutral gradient,
/// circular icons, thin white-on-glass strokes, one cool accent.
class LibraryTheme {
  // backdrop
  static const bgTop = Color(0xFF0C0F13);
  static const bgBottom = Color(0xFF161B22);
  static const glow = Color(0xFF223040);

  // glass
  static const glassFill = Color(0x14FFFFFF);
  static const glassFillHi = Color(0x24FFFFFF);
  static const glassStroke = Color(0x2EFFFFFF);
  static const glassStrokeHi = Color(0x40FFFFFF);

  static const accent = Color(0xFF6AABFF);
  static const danger = Color(0xFFFF6B66);
  static const textPrimary = Color(0xF2FFFFFF);
  static const textSecondary = Color(0x99FFFFFF);
  static const textFaint = Color(0x66FFFFFF);

  static const tileRadius = 26.0;
  static const pillRadius = 24.0;

  /// Page backdrop: near-black with a faint cool glow up top, so the glass
  /// controls have something to sit against.
  static const backdropGradient = LinearGradient(
    begin: Alignment.topCenter,
    end: Alignment.bottomCenter,
    colors: [bgTop, bgBottom],
  );

  static const backdropGlow = RadialGradient(
    center: Alignment(0, -0.85),
    radius: 1.1,
    colors: [Color(0x33223040), Color(0x00000000)],
  );

  static ThemeData data() {
    final base = ThemeData.dark(useMaterial3: true);
    return base.copyWith(
      scaffoldBackgroundColor: bgBottom,
      colorScheme: const ColorScheme.dark(
        surface: Color(0xFF1C232C),
        primary: accent,
        error: danger,
        onSurface: textPrimary,
      ),
      textTheme: base.textTheme.apply(
        bodyColor: textPrimary,
        displayColor: textPrimary,
      ),
      dividerColor: glassStroke,
      popupMenuTheme: PopupMenuThemeData(
        color: const Color(0xF228323D),
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(18),
          side: const BorderSide(color: glassStroke),
        ),
        labelTextStyle: WidgetStateProperty.all(
          const TextStyle(color: textPrimary, fontSize: 14),
        ),
      ),
      dialogTheme: DialogThemeData(
        backgroundColor: const Color(0xF21F2730),
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(28),
          side: const BorderSide(color: glassStroke),
        ),
        titleTextStyle: const TextStyle(
          color: textPrimary,
          fontSize: 18,
          fontWeight: FontWeight.w600,
        ),
      ),
      snackBarTheme: const SnackBarThemeData(
        backgroundColor: Color(0xF22E3946),
        contentTextStyle: TextStyle(color: textPrimary),
        behavior: SnackBarBehavior.floating,
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: glassFill,
        hintStyle: const TextStyle(color: textSecondary),
        contentPadding:
            const EdgeInsets.symmetric(horizontal: 20, vertical: 14),
        enabledBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(pillRadius),
          borderSide: const BorderSide(color: glassStroke),
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(pillRadius),
          borderSide: const BorderSide(color: accent, width: 1.5),
        ),
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(pillRadius),
          borderSide: BorderSide.none,
        ),
      ),
    );
  }
}
