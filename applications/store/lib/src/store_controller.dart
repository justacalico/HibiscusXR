import 'package:flutter/widgets.dart';

import 'downloader.dart';
import 'fdroid_index.dart';
import 'installer.dart';
import 'persistence.dart';
import 'repo_client.dart';
import 'store_state.dart';

/// How an app icon or screenshot URL becomes an [ImageProvider]. The
/// default resolves over the network; tests substitute fixed bytes so
/// goldens stay deterministic.
typedef ImageResolver = ImageProvider? Function(String url);

ImageProvider? networkImageResolver(String url) => NetworkImage(url);

/// Glue between [RepoClient]/[ApkDownloader]/[ApkInstaller] and the
/// [StoreState]. Holds no layout decisions.
class StoreController {
  StoreController({
    required this.client,
    required this.downloader,
    required this.installer,
    required this.persistence,
    StoreState? store,
    this.imageResolver = networkImageResolver,
  }) : store = store ?? StoreState(repoUrl: kDefaultRepo);

  /// The public F-Droid repo until the self-hosted one stands up.
  static final kDefaultRepo = Uri.parse('https://f-droid.org/repo');

  final RepoClient client;
  final ApkDownloader downloader;
  final ApkInstaller installer;
  final StorePersistence persistence;
  final StoreState store;
  ImageResolver imageResolver;

  bool _started = false;

  /// Restore the persisted repo URL and pull the index. Safe to call
  /// once.
  Future<void> start() async {
    if (_started) return;
    _started = true;
    final saved = await persistence.load();
    final savedUrl = saved?['repoUrl'];
    if (savedUrl is String) {
      final parsed = Uri.tryParse(savedUrl);
      if (parsed != null) store.repoUrl = parsed;
    }
    await refresh();
  }

  Future<void> refresh() async {
    store.setLoading();
    try {
      store.setReady(await client.fetchIndex(store.repoUrl));
    } on RepoException catch (e) {
      store.setError(e.message);
    } catch (e) {
      store.setError('$e');
    }
  }

  /// Point the catalog at a different F-Droid compatible repository,
  /// persist it, and reload.
  Future<void> setRepoUrl(String raw) async {
    final uri = Uri.parse(raw);
    store.setRepoUrl(uri);
    await persistence.save({'repoUrl': uri.toString()});
    await refresh();
  }

  /// Download the latest build of [app] and hand it to the installer.
  /// Progress lands in [StoreState.installs] so every surface showing
  /// the app updates together.
  Future<void> install(StoreApp app) async {
    final latest = app.latest;
    if (latest == null) return;
    final pkg = app.packageName;
    if (store.progressOf(pkg).phase == InstallPhase.downloading ||
        store.progressOf(pkg).phase == InstallPhase.installing) {
      return;
    }
    store.setInstall(pkg, const InstallProgress(InstallPhase.downloading));
    try {
      final file = await downloader.download(
        repoFileUri(Uri.parse(store.index?.address ?? store.repoUrl.toString()),
            latest.apkPath),
        '${app.packageName}_${latest.versionCode}.apk',
        onProgress: (received, total) {
          store.setInstall(
            pkg,
            InstallProgress(
              InstallPhase.downloading,
              progress: total > 0 ? received / total : 0,
            ),
          );
        },
      );
      store.setInstall(pkg, const InstallProgress(InstallPhase.installing));
      final outcome = await installer.installApk(file.path);
      store.setInstall(
        pkg,
        switch (outcome) {
          InstallOutcome.installed =>
            const InstallProgress(InstallPhase.installed),
          InstallOutcome.prompted =>
            const InstallProgress(InstallPhase.installed),
          InstallOutcome.failed => const InstallProgress(
              InstallPhase.failed,
              error: 'install failed',
            ),
        },
      );
    } catch (e) {
      store.setInstall(
        pkg,
        InstallProgress(InstallPhase.failed, error: '$e'),
      );
    }
  }

  /// Launch an installed app through the platform. Returns whether the
  /// system found a launcher activity for it.
  Future<bool> openApp(String packageName) =>
      installer.openApp(packageName);

  void dispose() => store.dispose();
}
