import 'package:flutter_test/flutter_test.dart';

import 'package:pn2_website/src/builds.dart';

void main() {
  group('channelOfTag', () {
    test('release tags have no prefix', () {
      expect(channelOfTag('v2026.09.15-r6'), BuildChannel.release);
    });

    test('beta tags are prefixed', () {
      expect(channelOfTag('beta-v2026.09.15-r6'), BuildChannel.beta);
    });

    test('alpha tags are prefixed', () {
      expect(channelOfTag('alpha-v2026.09.15-r6'), BuildChannel.alpha);
    });
  });

  group('parseReleases', () {
    final json = [
      {
        'tag_name': 'v2026.09.15-r6',
        'name': 'PN2 images v2026.09.15-r6',
        'created_at': '2026-09-15T16:00:00.000Z',
        'assets': {
          'links': [
            {'name': 'system-pn2-full.img.xz',
             'url': 'https://gitlab.com/api/v4/projects/86495557/packages/generic/release-assets/v2026.09.15-r6/system-pn2-full.img.xz'},
            {'name': 'system-pn2.img.xz',
             'url': 'https://gitlab.com/api/v4/projects/86495557/packages/generic/release-assets/v2026.09.15-r6/system-pn2.img.xz'},
            {'name': 'SHA256SUMS.txt',
             'url': 'https://gitlab.com/api/v4/projects/86495557/packages/generic/release-assets/v2026.09.15-r6/SHA256SUMS.txt'},
          ],
        },
      },
      {
        'tag_name': 'alpha-v2026.09.15-r7',
        'name': 'PN2 images alpha-v2026.09.15-r7',
        'created_at': '2026-09-15T17:00:00.000Z',
        'assets': {
          'links': [
            {'name': 'system-pn2-full.img.xz',
             'url': 'https://gitlab.com/api/v4/projects/86495557/packages/generic/release-assets/alpha-v2026.09.15-r7/system-pn2-full.img.xz'},
          ],
        },
      },
      {
        'tag_name': 'beta-v2026.09.15-r5',
        'name': 'PN2 images beta-v2026.09.15-r5',
        'created_at': '2026-09-15T15:00:00.000Z',
        'assets': {
          'links': [],
        },
      },
    ];

    test('parses tag, name, date, channel', () {
      final releases = parseReleases(json);
      expect(releases.length, 3);

      expect(releases[0].tag, 'v2026.09.15-r6');
      expect(releases[0].channel, BuildChannel.release);
      expect(releases[0].name, 'PN2 images v2026.09.15-r6');
      expect(releases[0].createdAt, DateTime.parse('2026-09-15T16:00:00.000Z'));

      expect(releases[1].tag, 'alpha-v2026.09.15-r7');
      expect(releases[1].channel, BuildChannel.alpha);

      expect(releases[2].tag, 'beta-v2026.09.15-r5');
      expect(releases[2].channel, BuildChannel.beta);
    });

    test('parses asset links', () {
      final releases = parseReleases(json);
      expect(releases[0].assets.length, 3);
      expect(releases[0].assets[0].name, 'system-pn2-full.img.xz');
      expect(releases[0].assets[0].isImage, isTrue);
      expect(releases[0].assets[2].name, 'SHA256SUMS.txt');
      expect(releases[0].assets[2].isImage, isFalse);
    });

    test('fullImage and cleanImage find the right assets', () {
      final releases = parseReleases(json);
      expect(releases[0].fullImage!.name, 'system-pn2-full.img.xz');
      expect(releases[0].cleanImage!.name, 'system-pn2.img.xz');
    });

    test('empty assets list is fine', () {
      final releases = parseReleases(json);
      expect(releases[2].assets, isEmpty);
      expect(releases[2].fullImage!.name, '');
    });
  });

  group('releasesInChannel', () {
    final releases = parseReleases([
      {
        'tag_name': 'v2026.09.15-r6',
        'name': 'r6',
        'created_at': '2026-09-15T16:00:00.000Z',
        'assets': {'links': []},
      },
      {
        'tag_name': 'alpha-v2026.09.15-r7',
        'name': 'r7',
        'created_at': '2026-09-15T17:00:00.000Z',
        'assets': {'links': []},
      },
      {
        'tag_name': 'alpha-v2026.09.14-r3',
        'name': 'r3',
        'created_at': '2026-09-14T10:00:00.000Z',
        'assets': {'links': []},
      },
      {
        'tag_name': 'beta-v2026.09.15-r5',
        'name': 'r5',
        'created_at': '2026-09-15T15:00:00.000Z',
        'assets': {'links': []},
      },
    ]);

    test('filters to one channel', () {
      expect(releasesInChannel(releases, BuildChannel.alpha).length, 2);
      expect(releasesInChannel(releases, BuildChannel.beta).length, 1);
      expect(releasesInChannel(releases, BuildChannel.release).length, 1);
    });

    test('sorts newest first', () {
      final alpha = releasesInChannel(releases, BuildChannel.alpha);
      expect(alpha[0].tag, 'alpha-v2026.09.15-r7');
      expect(alpha[1].tag, 'alpha-v2026.09.14-r3');
    });
  });

  test('parseReleases handles missing name and assets', () {
    final releases = parseReleases([
      {'tag_name': 'v2026.01.01-r1', 'created_at': '2026-01-01T00:00:00.000Z'},
    ]);
    expect(releases.length, 1);
    expect(releases[0].name, 'v2026.01.01-r1');
    expect(releases[0].assets, isEmpty);
  });
}
