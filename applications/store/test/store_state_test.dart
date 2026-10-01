import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_store/src/fdroid_index.dart';
import 'package:pn2_store/src/installer.dart';
import 'package:pn2_store/src/platform/fakes.dart';
import 'package:pn2_store/src/persistence.dart';
import 'package:pn2_store/src/store_state.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:pn2_store/src/platform/prefs_persistence.dart';

import 'helpers.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  group('StoreState', () {
    late StoreState store;

    setUp(() {
      store = StoreState(repoUrl: Uri.parse('https://repo.example/fdroid/repo'));
    });

    test('apps filters by query across name, package and summary', () {
      store.setReady(testIndex());
      store.setQuery('alpha');
      expect(store.apps.map((a) => a.packageName), ['com.example.alpha']);
      store.setQuery('COM.EXAMPLE.BETA');
      expect(store.apps.map((a) => a.packageName), ['com.example.beta']);
      store.setQuery('notes');
      expect(store.apps.map((a) => a.packageName), ['com.example.gamma']);
      store.setQuery('');
      expect(store.apps, hasLength(3));
    });

    test('apps filters by category', () {
      store.setReady(testIndex());
      store.setCategory('System');
      expect(store.apps.map((a) => a.packageName), ['com.example.beta']);
      store.setCategory('Multimedia');
      expect(store.apps.map((a) => a.packageName), ['com.example.alpha']);
      store.setCategory('Nope');
      expect(store.apps, isEmpty);
    });

    test('query and category compose', () {
      store.setReady(testIndex());
      store.setCategory('System');
      store.setQuery('utility');
      expect(store.apps, hasLength(1));
      store.setQuery('zzz');
      expect(store.apps, isEmpty);
    });

    test('sort modes order by recency or name', () {
      store.setReady(testIndex());
      expect(
        store.apps.map((a) => a.packageName),
        ['com.example.alpha', 'com.example.beta', 'com.example.gamma'],
      );
      store.setSort(SortMode.name);
      expect(
        store.apps.map((a) => a.packageName),
        ['com.example.alpha', 'com.example.beta', 'com.example.gamma'],
      );
      // rename scenario: beta's name sorts after alpha's
      expect(store.apps.first.name, 'Alpha Player');
    });

    test('categories come out sorted and distinct', () {
      store.setReady(testIndex());
      expect(store.categories, ['Multimedia', 'System', 'Utilities', 'Writing']);
    });

    test('empty index behaves', () {
      expect(store.apps, isEmpty);
      expect(store.categories, isEmpty);
      expect(store.selected, isNull);
      store.setReady(RepoIndex(
        name: 'empty',
        address: 'https://r',
        timestamp: DateTime.fromMillisecondsSinceEpoch(0, isUtc: true),
        apps: [],
      ));
      expect(store.apps, isEmpty);
    });

    test('setReady clears a category the new index dropped', () {
      store.setReady(testIndex());
      store.setCategory('System');
      store.setReady(RepoIndex(
        name: 'x',
        address: 'https://r',
        timestamp: DateTime.fromMillisecondsSinceEpoch(0, isUtc: true),
        apps: [testIndex().apps.first], // Multimedia only
      ));
      expect(store.category, isNull);
      expect(store.apps, hasLength(1));
    });

    test('setReady clears a selection the new index dropped', () {
      store.setReady(testIndex());
      store.select('com.example.alpha');
      store.setReady(RepoIndex(
        name: 'empty',
        address: 'https://r',
        timestamp: DateTime.fromMillisecondsSinceEpoch(0, isUtc: true),
        apps: [],
      ));
      expect(store.selectedPackage, isNull);
      expect(store.selected, isNull);
    });

    test('selection and lookups', () {
      store.setReady(testIndex());
      store.select('com.example.beta');
      expect(store.selected!.name, 'Beta Tool');
      expect(store.appByPackage('com.example.gamma')!.name, 'Gamma Notes');
      expect(store.appByPackage('missing'), isNull);
      store.select(null);
      expect(store.selected, isNull);
    });

    test('status transitions notify listeners', () {
      var count = 0;
      store.addListener(() => count++);
      store.setLoading();
      expect(store.status, LoadStatus.loading);
      store.setReady(testIndex());
      expect(store.status, LoadStatus.ready);
      store.setError('boom');
      expect(store.status, LoadStatus.error);
      expect(store.error, 'boom');
      expect(count, 3);
    });

    test('no-op setters do not notify', () {
      var count = 0;
      store.addListener(() => count++);
      store.setQuery('');
      store.setCategory(null);
      store.setSort(SortMode.updated);
      store.select(null);
      store.setRepoUrl(Uri.parse('https://repo.example/fdroid/repo'));
      expect(count, 0);
    });

    test('install progress defaults to none', () {
      expect(
        store.progressOf('x').phase,
        InstallPhase.none,
      );
      store.setInstall('x', const InstallProgress(InstallPhase.downloading, progress: 0.5));
      expect(store.progressOf('x').progress, 0.5);
    });
  });

  group('persistence', () {
    test('MemoryPersistence round-trips', () async {
      final p = MemoryPersistence();
      expect(await p.load(), isNull);
      await p.save({'repoUrl': 'https://a'});
      expect(p.saves, 1);
      expect((await p.load())!['repoUrl'], 'https://a');
      final seeded = MemoryPersistence({'repoUrl': 'https://b'});
      expect((await seeded.load())!['repoUrl'], 'https://b');
    });

    test('PrefsPersistence round-trips through SharedPreferences', () async {
      SharedPreferences.setMockInitialValues({});
      final p = await PrefsPersistence.open();
      expect(await p.load(), isNull);
      await p.save({'repoUrl': 'https://custom.example/repo'});
      expect(
        (await p.load())!['repoUrl'],
        'https://custom.example/repo',
      );
      // non-string payloads are ignored
      await p.save({'other': 5});
      expect(
        (await p.load())!['repoUrl'],
        'https://custom.example/repo',
      );
    });

    test('PrefsPersistence loads a seeded value', () async {
      SharedPreferences.setMockInitialValues({'repoUrl': 'https://seeded'});
      final p = await PrefsPersistence.open();
      expect((await p.load())!['repoUrl'], 'https://seeded');
    });
  });

  group('fakes', () {
    test('FakeRepoClient returns its index and records calls', () async {
      final client = FakeRepoClient();
      final index = await client.fetchIndex(Uri.parse('https://a/'));
      expect(index.apps, hasLength(2));
      expect(client.fetches, 1);
      expect(client.lastRepo.toString(), 'https://a/');
      final failing = FakeRepoClient(throwError: StateError('x'));
      expect(() => failing.fetchIndex(Uri.parse('https://a/')), throwsStateError);
    });

    test('FakeDownloader returns a fake file or the set error', () async {
      final dl = FakeDownloader();
      final file = await dl.download(
        Uri.parse('https://r.example/a.apk'),
        'a.apk',
        onProgress: (r, t) {},
      );
      expect(file.path, endsWith('a.apk'));
      expect(dl.downloads.single.path, '/a.apk');
      dl.error = StateError('x');
      expect(
        dl.download(Uri.parse('https://r.example/b.apk'), 'b.apk'),
        throwsStateError,
      );
    });

    test('FakeInstaller records calls', () async {
      final installer = FakeInstaller();
      expect(await installer.installApk('/p'), InstallOutcome.installed);
      expect(installer.installedPaths, ['/p']);
      expect(await installer.openApp('pkg'), isTrue);
      expect(installer.openedPackages, ['pkg']);
      installer.openResult = false;
      expect(await installer.openApp('pkg'), isFalse);
      installer.outcome = InstallOutcome.failed;
      expect(await installer.installApk('/p2'), InstallOutcome.failed);
    });
  });
}
