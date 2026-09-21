import 'package:flutter_test/flutter_test.dart';

import 'package:pn2_website/src/links.dart';
import 'package:pn2_website/src/routes.dart';

void main() {
  test('site routes are all root-anchored and unique', () {
    const paths = [
      Routes.home,
      Routes.status,
      Routes.repositories,
      Routes.screenshots,
      Routes.download,
      Routes.faq,
      Routes.about,
    ];
    expect(paths.toSet().length, paths.length);
    for (final path in paths) {
      expect(path.startsWith('/'), isTrue, reason: path);
    }
  });

  test('external links are https gitlab URLs', () {
    for (final url in [
      Links.docs,
      Links.group,
      Links.vrhome,
      Links.library,
      Links.out,
      Links.notes,
      Links.issues,
      Links.newIssue,
    ]) {
      final uri = Uri.parse(url);
      expect(uri.scheme, 'https', reason: url);
      if (uri.host == 'gitlab.com') {
        expect(uri.path.startsWith('/neosalsa'), isTrue, reason: url);
      } else {
        expect(uri.host.endsWith('.gitlab.io'), isTrue, reason: url);
      }
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
