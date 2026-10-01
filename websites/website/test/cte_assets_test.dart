import 'dart:io';

import 'package:flutter_test/flutter_test.dart';

void main() {
  test('cte page shots mirror the app goldens byte for byte', () {
    const shotsDir = 'assets/cte';
    const goldensDir =
        '../../applications/desktop/cte/test/golden/goldens';

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
            're-copy the changed files from applications/desktop/cte goldens');

    for (final name in shots) {
      expect(
        File('$shotsDir/$name').readAsBytesSync(),
        File('$goldensDir/$name').readAsBytesSync(),
        reason: name,
      );
    }
  });
}
