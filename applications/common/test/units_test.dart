import 'package:common/common.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  group('formatBytes', () {
    test('bytes stay raw under a KB', () {
      expect(formatBytes(0), '0 B');
      expect(formatBytes(512), '512 B');
      expect(formatBytes(1023), '1023 B');
    });

    test('one decimal under 100 of a unit', () {
      expect(formatBytes(1024), '1.0 KB');
      expect(formatBytes(1536), '1.5 KB');
      expect(formatBytes(1500000), '1.4 MB');
      expect(formatBytes(4194304), '4.0 MB');
      expect(formatBytes(3865470464), '3.6 GB');
      expect(formatBytes(3 * 1024 * 1024 * 1024), '3.0 GB');
      expect(formatBytes(2 * 1024 * 1024 * 1024 * 1024), '2.0 TB');
    });

    test('rounds to a whole number at 100+ of a unit', () {
      expect(formatBytes(150 * 1024 * 1024), '150 MB');
      expect(formatBytes(200 * 1024 * 1024 * 1024), '200 GB');
    });
  });
}
