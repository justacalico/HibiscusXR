import 'dart:async';

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
    this.deviceSdk,
  }) : store = store ?? StoreState(repoUrl: kDefaultRepo);

  /// The public F-Droid repo until the self-hosted one stands up.
  static final kDefaultRepo = Uri.parse('https://f-droid.org/repo');

  /// How long an install may sit in PackageInstaller before we give up.
  static const installTimeout = Duration(minutes: 2);

  final RepoClient client;
  final ApkDownloader downloader;
  final ApkInstaller installer;
  final StorePersistence persistence;
  final StoreState store;
  ImageResolver imageResolver;

  /// The API level installs target; null installs whatever the repo
  /// calls newest. The image build passes [kImageSdk].
  final int? deviceSdk;

  bool _started = false;
  int _fetchGen = 0;

  /// API level of the Hibiscus system image (Android 10 / LineageOS
  /// 17.1 on sdm845). Used to pick compatible versions.
  static const kImageSdk = 29;

  /// Where repo-relative files resolve: the index's own address wins,
  /// falling back to the configured URL when the field is empty.
  Uri get repoBase {
    final address = store.index?.address ?? '';
    return address.isNotEmpty
        ? Uri.parse(address)
        : store.repoUrl;
  }

  /// Restore the persisted repo URL and pull the index. Safe to call
  /// once. A broken persistence layer downgrades to defaults instead of
  /// wedging the app on the loading spinner.
  Future<void> start() async {
    if (_started) return;
    _started = true;
    final saved = await persistence.load().onError((_, _) => null);
    final savedUrl = saved?['repoUrl'];
    if (savedUrl is String) {
      final parsed = Uri.tryParse(savedUrl);
      if (parsed != null) store.repoUrl = parsed;
    }
    await refresh();
  }

  /// Fetch the index. A later call supersedes an in-flight one - the
  /// stale response is discarded, so a repo switch can't resurrect the
  /// old catalog.
  Future<void> refresh() async {
    final gen = ++_fetchGen;
    store.setLoading();
    try {
      final index = await client.fetchIndex(store.repoUrl);
      if (gen != _fetchGen) return;
      store.setReady(index);
    } catch (e) {
      if (gen != _fetchGen) return;
      store.setError('$e');
    }
  }

  /// Point the catalog at a different F-Droid compatible repository,
  /// persist it, and reload.
  Future<void> setRepoUrl(String raw) async {
    final uri = Uri.parse(raw);
    store.setRepoUrl(uri);
    unawaited(persistence.save({'repoUrl': uri.toString()}));
    await refresh();
  }

  /// Download the newest compatible build of [app] and hand it to the
  /// installer. Progress lands in [StoreState.installs] so every surface
  /// showing the app updates together.
  Future<void> install(RepoApp app) async {
    final version = deviceSdk != null
        ? app.compatibleVersion(deviceSdk!)
        : app.latest;
    if (version == null || version.apkPath.isEmpty) return;
    final pkg = app.packageName;
    final inFlight = store.progressOf(pkg).phase;
    if (inFlight == InstallPhase.downloading ||
        inFlight == InstallPhase.installing) {
      return;
    }
    store.setInstall(pkg, const InstallProgress(InstallPhase.downloading));
    try {
      final file = await downloader.download(
        repoFileUri(repoBase, version.apkPath),
        '${app.packageName}_${version.versionCode}.apk',
        expectedSha256: version.sha256,
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
      final outcome = await installer
          .installApk(file.path)
          .timeout(installTimeout);
      store.setInstall(
        pkg,
        switch (outcome) {
          InstallOutcome.installed =>
            const InstallProgress(InstallPhase.installed),
          // the system dialog is up - the outcome is unknown until the
          // user answers it, so don't pretend the install happened
          InstallOutcome.prompted =>
            const InstallProgress(InstallPhase.prompted),
          InstallOutcome.failed => const InstallProgress(InstallPhase.failed),
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

  void dispose() {
    store.dispose();
    client.dispose();
    downloader.dispose();
  }
}
