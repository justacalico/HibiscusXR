/// Slider values in the store are normalized 0..1, but the platform
/// channel and the UI speak real units for some rows. The conversions
/// live here so tests can reach them.
library;

/// Interpupillary distance range offered by the slider, in millimetres.
/// 56-74 covers essentially the whole adult population; the default is
/// the panel's physical lens spacing, which is what stock ships.
const double kIpdMinMm = 56.0;
const double kIpdMaxMm = 74.0;
const double kIpdDefaultMm = 63.5;

/// Normalized slider position -> millimetres.
double ipdFromSlider(double v) =>
    kIpdMinMm + v.clamp(0.0, 1.0) * (kIpdMaxMm - kIpdMinMm);

/// Millimetres -> normalized slider position.
double ipdToSlider(double mm) =>
    ((mm - kIpdMinMm) / (kIpdMaxMm - kIpdMinMm)).clamp(0.0, 1.0);
