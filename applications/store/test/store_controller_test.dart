import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:http/http.dart' as http;
import 'package:http/testing.dart';
import 'package:pn2_store/src/fdroid_index.dart';
import 'package:pn2_store/src/installer.dart';
import 'package:pn2_store/src/platform/fakes.dart';
import 'package:pn2_store/src/persistence.dart';
import 'package:pn2_store/src/repo_client.dart';
import 'package:pn2_store/src/store_controller.dart';
import 'package:pn2_store/src/store_state.dart';
import 'package:pn2_store/src/downloader.dart';

import 'helpers.dart';

void main() {
  group('StoreController.start', () {
    test('uses the default repo and fetches the index', () async {
      final client = FakeRepoClient(index: testIndex());
      final c = StoreController(
        client: client,
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: MemoryPersistence(),
      );
      await c.start();
      expect(client.fetches, 1);
      expect(client.lastRepo, StoreController.kDefaultRepo);
      expect(c.store.status, LoadStatus.ready);
      expect(c.store.apps, hasLength(3));
    });

    test('restores a persisted repo url', () async {
      final client = FakeRepoClient(index: testIndex());
      final c = StoreController(
        client: client,
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: MemoryPersistence({'repoUrl': 'https://self.hosted/repo'}),
      );
      await c.start();
      expect(client.lastRepo, Uri.parse('https://self.hosted/repo'));
      expect(c.store.repoUrl, Uri.parse('https://self.hosted/repo'));
    });

    test('ignores a persisted url that does not parse', () async {
      final client = FakeRepoClient(index: testIndex());
      final c = StoreController(
        client: client,
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: MemoryPersistence({'repoUrl': '::nope'}),
      );
      await c.start();
      expect(client.lastRepo, StoreController.kDefaultRepo);
    });

    test('a second start is a no-op', () async {
      final client = FakeRepoClient(index: testIndex());
      final c = StoreController(
        client: client,
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: MemoryPersistence(),
      );
      await c.start();
      await c.start();
      expect(client.fetches, 1);
    });

    test('a persistence failure falls back to defaults', () async {
      final client = FakeRepoClient(index: testIndex());
      final c = StoreController(
        client: client,
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: _BrokenPersistence(),
      );
      await c.start();
      expect(client.lastRepo, StoreController.kDefaultRepo);
      expect(c.store.status, LoadStatus.ready);
    });
  });

  group('StoreController.refresh', () {
    test('repo errors surface on the store', () async {
      final c = StoreController(
        client: FakeRepoClient(throwError: const RepoException('offline')),
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: MemoryPersistence(),
      );
      await c.start();
      expect(c.store.status, LoadStatus.error);
      expect(c.store.error, contains('offline'));
      (c.client as FakeRepoClient).throwError = null;
      await c.refresh();
      expect(c.store.status, LoadStatus.ready);
    });

    test('non-repo exceptions land in the error string', () async {
      final c = StoreController(
        client: FakeRepoClient(throwError: StateError('weird')),
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: MemoryPersistence(),
      );
      await c.start();
      expect(c.store.status, LoadStatus.error);
      expect(c.store.error, contains('weird'));
    });
  });

  group('StoreController.setRepoUrl', () {
    test('persists and refetches against the new repo', () async {
      final client = FakeRepoClient(index: testIndex());
      final persistence = MemoryPersistence();
      final c = StoreController(
        client: client,
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: persistence,
      );
      await c.start();
      await c.setRepoUrl('https://home.example/fdroid/repo');
      expect(c.store.repoUrl, Uri.parse('https://home.example/fdroid/repo'));
      expect(client.lastRepo, Uri.parse('https://home.example/fdroid/repo'));
      expect(
        persistence.stored!['repoUrl'],
        'https://home.example/fdroid/repo',
      );
    });
  });

  group('StoreController.install', () {
    Future<StoreController> installing({
      required FakeInstaller installer,
      http.Client? downloaderClient,
      StoreState? store,
    }) async {
      final c = StoreController(
        client: FakeRepoClient(index: testIndex()),
        downloader: ApkDownloader(
          client: downloaderClient ??
              MockClient((req) async => http.Response.bytes(
                    utf8.encode('apk'),
                    200,
                  )),
          directory: Directory.systemTemp,
        ),
        installer: installer,
        persistence: MemoryPersistence(),
        store: store,
      );
      await c.start();
      return c;
    }

    test('downloads then installs and lands on installed', () async {
      final installer = FakeInstaller();
      final c = await installing(installer: installer);
      final alpha = c.store.appByPackage('com.example.alpha')!;
      await c.install(alpha);
      expect(installer.installedPaths.single, endsWith('com.example.alpha_12.apk'));
      expect(
        c.store.progressOf('com.example.alpha').phase,
        InstallPhase.installed,
      );
    });

    test('a prompted outcome lands on prompted, not installed', () async {
      final installer = FakeInstaller(outcome: InstallOutcome.prompted);
      final c = await installing(installer: installer);
      await c.install(c.store.appByPackage('com.example.beta')!);
      expect(
        c.store.progressOf('com.example.beta').phase,
        InstallPhase.prompted,
      );
    });

    test('installer failure lands on failed', () async {
      final installer = FakeInstaller(outcome: InstallOutcome.failed);
      final c = await installing(installer: installer);
      await c.install(c.store.appByPackage('com.example.beta')!);
      final p = c.store.progressOf('com.example.beta');
      expect(p.phase, InstallPhase.failed);
      expect(p.error, isNull);
    });

    test('download failure lands on failed with the error', () async {
      final installer = FakeInstaller();
      final c = await installing(
        installer: installer,
        downloaderClient:
            MockClient((req) async => http.Response('gone', 410)),
      );
      await c.install(c.store.appByPackage('com.example.beta')!);
      final p = c.store.progressOf('com.example.beta');
      expect(p.phase, InstallPhase.failed);
      expect(p.error, contains('410'));
      expect(installer.installedPaths, isEmpty);
    });

    test('apps with no versions do nothing', () async {
      final installer = FakeInstaller();
      final c = await installing(installer: installer);
      await c.install(c.store.appByPackage('com.example.gamma')!);
      expect(installer.installedPaths, isEmpty);
      expect(
        c.store.progressOf('com.example.gamma').phase,
        InstallPhase.none,
      );
    });

    test('a second install while in flight is ignored', () async {
      final installer = FakeInstaller();
      final store = StoreState(repoUrl: StoreController.kDefaultRepo);
      final c = await installing(installer: installer, store: store);
      final app = c.store.appByPackage('com.example.beta')!;
      store.setInstall(
        'com.example.beta',
        const InstallProgress(InstallPhase.downloading),
      );
      await c.install(app);
      store.setInstall(
        'com.example.beta',
        const InstallProgress(InstallPhase.installing),
      );
      await c.install(app);
      expect(installer.installedPaths, isEmpty);
    });

    test('openApp delegates to the installer', () async {
      final installer = FakeInstaller();
      final c = await installing(installer: installer);
      expect(await c.openApp('com.example.alpha'), isTrue);
      expect(installer.openedPackages, ['com.example.alpha']);
    });

    test('dispose tears down the store', () {
      final c = testController();
      c.dispose();
      // a disposed ChangeNotifier throws when asked to notify
      expect(() => c.store.setQuery('x'), throwsA(anything));
    });
  });

  group('refresh races', () {
    test('a stale fetch never overwrites a newer one', () async {
      final client = _ControlledClient();
      final c = StoreController(
        client: client,
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: MemoryPersistence(),
      );
      final started = c.start();
      // let start() reach its first fetchIndex before the repo switch
      await Future.delayed(Duration.zero);
      expect(client.pending, hasLength(1));
      final switchRepo = c.setRepoUrl('https://new.example/repo');
      expect(client.pending, hasLength(2));
      // gen2's answer lands first, then the stale gen1 answer
      client.pending[1].complete(testIndex());
      await switchRepo;
      client.pending[0].complete(
        RepoIndex(
          name: 'stale',
          address: 'https://old',
          timestamp: DateTime.utc(2020),
          apps: [],
        ),
      );
      await started;
      expect(c.store.index!.name, 'Test Repo');
      expect(c.store.apps, hasLength(3));
    });

    test('a stale fetch error does not clobber a good newer index',
        () async {
      final client = _ControlledClient();
      final c = StoreController(
        client: client,
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: MemoryPersistence(),
      );
      final started = c.start();
      await Future.delayed(Duration.zero);
      final switchRepo = c.setRepoUrl('https://new.example/repo');
      client.pending[1].complete(testIndex());
      await switchRepo;
      client.pending[0].completeError(const RepoException('old broke'));
      await started;
      expect(c.store.status, LoadStatus.ready);
      expect(c.store.apps, hasLength(3));
    });
  });

  group('repo base', () {
    test('an empty index address falls back to the configured repo', () async {
      final emptyAddress = RepoIndex(
        name: 'x',
        address: '',
        timestamp: DateTime.utc(2020),
        apps: [],
      );
      final c = testController(index: emptyAddress);
      await c.start();
      expect(c.repoBase, StoreController.kDefaultRepo);
    });
  });

  group('install picks compatible builds', () {
    test('deviceSdk skips versions above it', () async {
      final app = RepoApp(
        packageName: 'x',
        name: 'x',
        summary: '',
        description: '',
        categories: [],
        added: DateTime.utc(2026),
        lastUpdated: DateTime.utc(2026),
        versions: [
          AppVersion(
            versionCode: 2,
            versionName: '2.0',
            apkPath: '/x_2.apk',
            size: 1,
            added: DateTime.utc(2026),
            minSdk: 30,
          ),
          AppVersion(
            versionCode: 1,
            versionName: '1.0',
            apkPath: '/x_1.apk',
            size: 1,
            added: DateTime.utc(2026),
            minSdk: 24,
            maxSdk: 29,
          ),
        ],
      );
      expect(app.compatibleVersion(29)!.versionCode, 1);
      expect(app.compatibleVersion(35)!.versionCode, 2);
      final unconstrained = RepoApp(
        packageName: 'y',
        name: 'y',
        summary: '',
        description: '',
        categories: [],
        added: DateTime.utc(2026),
        lastUpdated: DateTime.utc(2026),
        versions: [
          AppVersion(
            versionCode: 1,
            versionName: '1',
            apkPath: '/y.apk',
            size: 1,
            added: DateTime.utc(2026),
          ),
        ],
      );
      expect(unconstrained.compatibleVersion(1)!.versionCode, 1);
      expect(unconstrained.compatibleVersion(999)!.versionCode, 1);
      expect(unconstrained.latest!.versionCode, 1);
      final empty = RepoApp(
        packageName: 'z',
        name: 'z',
        summary: '',
        description: '',
        categories: [],
        added: DateTime.utc(2026),
        lastUpdated: DateTime.utc(2026),
        versions: [],
      );
      expect(empty.compatibleVersion(29), isNull);
    });

    test('install uses the compatible version when deviceSdk is set', () async {
      final index = RepoIndex(
        name: 'x',
        address: 'https://r.example/repo',
        timestamp: DateTime.utc(2026),
        apps: [app31Only],
      );
      final installer = FakeInstaller();
      final downloader = FakeDownloader();
      final c = StoreController(
        client: FakeRepoClient(index: index),
        downloader: downloader,
        installer: installer,
        persistence: MemoryPersistence(),
        deviceSdk: 29,
      );
      await c.start();
      await c.install(c.store.appByPackage('a.b')!);
      expect(downloader.downloads.single.path, '/repo/a.b_1.apk');
    });
  });
}

final app31Only = RepoApp(
  packageName: 'a.b',
  name: 'a',
  summary: '',
  description: '',
  categories: [],
  added: DateTime.utc(2026),
  lastUpdated: DateTime.utc(2026),
  versions: [
    AppVersion(
      versionCode: 2,
      versionName: '2',
      apkPath: '/a.b_2.apk',
      size: 1,
      added: DateTime.utc(2026),
      minSdk: 31,
    ),
    AppVersion(
      versionCode: 1,
      versionName: '1',
      apkPath: '/a.b_1.apk',
      size: 1,
      added: DateTime.utc(2026),
    ),
  ],
);


class _BrokenPersistence implements StorePersistence {
  @override
  Future<Map<String, dynamic>?> load() => Future.error(StateError('dead'));

  @override
  Future<void> save(Map<String, dynamic> snapshot) async {}
}

/// fetchIndex futures come off a queue the test fills.
class _ControlledClient implements RepoClient {
  final pending = <Completer<RepoIndex>>[];

  @override
  Future<RepoIndex> fetchIndex(Uri repo) {
    final c = Completer<RepoIndex>();
    pending.add(c);
    return c.future;
  }

  @override
  void dispose() {}
}
