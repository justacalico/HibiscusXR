import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/src/debug_dump.dart';

import 'tar_fixture.dart';

void main() {
  test('decodeTarGz recovers names and bodies', () {
    final blob = tarArchive({
      'meta.txt': utf8.encode('model=Pico Neo 2\n'),
      'proc/interrupts.txt': utf8.encode('  1: 42 msm\n'),
      './getprop.txt': utf8.encode('[ro.x]: [y]\n'),
    });
    final sections = decodeTarGz(blob);
    expect(sections.map((s) => s.name).toList(),
        ['meta.txt', 'proc/interrupts.txt', 'getprop.txt']);
    expect(utf8.decode(sections.first.bytes), 'model=Pico Neo 2\n');
  });

  test('decodeTarGz reads an uncompressed tar too', () {
    final blob = tarArchive({'a.txt': utf8.encode('x')}, gzipIt: false);
    expect(decodeTarGz(blob).single.name, 'a.txt');
  });

  test('decodeTarGz drops dir entries and survives junk', () {
    final dir = tarEntry('dumpsys/', Uint8List(0));
    dir[156] = 0x35; // '5' directory
    final blob = tarArchive({});
    final sections = decodeTarGz(Uint8List.fromList([...dir, ...blob]));
    expect(sections, isEmpty);
    expect(decodeTarGz(Uint8List.fromList([1, 2, 3])), isEmpty);
  });

  test('crc32 matches the ieee vector', () {
    expect(crc32(Uint8List.fromList(utf8.encode('123456789'))), 0xCBF43926);
    expect(crc32(Uint8List(0)), 0);
  });

  test('zipDump produces a readable zip', () {
    final sections = [
      DumpSection('logcat.txt',
          Uint8List.fromList(utf8.encode('I/cte: hi\n' * 200))),
      DumpSection('ctrlshare.bin', Uint8List.fromList(List.filled(1024, 7))),
    ];
    final zip = zipDump(sections);

    expect(zip.sublist(0, 4), [0x50, 0x4B, 0x03, 0x04]); // PK local header
    expect(zip.sublist(zip.length - 22, zip.length - 18),
        [0x50, 0x4B, 0x05, 0x06]); // EOCD

    // the EOCD carries the central directory offset at EOCD+16
    final cdOff = zip[zip.length - 6] |
        (zip[zip.length - 5] << 8) |
        (zip[zip.length - 4] << 16) |
        (zip[zip.length - 3] << 24);
    expect(cdOff, greaterThan(0));
    expect(cdOff, lessThan(zip.length - 22));
    final found = <String, Uint8List>{};
    var p = cdOff;
    while (zip[p] == 0x50 && zip[p + 1] == 0x4B && zip[p + 3] == 0x02) {
      int u16(int o) => zip[p + o] | (zip[p + o + 1] << 8);
      int u32(int o) =>
          u16(o) | (zip[p + o + 2] << 16) | (zip[p + o + 3] << 24);
      final method = u16(10);
      final crc = u32(16);
      final csize = u32(20);
      final nameLen = u16(28);
      final extraLen = u16(30);
      final commentLen = u16(32);
      final localOff = u32(42);
      final name = utf8.decode(zip.sublist(p + 46, p + 46 + nameLen));
      final localNameLen =
          zip[localOff + 26] | (zip[localOff + 27] << 8);
      final localExtraLen =
          zip[localOff + 28] | (zip[localOff + 29] << 8);
      final dataOff = localOff + 30 + localNameLen + localExtraLen;
      final body = zip.sublist(dataOff, dataOff + csize);
      final raw = method == 8
          ? Uint8List.fromList(ZLibCodec(raw: true).decode(body))
          : Uint8List.fromList(body);
      expect(crc32(raw), crc, reason: '$name crc');
      found[name] = raw;
      p += 46 + nameLen + extraLen + commentLen;
    }
    expect(found.keys.toSet(), {'logcat.txt', 'ctrlshare.bin'});
    expect(utf8.decode(found['logcat.txt']!), 'I/cte: hi\n' * 200);
    expect(found['ctrlshare.bin']!.length, 1024);
  });

  test('dumpFileName stamps device and time', () {
    final n = dumpFileName('SER9', DateTime(2026, 10, 15, 12, 14, 30));
    expect(n, 'hibiscus-dump-SER9-20261015-121430.zip');
    expect(dumpFileName('', DateTime(2026, 1, 2, 3, 4, 5)),
        'hibiscus-dump-headset-20260102-030405.zip');
    expect(dumpFileName('a b/c', DateTime(2026, 1, 1)),
        'hibiscus-dump-a_b_c-20260101-000000.zip');
  });

  test('legacyDumpSections are sane shell one-liners', () {
    final names = legacyDumpSections.map((s) => s.$1).toSet();
    expect(names.length, legacyDumpSections.length);
    for (final (name, cmd) in legacyDumpSections) {
      expect(name, matches(RegExp(r'^[A-Za-z0-9_./-]+\.(txt|bin)$')));
      expect(cmd.trim(), isNotEmpty);
    }
    expect(names, containsAll(
        ['getprop.txt', 'logcat.txt', 'dmesg.txt', 'ctrlshare.bin']));
  });
}
