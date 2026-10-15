import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

/// Minimal ustar entry writer so tests can hand a tarball to decodeTarGz
/// without depending on a host `tar` binary.
Uint8List tarEntry(String name, Uint8List body) {
  final h = Uint8List(512);
  final n = utf8.encode(name);
  h.setRange(0, n.length, n);
  void octal(int off, int len, int v) {
    final s = v.toRadixString(8).padLeft(len - 1, '0');
    h.setRange(off, off + s.length, utf8.encode(s));
  }

  octal(100, 8, 0x1a4); // 644
  octal(124, 12, body.length);
  octal(136, 12, 1700000000);
  h[156] = 0x30; // '0' regular file
  h.setRange(257, 262, utf8.encode('ustar'));
  h.setRange(263, 265, utf8.encode('00'));
  var sum = 0;
  for (var i = 0; i < 512; i++) {
    sum += (i >= 148 && i < 156) ? 0x20 : h[i];
  }
  final chk = sum.toRadixString(8).padLeft(6, '0');
  h.setRange(148, 148 + chk.length, utf8.encode(chk));
  h[154] = 0;
  h[155] = 0x20;

  final pad = (512 - body.length % 512) % 512;
  return Uint8List.fromList([...h, ...body, ...List.filled(pad, 0)]);
}

/// Whole-archive helper: files + the two trailing zero blocks.
Uint8List tarArchive(Map<String, Uint8List> files, {bool gzipIt = true}) {
  final b = BytesBuilder(copy: false);
  files.forEach((name, body) => b.add(tarEntry(name, body)));
  b.add(Uint8List(1024));
  final tar = b.takeBytes();
  return gzipIt
      ? Uint8List.fromList(GZipCodec().encode(tar))
      : tar;
}
