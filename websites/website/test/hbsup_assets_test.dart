import 'dart:io';

import 'package:flutter_test/flutter_test.dart';

void main() {
  test('hbsup page shots mirror the app goldens byte for byte', () {
    const shotsDir = 'assets/hbsup';
    const goldensDir =
        '../../applications/desktop/hbsup/test/golden/goldens';

    Set<String> names(String dir) => Directory(dir)
        .listSync()
        .whereType<File>()
        .where((f) => f.path.endsWith('.png'))
        .map((f) => f.uri.pathSegments.last)
        .toSet();

    final shots = names(shotsDir);
    final goldens = names(goldensDir);
    expect(shots, goldens,
        reason:
            're-copy the changed files from applications/desktop/hbsup goldens');

    for (final name in shots) {
      expect(
        File('$shotsDir/$name').readAsBytesSync(),
        File('$goldensDir/$name').readAsBytesSync(),
        reason: name,
      );
    }
  });
}
