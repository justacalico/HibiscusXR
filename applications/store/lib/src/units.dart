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
