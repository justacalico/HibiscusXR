/// Byte counts as "820 B", "1.4 MB", ...
String formatBytes(int bytes) {
  if (bytes < 1024) return '$bytes B';
  const units = ['KB', 'MB', 'GB', 'TB'];
  var value = bytes.toDouble();
  var unit = 'B';
  for (final u in units) {
    if (value < 1024) break;
    value /= 1024;
    unit = u;
  }
  final text =
      value >= 100 ? value.round().toString() : value.toStringAsFixed(1);
  return '$text $unit';
}
