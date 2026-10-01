import 'package:http/http.dart' as http;

import 'fdroid_index.dart';

/// Thrown when the repository index cannot be fetched or parsed.
class RepoException implements Exception {
  const RepoException(this.message);

  final String message;

  @override
  String toString() => 'RepoException: $message';
}

/// Where the catalog comes from. The HTTP implementation is the default;
/// tests and the desktop preview substitute canned data.
abstract class RepoClient {
  Future<RepoIndex> fetchIndex(Uri repo);
}

class HttpRepoClient implements RepoClient {
  HttpRepoClient({
    http.Client? client,
    this.timeout = const Duration(seconds: 60),
  }) : _client = client ?? http.Client(),
       _ownsClient = client == null;

  final http.Client _client;
  final bool _ownsClient;
  final Duration timeout;

  @override
  Future<RepoIndex> fetchIndex(Uri repo) async {
    final url = repoFileUri(repo, 'index-v2.json');
    http.Response res;
    try {
      res = await _client.get(url).timeout(timeout);
    } catch (e) {
      throw RepoException('$url unreachable: $e');
    }
    if (res.statusCode != 200) {
      throw RepoException('$url returned ${res.statusCode}');
    }
    try {
      return decodeFdroidIndex(res.body);
    } on FormatException catch (e) {
      throw RepoException('$url is not a valid index: ${e.message}');
    }
  }

  void dispose() {
    if (_ownsClient) _client.close();
  }
}
