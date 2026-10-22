import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

/// One collected file inside a debug dump.
class DumpSection {
  const DumpSection(this.name, this.bytes);
  final String name;
  final Uint8List bytes;
}

/// Core sections grabbed one command at a time when the headset image
/// predates /system/bin/cte-dump. Mirrors the on-device collector
/// (system/cted/cte-dump.sh) - keep them in step.
const legacyDumpSections = <(String, String)>[
  ('meta.txt',
      'date; uptime; uname -a; getprop ro.hibiscus.version; getprop ro.serialno'),
  ('getprop.txt', 'getprop'),
  ('logcat.txt', 'logcat -d -v threadtime'),
  ('logcat_events.txt', 'logcat -d -b events'),
  ('logcat_radio.txt', 'logcat -d -b radio'),
  ('logcat_crash.txt', 'logcat -d -b crash'),
  ('dmesg.txt', 'dmesg'),
  ('dumpsys.txt', 'dumpsys'),
  ('packages.txt', 'pm list packages -f -u'),
  ('appops.txt', 'dumpsys appops'),
  ('sensors.txt', 'dumpsys sensorservice'),
  ('ps.txt', 'ps -A'),
  ('top.txt', 'top -bn1'),
  ('proc/interrupts.txt', 'cat /proc/interrupts'),
  ('proc/mounts.txt', 'cat /proc/mounts; df -h'),
  ('net.txt', 'ip addr; ip route; dumpsys wifi'),
  ('bluetooth.txt', 'dumpsys bluetooth_manager'),
  ('dropbox.txt', 'dumpsys dropbox'),
  ('tombstones.txt',
      'ls -l /data/tombstones; i=0; for f in /data/tombstones/*; do [ \$i -ge 8 ] && break; echo "== \$f"; head -c 65536 "\$f"; echo; i=\$((i+1)); done'),
  ('ctrlshare.bin', 'cat /sdcard/CtrlShareMem'),
  ('settings_global.txt', 'settings list global'),
  ('settings_system.txt', 'settings list system'),
  ('settings_secure.txt', 'settings list secure'),
];

/// Default save name for the zip: `hibiscus-dump-SER9-20261015-121430.zip`.
String dumpFileName(String tag, DateTime now) {
  final dev = tag.isEmpty ? 'headset' : tag;
  final clean = dev.replaceAll(RegExp(r'[^A-Za-z0-9_-]'), '_');
  String two(int v) => v.toString().padLeft(2, '0');
  final stamp = '${now.year}${two(now.month)}${two(now.day)}-'
      '${two(now.hour)}${two(now.minute)}${two(now.second)}';
  return 'hibiscus-dump-$clean-$stamp.zip';
}

/// Unpack the tar.gz stream cte-dump emits on the device. Also accepts a
/// plain tar so an uncompressed blob still decodes. Directory entries and
/// GNU longname markers are skipped; unknown entries are dropped rather
/// than failing the whole dump.
List<DumpSection> decodeTarGz(Uint8List blob) {
  var data = blob;
  if (data.length >= 2 && data[0] == 0x1f && data[1] == 0x8b) {
    data = Uint8List.fromList(GZipCodec().decode(data));
  }
  final out = <DumpSection>[];
  var off = 0;
  var longName = '';
  while (off + 512 <= data.length) {
    final h = data.sublist(off, off + 512);
    if (h.every((b) => b == 0)) break;
    final size = int.tryParse(
            utf8
                .decode(h.sublist(124, 136), allowMalformed: true)
                .split('\x00')
                .first
                .trim(),
            radix: 8) ??
        0;
    final type = h[156];
    final name = utf8.decode(h.sublist(0, 100), allowMalformed: true);
    off += 512;
    if (off + size > data.length) break;
    final body = data.sublist(off, off + size);
    off += ((size + 511) ~/ 512) * 512;

    if (type == 0x4C) {
      // GNU 'L' longname: the real name is in the body
      longName = utf8.decode(body, allowMalformed: true).split('\x00').first;
      continue;
    }
    if (type != 0x30 && type != 0x00 && type != 0x37) {
      continue; // dirs, links, pax headers - only plain files ship
    }
    var n = longName.isNotEmpty ? longName : name.split('\x00').first;
    longName = '';
    while (n.startsWith('./')) {
      n = n.substring(2);
    }
    if (n.isEmpty) continue;
    out.add(DumpSection(n, Uint8List.fromList(body)));
  }
  return out;
}

final _crcTable = List<int>.generate(256, (i) {
  var c = i;
  for (var k = 0; k < 8; k++) {
    c = (c & 1) != 0 ? 0xEDB88320 ^ (c >> 1) : c >> 1;
  }
  return c;
});

/// CRC-32 (IEEE), used by the zip writer.
int crc32(Uint8List data) {
  var crc = 0xFFFFFFFF;
  for (final b in data) {
    crc = _crcTable[(crc ^ b) & 0xFF] ^ (crc >> 8);
  }
  return crc ^ 0xFFFFFFFF;
}

/// Pack sections into a .zip - deflate when it wins, stored otherwise.
/// Structure is the boring standard kind: local headers + data, then the
/// central directory, then EOCD.
Uint8List zipDump(List<DumpSection> sections) {
  final out = BytesBuilder(copy: false);
  final cd = BytesBuilder(copy: false);
  final now = DateTime.now();
  final dosTime =
      (now.hour << 11) | (now.minute << 5) | (now.second ~/ 2);
  final dosDate =
      (((now.year - 1980).clamp(0, 127)) << 9) | (now.month << 5) | now.day;

  void u16(BytesBuilder b, int v) {
    b.addByte(v & 0xFF);
    b.addByte((v >> 8) & 0xFF);
  }

  void u32(BytesBuilder b, int v) {
    u16(b, v & 0xFFFF);
    u16(b, (v >> 16) & 0xFFFF);
  }

  for (final s in sections) {
    final name = utf8.encode(s.name);
    final flat = ZLibCodec(raw: true).encode(s.bytes);
    final store = flat.length >= s.bytes.length;
    final body = store ? s.bytes : Uint8List.fromList(flat);
    final crc = crc32(s.bytes);
    final offset = out.length;

    u32(out, 0x04034b50);
    u16(out, 20); // version needed
    u16(out, 0x0800); // UTF-8 names
    u16(out, store ? 0 : 8);
    u16(out, dosTime);
    u16(out, dosDate);
    u32(out, crc);
    u32(out, body.length);
    u32(out, s.bytes.length);
    u16(out, name.length);
    u16(out, 0); // extra len
    out.add(name);
    out.add(body);

    u32(cd, 0x02014b50);
    u16(cd, 20); // version made by
    u16(cd, 20);
    u16(cd, 0x0800);
    u16(cd, store ? 0 : 8);
    u16(cd, dosTime);
    u16(cd, dosDate);
    u32(cd, crc);
    u32(cd, body.length);
    u32(cd, s.bytes.length);
    u16(cd, name.length);
    for (var i = 0; i < 6; i++) {
      u16(cd, 0); // extra, comment, disk, internal, external attrs
    }
    u32(cd, offset);
    cd.add(name);
  }

  final cdBytes = cd.takeBytes();
  final cdOff = out.length;
  out.add(cdBytes);
  u32(out, 0x06054b50);
  u16(out, 0);
  u16(out, 0);
  u16(out, sections.length);
  u16(out, sections.length);
  u32(out, cdBytes.length);
  u32(out, cdOff);
  u16(out, 0);
  return out.takeBytes();
}
