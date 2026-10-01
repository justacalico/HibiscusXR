import 'dart:convert';
import 'package:flutter/widgets.dart';
import 'package:pn2_store/src/fdroid_index.dart';
import 'package:pn2_store/src/store_controller.dart';
import 'package:pn2_store/src/platform/fakes.dart';
import 'package:pn2_store/src/persistence.dart';
import 'package:pn2_store/src/downloader.dart';

/// A flat blue tile, small enough to inline. Every network image in the
/// catalog resolves to this in tests so goldens stay deterministic.
final kTestImageBytes = base64Decode(
  'iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAAGUlEQVR4nGP0m/OfgRTARJLqUQ2jGoaUBgAJdwIJZT60lgAAAABJRU5ErkJggg==',
);

ImageProvider testImageResolver(String url) => MemoryImage(kTestImageBytes);

/// A small index-v2 document exercising localized fields, categories,
/// screenshots and multiple versions.
const kIndexJson = '''
{
  "repo": {
    "name": {"en-US": "Test Repo"},
    "address": "https://repo.example/fdroid/repo",
    "timestamp": 1756700000000
  },
  "packages": {
    "com.example.alpha": {
      "metadata": {
        "name": {"en-US": "Alpha Player"},
        "summary": {"en-US": "Plays things"},
        "description": {"en-US": "<p>Long <b>about</b> text.</p>"},
        "authorName": "Example Inc",
        "license": "GPL-3.0-only",
        "categories": ["Multimedia"],
        "added": 1730000000000,
        "lastUpdated": 1756700000000,
        "icon": {"en-US": {"name": "icons/alpha.png"}},
        "screenshots": {
          "phone": {"en-US": [{"name": "phone/a1.png"}, {"name": "phone/a2.png"}]}
        }
      },
      "versions": {
        "hash-one": {
          "added": 1756700000000,
          "file": {"name": "/com.example.alpha_12.apk", "size": 1500000},
          "manifest": {"versionName": "1.2", "versionCode": 12, "usesSdk": {"minSdkVersion": 24}}
        },
        "hash-zero": {
          "added": 1730000000000,
          "file": {"name": "/com.example.alpha_10.apk", "size": 1200000},
          "manifest": {"versionName": "1.0", "versionCode": 10}
        }
      }
    },
    "com.example.beta": {
      "metadata": {
        "name": {"de": "Beta Werkzeug", "en-US": "Beta Tool"},
        "summary": {"en-US": "Does utility things"},
        "categories": ["System", "Utilities"],
        "lastUpdated": 1750000000000
      },
      "versions": {
        "hash-two": {
          "file": {"name": "beta/beta_7.apk", "size": 500, "sha256": "dd37c2d7274f7ea982cb83390c36918fee9ce8889073c44b68cdc00bdb8c3e04"},
          "manifest": {"versionName": "0.7", "versionCode": 7}
        }
      }
    },
    "com.example.gamma": {
      "metadata": {
        "name": {"en-US": "Gamma Notes"},
        "summary": {"en-US": "Takes notes"},
        "categories": ["Writing"],
        "lastUpdated": 1740000000000
      },
      "versions": {}
    }
  }
}
''';

RepoIndex testIndex() => decodeFdroidIndex(kIndexJson);

/// A controller wired for tests: canned index, memory persistence, a
/// MockClient downloader that returns bytes, recording installer.
StoreController testController({
  RepoIndex? index,
  Object? throwError,
  StorePersistence? persistence,
  FakeInstaller? installer,
  ApkDownloader? downloader,
}) {
  return StoreController(
    client: FakeRepoClient(index: index ?? testIndex(), throwError: throwError),
    downloader: downloader ?? FakeDownloader(),
    installer: installer ?? FakeInstaller(),
    persistence: persistence ?? MemoryPersistence(),
    imageResolver: testImageResolver,
  );
}

/// Same, but run through start() so the store is ready.
Future<StoreController> readyController({
  RepoIndex? index,
  StorePersistence? persistence,
  FakeInstaller? installer,
  FakeDownloader? downloader,
}) async {
  final c = testController(
    index: index,
    persistence: persistence,
    installer: installer,
    downloader: downloader,
  );
  await c.start();
  return c;
}
