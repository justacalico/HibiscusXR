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
  final text = value >= 100 ? value.round().toString() : value.toStringAsFixed(1);
  return '$text $unit';
}

/// F-Droid descriptions ship as light HTML. Reduce them to readable
/// plain text: line breaks kept, tags dropped, common entities decoded.
String stripHtml(String html) {
  var text = html
      .replaceAll(RegExp(r'<\s*br\s*/?\s*>', caseSensitive: false), '\n')
      .replaceAll(RegExp(r'</\s*(p|li|ul|ol|h[1-6])\s*>', caseSensitive: false), '\n')
      .replaceAll(RegExp(r'<\s*li\s*>', caseSensitive: false), '- ')
      .replaceAll(RegExp(r'<[^>]*>'), '')
      .replaceAll('&lt;', '<')
      .replaceAll('&gt;', '>')
      .replaceAll('&quot;', '"')
      .replaceAll('&#39;', "'")
      .replaceAll('&nbsp;', ' ')
      .replaceAll('&amp;', '&');
  text = text
      .split('\n')
      .map((line) => line.trim())
      .join('\n')
      .replaceAll(RegExp(r'\n{3,}'), '\n\n')
      .trim();
  return text;
}
