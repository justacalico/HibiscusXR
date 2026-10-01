import 'dart:io';

import 'package:crypto/crypto.dart';
import 'package:http/http.dart' as http;

/// Thrown when an APK download fails partway through or fails integrity.
class DownloadException implements Exception {
  const DownloadException(this.message);

  final String message;

  @override
  String toString() => 'DownloadException: $message';
}

/// Streams APK files into a local directory so the installer can hand
/// them to PackageInstaller. The directory is injectable so tests can
/// use temp space.
class ApkDownloader {
  ApkDownloader({http.Client? client, required this.directory})
    : _client = client ?? http.Client(),
      _ownsClient = client == null;

  /// A repo that never answers stalls the download phase forever
  /// otherwise.
  static const connectTimeout = Duration(seconds: 30);

  final http.Client _client;
  final bool _ownsClient;
  final Directory directory;

  /// Download [url] to `apks/<fileName>` under [directory], reporting
  /// bytes received via [onProgress] as the stream lands. When
  /// [expectedSha256] is set (the index ships it per version), the bytes
  /// are verified before the file is returned - a mismatch deletes the
  /// file and throws.
  Future<File> download(
    Uri url,
    String fileName, {
    String? expectedSha256,
    void Function(int received, int total)? onProgress,
  }) async {
    http.StreamedResponse res;
    try {
      res = await _client
          .send(http.Request('GET', url))
          .timeout(connectTimeout);
    } catch (e) {
      throw DownloadException('$url unreachable: $e');
    }
    if (res.statusCode != 200) {
      throw DownloadException('$url returned ${res.statusCode}');
    }
    final dir = Directory('${directory.path}/apks')
      ..createSync(recursive: true);
    final file = File('${dir.path}/$fileName');
    final sink = file.openWrite();
    var received = 0;
    try {
      await for (final chunk in res.stream) {
        sink.add(chunk);
        received += chunk.length;
        onProgress?.call(received, res.contentLength ?? -1);
      }
      await sink.flush();
    } catch (e) {
      throw DownloadException('$url failed mid-download: $e');
    } finally {
      await sink.close();
    }
    if (expectedSha256 != null) {
      final digest = await sha256.bind(file.openRead()).first;
      if (digest.toString() != expectedSha256.toLowerCase()) {
        await file.delete();
        throw DownloadException('$url checksum mismatch');
      }
    }
    return file;
  }

  void dispose() {
    if (_ownsClient) _client.close();
  }
}
