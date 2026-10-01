import 'package:flutter_test/flutter_test.dart';

import 'package:hibiscusxr_website/src/flashdocs.dart';
import 'package:hibiscusxr_website/src/routes.dart';

void main() {
  test('flashdocs device slugs are unique and routable', () {
    final slugs = flashDocDevices.map((d) => d.slug).toList();
    expect(slugs.toSet().length, slugs.length);
    for (final d in flashDocDevices) {
      expect(d.slug, isNotEmpty);
      expect(d.path, Routes.flashdocsDevice(d.slug));
      expect(d.path.startsWith(Routes.flashdocs), isTrue);
      for (final s in d.systems) {
        expect(
          d.guidePath(s.slug),
          Routes.flashdocsGuide(d.slug, s.slug),
        );
        expect(d.guidePath(s.slug).startsWith(d.path), isTrue);
      }
    }
  });

  test('lookups resolve known slugs and reject unknown ones', () {
    final neo2 = flashDocDevice('pico-neo-2');
    expect(neo2, isNotNull);
    expect(flashDocDevice('quest-3'), isNull);
    expect(flashDocDevice(null), isNull);
    expect(flashDocDevice(''), isNull);

    expect(flashDocSystem(neo2!, 'linux'), isNotNull);
    expect(flashDocSystem(neo2, 'windows'), isNull);
    expect(flashDocSystem(neo2, 'macos'), isNull);
    expect(flashDocSystem(neo2, null), isNull);
  });
}
