import 'package:flutter_test/flutter_test.dart';

import 'package:pn2_website/src/links.dart';
import 'package:pn2_website/src/routes.dart';

void main() {
  test('site routes are all root-anchored and unique', () {
    const paths = [
      Routes.home,
      Routes.features,
      Routes.screenshots,
      Routes.download,
      Routes.about,
    ];
    expect(paths.toSet().length, paths.length);
    for (final path in paths) {
      expect(path.startsWith('/'), isTrue, reason: path);
    }
  });

  test('external links are https gitlab URLs', () {
    for (final url in [Links.docs, Links.repo, Links.group]) {
      final uri = Uri.parse(url);
      expect(uri.scheme, 'https', reason: url);
      expect(uri.host, 'gitlab.com', reason: url);
    }
  });

  test('destination flags external targets', () {
    expect(Destination('/about', (l) => '').isExternal, isFalse);
    expect(
      Destination('https://gitlab.com/x', (l) => '').isExternal,
      isTrue,
    );
  });
}
