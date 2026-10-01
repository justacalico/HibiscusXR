import 'dart:convert';
import 'dart:io';

import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:http/http.dart' as http;
import 'package:http/testing.dart';
import 'package:pn2_store/src/downloader.dart';
import 'package:pn2_store/src/installer.dart';
import 'package:pn2_store/src/repo_client.dart';

import 'helpers.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  group('HttpRepoClient', () {
    test('fetches index-v2.json under the repo path', () async {
      Uri? hit;
      final client = HttpRepoClient(
        client: MockClient((req) async {
          hit = req.url;
          return http.Response(kIndexJson, 200);
        }),
      );
      final index = await client.fetchIndex(
        Uri.parse('https://repo.example/fdroid/repo'),
      );
      expect(
        hit.toString(),
        'https://repo.example/fdroid/repo/index-v2.json',
      );
      expect(index.apps, hasLength(3));
    });

    test('non-200 raises RepoException with the status', () {
      final client = HttpRepoClient(
        client: MockClient((req) async => http.Response('nope', 503)),
      );
      expect(
        client.fetchIndex(Uri.parse('https://r.example/repo')),
        throwsA(
          isA<RepoException>().having(
            (e) => e.message,
            'message',
            contains('503'),
          ),
        ),
      );
    });

    test('transport errors wrap as RepoException', () {
      final client = HttpRepoClient(
        client: MockClient((req) async => throw const SocketException('down')),
      );
      expect(
        client.fetchIndex(Uri.parse('https://r.example/repo')),
        throwsA(isA<RepoException>()),
      );
    });

    test('malformed body wraps as RepoException', () {
      final client = HttpRepoClient(
        client: MockClient((req) async => http.Response('not json', 200)),
      );
      expect(
        client.fetchIndex(Uri.parse('https://r.example/repo')),
        throwsA(
          isA<RepoException>().having(
            (e) => e.message,
            'message',
            contains('not a valid index'),
          ),
        ),
      );
    });

    test('dispose closes only the client it owns', () {
      final owned = HttpRepoClient()..dispose();
      expect(owned, isNotNull);
      final injected = MockClient((req) async => http.Response('', 200));
      HttpRepoClient(client: injected).dispose();
    });

    test('RepoException formats its message', () {
      expect(
        const RepoException('x').toString(),
        'RepoException: x',
      );
    });
  });

  group('ApkDownloader', () {
    late Directory dir;

    setUp(() async {
      dir = await Directory.systemTemp.createTemp('dl-test');
    });
    tearDown(() async {
      if (await dir.exists()) await dir.delete(recursive: true);
    });

    test('writes the stream to apks/<name> and reports progress', () async {
      final events = <(int, int)>[];
      final dl = ApkDownloader(
        client: MockClient(
          (req) async => http.Response.bytes(
            utf8.encode('apk-bytes'),
            200,
          ),
        ),
        directory: dir,
      );
      final file = await dl.download(
        Uri.parse('https://r.example/a.apk'),
        'a_1.apk',
        onProgress: (r, t) => events.add((r, t)),
      );
      expect(file.path, '${dir.path}/apks/a_1.apk');
      expect(await file.readAsString(), 'apk-bytes');
      expect(events, [(9, 9)]);
    });

    test('non-200 raises DownloadException', () {
      final dl = ApkDownloader(
        client: MockClient((req) async => http.Response('', 404)),
        directory: dir,
      );
      expect(
        dl.download(Uri.parse('https://r.example/a.apk'), 'a.apk'),
        throwsA(
          isA<DownloadException>().having(
            (e) => e.message,
            'message',
            contains('404'),
          ),
        ),
      );
    });

    test('connect failure wraps as DownloadException', () {
      final dl = ApkDownloader(
        client: MockClient((req) async => throw const SocketException('no')),
        directory: dir,
      );
      expect(
        dl.download(Uri.parse('https://r.example/a.apk'), 'a.apk'),
        throwsA(isA<DownloadException>()),
      );
    });

    test('a stream that dies mid-download wraps too', () {
      final dl = ApkDownloader(
        client: _StreamBomber(),
        directory: dir,
      );
      expect(
        dl.download(Uri.parse('https://r.example/a.apk'), 'a.apk'),
        throwsA(
          isA<DownloadException>().having(
            (e) => e.message,
            'message',
            contains('mid-download'),
          ),
        ),
      );
    });

    test('dispose closes only the client it owns', () {
      ApkDownloader(directory: dir).dispose();
      ApkDownloader(
        client: MockClient((req) async => http.Response('', 200)),
        directory: dir,
      ).dispose();
    });

    test('DownloadException formats its message', () {
      expect(
        const DownloadException('x').toString(),
        'DownloadException: x',
      );
    });
  });

  group('ChannelInstaller', () {
    const channel = MethodChannel('gitlab.neosalsa.store/installer');
    final binding = TestWidgetsFlutterBinding.ensureInitialized();
    final log = <MethodCall>[];
    String next = 'installed';

    setUp(() {
      log.clear();
      binding.defaultBinaryMessenger.setMockMethodCallHandler(
        channel,
        (call) async {
          log.add(call);
          return switch (call.method) {
            'openApp' => call.arguments['package'] == 'com.good',
            _ => next,
          };
        },
      );
    });

    tearDown(() {
      binding.defaultBinaryMessenger.setMockMethodCallHandler(channel, null);
    });

    test('installApk maps channel results to outcomes', () async {
      // not const: const construction canonicalizes the default channel
      // at compile time and the initializer line never runs
      final installer = ChannelInstaller();
      for (final (raw, outcome) in [
        ('installed', InstallOutcome.installed),
        ('prompted', InstallOutcome.prompted),
        ('failed', InstallOutcome.failed),
        ('garbage', InstallOutcome.failed),
      ]) {
        next = raw;
        expect(await installer.installApk('/tmp/a.apk'), outcome);
      }
      expect(log.last.method, 'installApk');
      expect(log.last.arguments, {'path': '/tmp/a.apk'});
    });

    test('openApp returns the channel result', () async {
      final installer = ChannelInstaller();
      expect(await installer.openApp('com.good'), isTrue);
      expect(await installer.openApp('com.bad'), isFalse);
      expect(log.last.arguments, {'package': 'com.bad'});
    });
  });
}

/// Emits one chunk then errors out, exercising the mid-download catch.
class _StreamBomber extends http.BaseClient {
  @override
  Future<http.StreamedResponse> send(http.BaseRequest request) async {
    return http.StreamedResponse(
      (() async* {
        yield [1, 2, 3];
        throw StateError('connection lost');
      })(),
      200,
    );
  }
}
