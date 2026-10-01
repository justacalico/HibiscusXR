import 'package:flutter_test/flutter_test.dart';

import 'package:hibiscusxr_website/src/apps.dart';
import 'package:hibiscusxr_website/src/links.dart';
import 'package:hibiscusxr_website/src/routes.dart';

void main() {
  test('site routes are all root-anchored and unique', () {
    final paths = [
      Routes.home,
      Routes.repositories,
      Routes.screenshots,
      Routes.download,
      Routes.cte,
      Routes.cteDownload,
      Routes.hbsupDownload,
      Routes.faq,
      Routes.about,
      Routes.flashdocs,
      Routes.flashdocsDevice('pico-neo-2'),
      Routes.flashdocsGuide('pico-neo-2', 'linux'),
    ];
    expect(paths.toSet().length, paths.length);
    for (final path in paths) {
      expect(path.startsWith('/'), isTrue, reason: path);
    }
  });

  test('external links are https gitlab URLs', () {
    for (final url in [
      Links.repo,
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

  test('site app metadata', () {
    expect(SiteApps.label(SiteApp.cte), 'HCTE');
    expect(SiteApps.label(SiteApp.hbsup), 'HBSUP');
    expect(SiteApps.tagPrefix(SiteApp.cte), 'cte-');
    expect(SiteApps.tagPrefix(SiteApp.hbsup), 'hbsup-');
    expect(SiteApps.treePath(SiteApp.cte), 'applications/cte');
    expect(SiteApps.treePath(SiteApp.hbsup), 'applications/hbsup');
  });

  test('destination flags external targets', () {
    expect(Destination('/about', (l) => '').isExternal, isFalse);
    expect(
      Destination('https://gitlab.com/x', (l) => '').isExternal,
      isTrue,
    );
  });
}
