import 'dart:convert';
import 'dart:io' show ZLibDecoder;
import 'dart:typed_data';

/// One file in a zip's central directory.
class ZipEntry {
  const ZipEntry({
    required this.name,
    required this.method,
    required this.compressedSize,
    required this.size,
    required this.localOffset,
  });

  final String name;

  /// 0 stored, 8 deflate. Anything else can't be read back.
  final int method;
  final int compressedSize;
  final int size;
  final int localOffset;
}

const _sigEocd = 0x06054b50;
const _sigCentral = 0x02014b50;
const _sigLocal = 0x04034b50;
const _flagDataDesc = 0x0008;

int _endOfCentralDir(Uint8List b) {
  if (b.length < 22) return -1;
  final bd = ByteData.sublistView(b);
  // the EOCD lives in the last 64K + 22 bytes; scan backwards
  final first = b.length - 22 - 0xffff;
  for (var i = b.length - 22; i >= (first < 0 ? 0 : first); --i) {
    if (bd.getUint32(i, Endian.little) == _sigEocd) return i;
  }
  return -1;
}

/// Walk the central directory. Returns null when the buffer is not a zip;
/// entries stored with a data descriptor are skipped since their sizes
/// aren't declared up front.
List<ZipEntry>? zipEntries(Uint8List bytes) {
  final eocd = _endOfCentralDir(bytes);
  if (eocd < 0) return null;
  final bd = ByteData.sublistView(bytes);
  final count = bd.getUint16(eocd + 10, Endian.little);
  var p = bd.getUint32(eocd + 16, Endian.little);
  final out = <ZipEntry>[];
  for (var i = 0; i < count; ++i) {
    if (p + 46 > bytes.length ||
        bd.getUint32(p, Endian.little) != _sigCentral) {
      break;
    }
    final flags = bd.getUint16(p + 8, Endian.little);
    final nlen = bd.getUint16(p + 28, Endian.little);
    final elen = bd.getUint16(p + 30, Endian.little);
    final clen = bd.getUint16(p + 32, Endian.little);
    if (p + 46 + nlen > bytes.length) break;
    if (flags & _flagDataDesc == 0) {
      out.add(
        ZipEntry(
          name: utf8.decode(
            bytes.sublist(p + 46, p + 46 + nlen),
            allowMalformed: true,
          ),
          method: bd.getUint16(p + 10, Endian.little),
          compressedSize: bd.getUint32(p + 20, Endian.little),
          size: bd.getUint32(p + 24, Endian.little),
          localOffset: bd.getUint32(p + 42, Endian.little),
        ),
      );
    }
    p += 46 + nlen + elen + clen;
  }
  return out;
}

/// Extract one entry by exact name. Returns null when it's absent or its
/// compression isn't stored/deflate.
Uint8List? zipEntryData(Uint8List zip, String name) {
  final entries = zipEntries(zip);
  if (entries == null) return null;
  final bd = ByteData.sublistView(zip);
  for (final e in entries) {
    if (e.name != name) continue;
    final l = e.localOffset;
    if (l + 30 > zip.length ||
        bd.getUint32(l, Endian.little) != _sigLocal) {
      return null;
    }
    final nlen = bd.getUint16(l + 26, Endian.little);
    final elen = bd.getUint16(l + 28, Endian.little);
    final off = l + 30 + nlen + elen;
    if (off + e.compressedSize > zip.length) return null;
    final raw = zip.sublist(off, off + e.compressedSize);
    if (e.method == 0) return raw;
    if (e.method == 8) {
      try {
        final inflated = ZLibDecoder(raw: true).convert(raw);
        if (inflated.length != e.size) return null;
        return Uint8List.fromList(inflated);
      } catch (_) {
        return null;
      }
    }
    return null;
  }
  return null;
}
