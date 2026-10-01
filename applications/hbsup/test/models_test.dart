import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_hbsup/src/models.dart';

void main() {
  group('parseByNameLs', () {
    test('parses ls -l by-name output', () {
      const out = '''
total 0
lrwxrwxrwx 1 root root 1970-01-01 00:14 abl -> /dev/block/sde8
lrwxrwxrwx 1 root root 1970-01-01 00:14 boot -> /dev/block/sde17
lrwxrwxrwx 1 root root 1970-01-01 00:14 system -> /dev/block/sde32
''';
      final parts = parseByNameLs(out);
      expect(parts.length, 3);
      expect(parts[0].name, 'abl');
      expect(parts[0].path, '/dev/block/sde8');
      expect(parts[2].name, 'system');
    });

    test('skips totals, blanks and non-symlink rows', () {
      const out = '''
total 4
-rw-r--r-- 1 root root 3 boot

''';
      expect(parseByNameLs(out), isEmpty);
    });

    test('returns empty on garbage', () {
      expect(parseByNameLs('not a listing\nnope\n'), isEmpty);
    });
  });

  test('basenameOf takes the last segment', () {
    expect(basenameOf('/dev/block/sde17'), 'sde17');
    expect(basenameOf('sde17'), 'sde17');
    expect(basenameOf('/dev/'), '');
  });

  group('parseSectorCount', () {
    test('reads a sector count', () {
      expect(parseSectorCount('2097152\n'), 2097152);
    });

    test('rejects garbage and negatives', () {
      expect(parseSectorCount('oops'), isNull);
      expect(parseSectorCount('-12'), isNull);
      expect(parseSectorCount(''), isNull);
    });
  });

  test('sectorsToBytes multiplies by 512', () {
    expect(sectorsToBytes(2048), 1048576);
    expect(sectorsToBytes(0), 0);
  });

  test('partition fileName is the img dump name', () {
    const p = Partition('boot', '/dev/block/sde17', 67108864);
    expect(p.fileName, 'boot.img');
  });
}
