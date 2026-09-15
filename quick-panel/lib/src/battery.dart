/// Battery level bands that drive the status bar tint.
enum BatteryTint { good, normal, warn, critical }

/// >= 80 green, 60-79 neutral, 20-59 yellow, below 20 red.
BatteryTint batteryTintFor(int level) {
  if (level >= 80) return BatteryTint.good;
  if (level >= 60) return BatteryTint.normal;
  if (level >= 20) return BatteryTint.warn;
  return BatteryTint.critical;
}
