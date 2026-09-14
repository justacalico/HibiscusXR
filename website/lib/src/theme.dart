import 'package:flutter/material.dart';

/// Site palette. Light mirrors Apple's paper/grey; dark mirrors their
/// true-black sections. Accent is the Apple blue pair.
abstract final class AppColors {
  static const lightPaper = Color(0xFFFBFBFD);
  static const lightBand = Color(0xFFF5F5F7);
  static const lightInk = Color(0xFF1D1D1F);
  static const lightSecondary = Color(0xFF6E6E73);
  static const lightHairline = Color(0xFFD2D2D7);

  static const darkPaper = Color(0xFF000000);
  static const darkBand = Color(0xFF161617);
  static const darkInk = Color(0xFFF5F5F7);
  static const darkSecondary = Color(0xFFA1A1A6);
  static const darkHairline = Color(0xFF424245);

  static const accent = Color(0xFF0071E3);
  static const accentHover = Color(0xFF0077ED);
  static const accentOnDark = Color(0xFF2997FF);
}

/// Layout constants shared by every page.
abstract final class Layout {
  /// Max width of a text column, matching Apple's content rail.
  static const content = 1024.0;
  static const text = 660.0;
  static const navHeight = 52.0;
  static const mobileBreak = 760.0;

  static bool isMobile(BuildContext context) =>
      MediaQuery.sizeOf(context).width < mobileBreak;

  static EdgeInsets pagePadding(BuildContext context) =>
      EdgeInsets.symmetric(horizontal: isMobile(context) ? 20 : 32);
}

TextTheme _textTheme(Color ink, Color secondary) {
  const display = 'InterDisplay';
  const body = 'Inter';
  return TextTheme(
    displayLarge: TextStyle(
      fontFamily: display,
      fontSize: 80,
      height: 1.05,
      fontWeight: FontWeight.w600,
      letterSpacing: -1.8,
      color: ink,
    ),
    displayMedium: TextStyle(
      fontFamily: display,
      fontSize: 56,
      height: 1.07,
      fontWeight: FontWeight.w600,
      letterSpacing: -1.2,
      color: ink,
    ),
    headlineMedium: TextStyle(
      fontFamily: display,
      fontSize: 40,
      height: 1.1,
      fontWeight: FontWeight.w600,
      letterSpacing: -0.6,
      color: ink,
    ),
    titleLarge: TextStyle(
      fontFamily: display,
      fontSize: 24,
      height: 1.2,
      fontWeight: FontWeight.w600,
      letterSpacing: -0.3,
      color: ink,
    ),
    titleMedium: TextStyle(
      fontFamily: display,
      fontSize: 19,
      height: 1.3,
      fontWeight: FontWeight.w600,
      color: ink,
    ),
    bodyLarge: TextStyle(
      fontFamily: body,
      fontSize: 19,
      height: 1.42,
      fontWeight: FontWeight.w400,
      color: ink,
    ),
    bodyMedium: TextStyle(
      fontFamily: body,
      fontSize: 17,
      height: 1.47,
      fontWeight: FontWeight.w400,
      color: secondary,
    ),
    labelLarge: TextStyle(
      fontFamily: body,
      fontSize: 17,
      height: 1.2,
      fontWeight: FontWeight.w400,
      color: ink,
    ),
    labelSmall: TextStyle(
      fontFamily: body,
      fontSize: 12,
      height: 1.33,
      fontWeight: FontWeight.w400,
      color: secondary,
    ),
  );
}

ThemeData _base({
  required Brightness brightness,
  required Color paper,
  required Color band,
  required Color ink,
  required Color secondary,
  required Color hairline,
  required Color accent,
}) {
  final scheme = ColorScheme(
    brightness: brightness,
    primary: accent,
    onPrimary: Colors.white,
    secondary: secondary,
    onSecondary: ink,
    error: const Color(0xFFE30000),
    onError: Colors.white,
    surface: paper,
    onSurface: ink,
    surfaceContainerHighest: band,
    outline: hairline,
  );
  return ThemeData(
    useMaterial3: true,
    colorScheme: scheme,
    scaffoldBackgroundColor: paper,
    textTheme: _textTheme(ink, secondary),
    splashFactory: NoSplash.splashFactory,
    highlightColor: Colors.transparent,
    hoverColor: Colors.transparent,
    dividerColor: hairline,
    pageTransitionsTheme: const PageTransitionsTheme(
      builders: {
        TargetPlatform.android: FadeUpwardsPageTransitionsBuilder(),
        TargetPlatform.iOS: FadeUpwardsPageTransitionsBuilder(),
        TargetPlatform.linux: FadeUpwardsPageTransitionsBuilder(),
        TargetPlatform.macOS: FadeUpwardsPageTransitionsBuilder(),
        TargetPlatform.windows: FadeUpwardsPageTransitionsBuilder(),
      },
    ),
  );
}

ThemeData lightTheme() => _base(
      brightness: Brightness.light,
      paper: AppColors.lightPaper,
      band: AppColors.lightBand,
      ink: AppColors.lightInk,
      secondary: AppColors.lightSecondary,
      hairline: AppColors.lightHairline,
      accent: AppColors.accent,
    );

ThemeData darkTheme() => _base(
      brightness: Brightness.dark,
      paper: AppColors.darkPaper,
      band: AppColors.darkBand,
      ink: AppColors.darkInk,
      secondary: AppColors.darkSecondary,
      hairline: AppColors.darkHairline,
      accent: AppColors.accentOnDark,
    );

/// Convenience getters that keep widget code free of raw colors.
extension SiteTheme on BuildContext {
  ColorScheme get colors => Theme.of(this).colorScheme;
  TextTheme get text => Theme.of(this).textTheme;
  bool get isDark => Theme.of(this).brightness == Brightness.dark;
}
