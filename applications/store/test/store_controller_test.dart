import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:http/http.dart' as http;
import 'package:http/testing.dart';
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
      final c = testController();
      // swap in the recording client
      final c2 = StoreController(
        client: client,
        downloader: ApkDownloader(directory: Directory.systemTemp),
        installer: FakeInstaller(),
        persistence: MemoryPersistence(),
      );
      await c.start();
      await c2.start();
      await c2.start();
      expect(client.fetches, 1);
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
      expect(c.store.error, 'offline');
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

    test('a prompted outcome still lands on installed', () async {
      final installer = FakeInstaller(outcome: InstallOutcome.prompted);
      final c = await installing(installer: installer);
      await c.install(c.store.appByPackage('com.example.beta')!);
      expect(
        c.store.progressOf('com.example.beta').phase,
        InstallPhase.installed,
      );
    });

    test('installer failure lands on failed', () async {
      final installer = FakeInstaller(outcome: InstallOutcome.failed);
      final c = await installing(installer: installer);
      await c.install(c.store.appByPackage('com.example.beta')!);
      final p = c.store.progressOf('com.example.beta');
      expect(p.phase, InstallPhase.failed);
      expect(p.error, 'install failed');
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
}
