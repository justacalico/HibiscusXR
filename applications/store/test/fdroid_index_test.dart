import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_store/src/fdroid_index.dart';
import 'package:pn2_store/src/units.dart';

import 'helpers.dart';

void main() {
  group('repoFileUri', () {
    final repo = Uri.parse('https://repo.example/fdroid/repo');

    test('joins a plain name', () {
      expect(
        repoFileUri(repo, 'index-v2.json').toString(),
        'https://repo.example/fdroid/repo/index-v2.json',
      );
    });

    test('strips the leading slash index files carry', () {
      expect(
        repoFileUri(repo, '/org.fdroid.fdroid_1.apk').toString(),
        'https://repo.example/fdroid/repo/org.fdroid.fdroid_1.apk',
      );
    });

    test('keeps an already-trailing-slash base intact', () {
      final slashed = Uri.parse('https://repo.example/fdroid/repo/');
      expect(
        repoFileUri(slashed, 'icons/a.png').toString(),
        'https://repo.example/fdroid/repo/icons/a.png',
      );
    });
  });

  group('localized', () {
    test('prefers the exact locale', () {
      expect(
        localized({'de': 'Hallo', 'en-US': 'Hello'}, locale: 'de'),
        'Hallo',
      );
    });

    test('falls back through language then en-US then any value', () {
      expect(localized({'fr': 'Salut'}, locale: 'de-DE'), 'Salut');
      expect(localized({'en-US': 'Hey'}, locale: 'de-DE'), 'Hey');
      expect(
        localized({'en': 'lang-only'}, locale: 'en-GB'),
        'lang-only',
      );
      expect(localized(null), '');
      expect(localized({}, fallback: 'x'), 'x');
      // An empty preferred string keeps looking.
      expect(localized({'de': '', 'en-US': 'Hi'}, locale: 'de'), 'Hi');
    });
  });

  group('localizedFileName', () {
    test('reads the name field by locale', () {
      expect(
        localizedFileName({
          'en-US': {'name': 'icons/a.png'},
        }),
        'icons/a.png',
      );
      expect(
        localizedFileName({
          'de': {'name': 'icons/de.png'},
        }, locale: 'de-DE'),
        'icons/de.png',
      );
      expect(localizedFileName(null), isNull);
      expect(localizedFileName({}), isNull);
      expect(
        localizedFileName({
          'en-US': {'sha256': 'x'},
        }),
        isNull,
      );
      // A locale holding a malformed entry falls through to the next.
      expect(
        localizedFileName({
          'en-US': 'oops',
          'de': {'name': 'icons/fb.png'},
        }),
        'icons/fb.png',
      );
    });
  });

  group('parseFdroidIndex', () {
    test('rejects documents without repo+packages', () {
      expect(() => parseFdroidIndex({}), throwsFormatException);
      expect(
        () => parseFdroidIndex({'repo': {}, 'packages': []}),
        throwsFormatException,
      );
    });

    test('parses the fixture', () {
      final index = testIndex();
      expect(index.name, 'Test Repo');
      expect(index.address, 'https://repo.example/fdroid/repo');
      expect(index.timestamp.millisecondsSinceEpoch, 1756700000000);
      expect(index.apps, hasLength(3));
    });

    test('app fields land on the model', () {
      final alpha = testIndex()
          .apps
          .firstWhere((a) => a.packageName == 'com.example.alpha');
      expect(alpha.name, 'Alpha Player');
      expect(alpha.summary, 'Plays things');
      expect(alpha.description, '<p>Long <b>about</b> text.</p>');
      expect(alpha.author, 'Example Inc');
      expect(alpha.license, 'GPL-3.0-only');
      expect(alpha.categories, ['Multimedia']);
      expect(alpha.iconPath, 'icons/alpha.png');
      expect(alpha.screenshots, ['phone/a1.png', 'phone/a2.png']);
      expect(alpha.latest!.versionCode, 12);
      expect(alpha.latest!.versionName, '1.2');
      expect(alpha.latest!.apkPath, '/com.example.alpha_12.apk');
      expect(alpha.latest!.size, 1500000);
      expect(alpha.latest!.minSdk, 24);
    });

    test('versions sort newest first', () {
      final alpha = testIndex()
          .apps
          .firstWhere((a) => a.packageName == 'com.example.alpha');
      expect(alpha.versions.map((v) => v.versionCode), [12, 10]);
    });

    test('missing metadata degrades gracefully', () {
      final beta = testIndex()
          .apps
          .firstWhere((a) => a.packageName == 'com.example.beta');
      // German name exists but only en-US is requested: first value wins.
      expect(beta.name, isNotEmpty);
      expect(beta.summary, 'Does utility things');
      expect(beta.description, '');
      expect(beta.iconPath, isNull);
      expect(beta.screenshots, isEmpty);
      expect(beta.author, isNull);
      final v = beta.latest!;
      expect(v.apkPath, 'beta/beta_7.apk');
      expect(v.minSdk, isNull);
      expect(v.added.millisecondsSinceEpoch, 0);
    });

    test('empty versions map yields an empty list and null latest', () {
      final gamma = testIndex()
          .apps
          .firstWhere((a) => a.packageName == 'com.example.gamma');
      expect(gamma.versions, isEmpty);
      expect(gamma.latest, isNull);
      expect(gamma.name, 'Gamma Notes');
    });

    test('package entries that are not objects are skipped', () {
      final index = parseFdroidIndex({
        'repo': {'name': 'x', 'address': 'https://a', 'timestamp': 1},
        'packages': {'bad': 'nope', 'good': {'metadata': {}}},
      });
      expect(index.apps, hasLength(1));
      expect(index.apps.single.packageName, 'good');
      // No name falls back to the package name.
      expect(index.apps.single.name, 'good');
    });

    test('screenshots walk the non-phone buckets', () {
      final index = parseFdroidIndex({
        'repo': {'name': 'x', 'address': 'a', 'timestamp': 0},
        'packages': {
          'tv.app': {
            'metadata': {
              'name': {'en-US': 'TV'},
              'screenshots': {
                'tv': {
                  'en-US': [
                    {'name': 'tv/1.png'},
                  ],
                },
              },
            },
            'versions': {},
          },
          'wear.app': {
            'metadata': {
              'name': {'en-US': 'Wear'},
              'screenshots': {
                'wear': {
                  'en-US': [
                    {'name': 'w/1.png'},
                  ],
                },
              },
            },
            'versions': {},
          },
          'badshots.app': {
            'metadata': {
              'name': {'en-US': 'Bad'},
              'screenshots': {'phone': 'oops'},
            },
            'versions': {},
          },
          'emptylist.app': {
            'metadata': {
              'name': {'en-US': 'Empty'},
              'screenshots': {
                'phone': {'en-US': <String>[]},
              },
            },
            'versions': {},
          },
          'fallbacklist.app': {
            'metadata': {
              'name': {'en-US': 'Fb'},
              'screenshots': {
                'phone': {
                  'de': [
                    {'name': 'de/1.png'},
                  ],
                },
              },
            },
            'versions': {},
          },
        },
      });
      RepoApp app(String pkg) =>
          index.apps.firstWhere((a) => a.packageName == pkg);
      expect(app('tv.app').screenshots, ['tv/1.png']);
      expect(app('wear.app').screenshots, ['w/1.png']);
      expect(app('badshots.app').screenshots, isEmpty);
      expect(app('emptylist.app').screenshots, isEmpty);
      expect(app('fallbacklist.app').screenshots, ['de/1.png']);
    });
  });

  group('decodeFdroidIndex', () {
    test('decodes a JSON body', () {
      final index = decodeFdroidIndex(kIndexJson);
      expect(index.apps, hasLength(3));
    });

    test('invalid json throws', () {
      expect(() => decodeFdroidIndex('not json'), throwsA(anything));
    });
  });

  group('formatBytes', () {
    test('formats each band', () {
      expect(formatBytes(0), '0 B');
      expect(formatBytes(1023), '1023 B');
      expect(formatBytes(1024), '1.0 KB');
      expect(formatBytes(1500000), '1.4 MB');
      expect(formatBytes(150 * 1024 * 1024), '150 MB');
      expect(formatBytes(3 * 1024 * 1024 * 1024), '3.0 GB');
    });
  });

  group('stripHtml', () {
    test('drops tags and decodes entities', () {
      expect(
        stripHtml('<p>Tom &amp; Jerry <b>bold</b></p><p>&lt;tag&gt; &#39;q&#39; &quot;d&quot;&nbsp;x</p>'),
        'Tom & Jerry bold\n<tag> \'q\' "d" x',
      );
    });

    test('turns lists and breaks into newlines', () {
      expect(
        stripHtml('<ul><li>one</li><li>two</li></ul>tail<br>line'),
        '- one\n- two\n\ntail\nline',
      );
    });

    test('collapses runs of blank lines', () {
      expect(stripHtml('<p>a</p>\n\n\n<p>b</p>'), 'a\n\nb');
    });
  });
}
