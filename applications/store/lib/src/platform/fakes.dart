import 'dart:io';

import '../downloader.dart';
import '../fdroid_index.dart';
import '../installer.dart';
import '../repo_client.dart';

/// Canned catalog for the non-Android preview and widget tests. The
/// index a constructor takes is returned verbatim, so tests shape the
/// data they need.
class FakeRepoClient implements RepoClient {
  FakeRepoClient({RepoIndex? index, this.throwError})
    : index = index ?? previewIndex;

  /// Two seeded apps so `flutter run` on the desktop has something to
  /// show - the real Android build always fetches over HTTP.
  static final previewIndex = RepoIndex(
    name: 'preview repo',
    address: 'https://repo.example/fdroid/repo',
    timestamp: DateTime.utc(2026, 9, 1),
    apps: [
      RepoApp(
        packageName: 'com.example.dialer',
        name: 'Dialer',
        summary: 'A plain phone dialer',
        description: 'Calls people.',
        categories: const ['System'],
        added: DateTime.utc(2026, 1, 1),
        lastUpdated: DateTime.utc(2026, 9, 1),
        author: 'Example Inc',
        license: 'MIT',
        versions: [
          AppVersion(
            versionCode: 3,
            versionName: '1.2',
            apkPath: '/com.example.dialer_3.apk',
            size: 2048,
            added: DateTime.fromMillisecondsSinceEpoch(0, isUtc: true),
            minSdk: 24,
          ),
        ],
      ),
      RepoApp(
        packageName: 'com.example.paint',
        name: 'Paint',
        summary: 'Sketches on a canvas',
        description: 'Draws things.',
        categories: const ['Graphics'],
        added: DateTime.utc(2026, 2, 1),
        lastUpdated: DateTime.utc(2026, 8, 20),
        versions: [
          AppVersion(
            versionCode: 5,
            versionName: '2.0',
            apkPath: '/com.example.paint_5.apk',
            size: 4096,
            added: DateTime.fromMillisecondsSinceEpoch(0, isUtc: true),
          ),
        ],
      ),
    ],
  );


  final RepoIndex index;
  Object? throwError;
  var fetches = 0;
  Uri? lastRepo;

  @override
  Future<RepoIndex> fetchIndex(Uri repo) async {
    fetches++;
    lastRepo = repo;
    final error = throwError;
    if (error != null) throw error;
    return index;
  }

  @override
  void dispose() {}
}

/// A download that resolves instantly without touching the disk - real
/// dart:io writes never complete inside the widget-test fake zone.
class FakeDownloader extends ApkDownloader {
  FakeDownloader() : super(directory: Directory.systemTemp);

  Object? error;
  final List<Uri> downloads = [];

  @override
  Future<File> download(
    Uri url,
    String fileName, {
    String? expectedSha256,
    void Function(int received, int total)? onProgress,
  }) {
    downloads.add(url);
    onProgress?.call(1, 1);
    final e = error;
    if (e != null) return Future.error(e);
    return Future.value(File('${directory.path}/apks/$fileName'));
  }
}

/// Records install calls instead of touching PackageInstaller.
class FakeInstaller implements ApkInstaller {
  FakeInstaller({
    this.outcome = InstallOutcome.installed,
    this.openResult = true,
  });

  InstallOutcome outcome;
  bool openResult;
  final List<String> installedPaths = [];
  final List<String> openedPackages = [];

  @override
  Future<InstallOutcome> installApk(String path) async {
    installedPaths.add(path);
    return outcome;
  }

  @override
  Future<bool> openApp(String packageName) async {
    openedPackages.add(packageName);
    return openResult;
  }
}
