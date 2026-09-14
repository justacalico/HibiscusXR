import 'dart:io';
import 'dart:typed_data';

/// Minimal synchronous PNG writer (8-bit RGBA, no filtering) so tests can
/// mint icon bytes without touching the engine.
Uint8List solidPng(int size, int argb) {
  final px = Uint8List(size * (size * 4 + 1));
  for (var y = 0; y < size; y++) {
    final row = y * (size * 4 + 1);
    for (var x = 0; x < size; x++) {
      final i = row + 1 + x * 4;
      px[i] = (argb >> 16) & 0xFF;
      px[i + 1] = (argb >> 8) & 0xFF;
      px[i + 2] = argb & 0xFF;
      px[i + 3] = (argb >> 24) & 0xFF;
    }
  }
  return encodePng(size, size, px);
}

Uint8List encodePng(int w, int h, Uint8List scanlines) {
  final out = BytesBuilder();
  out.add(const [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]);

  final ihdr = ByteData(13)
    ..setUint32(0, w)
    ..setUint32(4, h)
    ..setUint8(8, 8)
    ..setUint8(9, 6);
  _chunk(out, 'IHDR', ihdr.buffer.asUint8List());
  _chunk(out, 'IDAT', Uint8List.fromList(zlib.encode(scanlines)));
  _chunk(out, 'IEND', const []);
  return out.toBytes();
}

void _chunk(BytesBuilder out, String type, List<int> data) {
  final len = ByteData(4)..setUint32(0, data.length);
  out.add(len.buffer.asUint8List());
  final typeBytes = type.codeUnits;
  out.add(typeBytes);
  out.add(data);
  final crc = ByteData(4)
    ..setUint32(0, _crc32([...typeBytes, ...data]));
  out.add(crc.buffer.asUint8List());
}

int _crc32(List<int> bytes) {
  var crc = 0xFFFFFFFF;
  for (final b in bytes) {
    crc ^= b;
    for (var i = 0; i < 8; i++) {
      crc = (crc & 1) != 0 ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
    }
  }
  return crc ^ 0xFFFFFFFF;
}
