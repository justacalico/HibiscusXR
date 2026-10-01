import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_hbsup/src/models.dart';
import 'package:hibiscus_hbsup/src/plan.dart';

const _parts = [
  Partition('boot', '/dev/block/sde17', 67108864),
  Partition('system', '/dev/block/sde32', 3865470464),
];

BackupPlan plan() =>
    const BackupPlan(destDir: '/tmp/x', partitions: _parts);

void main() {
  test('totalBytes sums the partitions', () {
    expect(plan().totalBytes, 67108864 + 3865470464);
  });

  group('checkSpace', () {
    test('fits when free covers the dump', () {
      final c = checkSpace(plan(), plan().totalBytes);
      expect(c.fits, isTrue);
      expect(c.shortfall, 0);
    });

    test('reports the shortfall when tight', () {
      final c = checkSpace(plan(), 1000);
      expect(c.fits, isFalse);
      expect(c.shortfall, plan().totalBytes - 1000);
    });

    test('unknown free space never blocks', () {
      final c = checkSpace(plan(), null);
      expect(c.fits, isTrue);
      expect(c.free, isNull);
    });
  });

  group('formatBytes', () {
    test('formats each unit', () {
      expect(formatBytes(512), '512 B');
      expect(formatBytes(1536), '1.5 KB');
      expect(formatBytes(4194304), '4.0 MB');
      expect(formatBytes(3865470464), '3.6 GB');
      expect(formatBytes(2 * 1024 * 1024 * 1024 * 1024), '2.0 TB');
    });
  });

  test('manifestFor lists every partition', () {
    final m = manifestFor('SERIAL9', _parts);
    expect(m, contains('serial: SERIAL9'));
    expect(m, contains('partitions: 2'));
    expect(m, contains('boot\t/dev/block/sde17\t67108864\tboot.img'));
    expect(
        m, contains('system\t/dev/block/sde32\t3865470464\tsystem.img'));
  });

  group('freeSpaceOf', () {
    test('df on the real host parses a number', () async {
      final free = await freeSpaceOf(Directory.systemTemp.path);
      expect(free, isNotNull);
      expect(free, greaterThan(0));
    });

    test('df output parses the available column', () async {
      Future<ProcessResult> proc(String e, List<String> a) async =>
          ProcessResult(0, 0,
              'Filesystem  1B-blocks  Used Available Use% Mounted on\n'
              '/dev/sda1  1000000 400000 600000  40% /\n',
              '');
      expect(await freeSpaceOf('/x', proc: proc, isWindows: false),
          600000);
    });

    test('unparseable or failing df gives null', () async {
      expect(
          await freeSpaceOf('/x',
              proc: (e, a) async => ProcessResult(0, 0, 'junk\n', ''),
              isWindows: false),
          isNull);
      expect(
          await freeSpaceOf('/x',
              proc: (e, a) async =>
                  ProcessResult(0, 0, 'header\nonlytwo 1\n', ''),
              isWindows: false),
          isNull);
      expect(
          await freeSpaceOf('/x',
              proc: (e, a) async => throw ProcessException('df', a),
              isWindows: false),
          isNull);
    });

    test('windows uses fsutil', () async {
      Future<ProcessResult> proc(String e, List<String> a) async =>
          ProcessResult(0, 0,
              'Total # of free bytes        : 409600\n'
              'Total # of bytes             : 999999\n',
              '');
      expect(await freeSpaceOf('C:\\backups', proc: proc, isWindows: true),
          409600);
    });

    test('fsutil without a free line or failing gives null', () async {
      expect(
          await freeSpaceOf('C:\\x',
              proc: (e, a) async =>
                  ProcessResult(0, 0, 'Total # of bytes : 1\n', ''),
              isWindows: true),
          isNull);
      expect(
          await freeSpaceOf('nodrive',
              proc: (e, a) async => throw ProcessException('fsutil', a),
              isWindows: true),
          isNull);
    });
  });
}
