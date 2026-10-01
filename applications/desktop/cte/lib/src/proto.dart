import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'models.dart';

/// Wire protocol for the on-headset `cted` daemon (system/cted).
///
/// One command per connection: the client opens a socket, reads the
/// `CTE/1` banner, writes one line command, then reads the reply -
/// either a single `+`-prefixed result or a stream of `KEY ...` lines
/// (and `FRAME <len>` + raw bytes) until disconnect.
abstract final class CteProto {
  static const port = 7340;
  static const banner = 'CTE/1';
  static const ok = '+OK';
  static const err = '+ERR';
  static const json = '+JSON';

  static const cmdPing = 'PING';
  static const cmdInfo = 'INFO';
  static const cmdProps = 'PROPS';
  static const cmdFrames = 'FRAMES';
  static const cmdPose = 'POSE';
  static const cmdCtrl = 'CTRL';
  static const cmdLog = 'LOG';
  static const cmdInstall = 'INSTALL';
}

/// A parsed stream message coming back from cted.
sealed class CteMsg {
  const CteMsg();
}

class CteFrameMsg extends CteMsg {
  const CteFrameMsg(this.png);
  final Uint8List png;
}

class CteCtrlMsg extends CteMsg {
  const CteCtrlMsg(this.state);
  final CtrlState state;
}

class CteLogMsg extends CteMsg {
  const CteLogMsg(this.line);
  final String line;
}

/// `POSELOG` forwards raw pose-tag logcat lines for pose_log.dart to chew on.
class CtePoseLogMsg extends CteMsg {
  const CtePoseLogMsg(this.line);
  final String line;
}

class CteInfoMsg extends CteMsg {
  const CteInfoMsg(this.json);
  final Map<String, dynamic> json;
}

/// `+TEXT` blob payload (PROPS replies).
class CteTextMsg extends CteMsg {
  const CteTextMsg(this.text);
  final String text;
}

/// `+OK`/`+PONG` style acknowledgements.
class CteResultMsg extends CteMsg {
  const CteResultMsg(this.message);
  final String message;
}

class CteErrorMsg extends CteMsg {
  const CteErrorMsg(this.message);
  final String message;
}

/// Parse a `CTRL <idx> k=v ...` line from cted. Returns null on junk.
CtrlState? parseCtrlLine(String line) {
  final parts = line.trim().split(RegExp(r'\s+'));
  if (parts.length < 2 || parts[0] != 'CTRL') return null;
  final idx = int.tryParse(parts[1]);
  if (idx == null) return null;
  final kv = <String, String>{};
  for (var i = 2; i < parts.length; i++) {
    final eq = parts[i].indexOf('=');
    if (eq > 0) kv[parts[i].substring(0, eq)] = parts[i].substring(eq + 1);
  }
  double n(String k) => double.tryParse(kv[k] ?? '') ?? 0;
  bool b(String k) => kv[k] == '1';
  return CtrlState(
    index: idx,
    connected: b('live'),
    battery: int.tryParse(kv['batt'] ?? '') ?? -1,
    pose: Pose(
      x: n('px'), y: n('py'), z: n('pz'),
      qx: n('qx'), qy: n('qy'), qz: n('qz'), qw: n('qw') == 0 ? 1 : n('qw'),
    ),
    tracked: b('trk'),
    stickX: int.tryParse(kv['sx'] ?? '') ?? 0,
    stickY: int.tryParse(kv['sy'] ?? '') ?? 0,
    buttonA: b('a'),
    buttonB: b('b'),
    menu: b('menu'),
    system: b('sys'),
    trigger: b('trig'),
    grip: b('grip'),
  );
}

/// Byte-stream reader: mixes newline-terminated lines with length-prefixed
/// blobs (`FRAME <len>` / `+JSON <len>`).
class CteWireReader {
  CteWireReader(Stream<Uint8List> source) {
    _sub = source.listen(
      _onData,
      onDone: () => _close(),
      onError: (Object e) => _close(),
      cancelOnError: true,
    );
  }

  final _buf = BytesBuilder(copy: false);
  final _out = StreamController<CteMsg>();
  late final StreamSubscription<Uint8List> _sub;
  bool _closed = false;

  Stream<CteMsg> get messages => _out.stream;

  void _onData(Uint8List chunk) {
    _buf.add(chunk);
    _drain();
  }

  void _drain() {
    while (true) {
      final bytes = _buf.takeBytes();
      final nl = _findNl(bytes);
      if (nl < 0) {
        _buf.add(bytes);
        return;
      }
      final line = utf8.decode(bytes.sublist(0, nl), allowMalformed: true).trimRight();
      final rest = bytes.sublist(nl + 1);

      final blobLen = _blobLen(line);
      if (blobLen != null) {
        if (rest.length < blobLen) {
          _buf.add(bytes); // wait for the blob
          return;
        }
        _emit(line, rest.sublist(0, blobLen));
        if (rest.length > blobLen) _buf.add(rest.sublist(blobLen));
        continue;
      }
      _emit(line, null);
      if (rest.isNotEmpty) _buf.add(rest);
    }
  }

  static int _findNl(Uint8List bytes) {
    for (var i = 0; i < bytes.length; i++) {
      if (bytes[i] == 0x0A) return i;
    }
    return -1;
  }

  /// Lines that are followed by a raw blob.
  static int? _blobLen(String line) {
    for (final prefix in const ['FRAME ', '+JSON ', '+TEXT ']) {
      if (line.startsWith(prefix)) {
        return int.tryParse(line.substring(prefix.length).trim());
      }
    }
    return null;
  }

  void _emit(String line, Uint8List? blob) {
    if (line.startsWith('FRAME ')) {
      if (blob != null) _out.add(CteFrameMsg(blob));
    } else if (line.startsWith('+JSON')) {
      if (blob != null) {
        try {
          final decoded = jsonDecode(utf8.decode(blob));
          if (decoded is Map<String, dynamic>) _out.add(CteInfoMsg(decoded));
        } on FormatException {
          _out.add(const CteErrorMsg('bad info json'));
        }
      }
    } else if (line.startsWith('+TEXT')) {
      if (blob != null) {
        _out.add(CteTextMsg(utf8.decode(blob, allowMalformed: true)));
      }
    } else if (line.startsWith('CTRL ')) {
      final s = parseCtrlLine(line);
      if (s != null) _out.add(CteCtrlMsg(s));
    } else if (line.startsWith('POSELOG ')) {
      _out.add(CtePoseLogMsg(line.substring(8)));
    } else if (line.startsWith('LOG ')) {
      _out.add(CteLogMsg(line.substring(4)));
    } else if (line.startsWith(CteProto.err)) {
      _out.add(CteErrorMsg(line.substring(CteProto.err.length).trim()));
    } else if (line.startsWith(CteProto.ok)) {
      _out.add(CteResultMsg(line.substring(CteProto.ok.length).trim()));
    }
    // banner and unknown lines are dropped
  }

  void _close() {
    if (_closed) return;
    _closed = true;
    _out.close();
  }

  Future<void> dispose() async {
    await _sub.cancel();
    _close();
  }
}

/// Build a DeviceInfo out of the INFO json cted replies with.
DeviceInfo deviceInfoFromJson(Map<String, dynamic> j, {String address = ''}) {
  final props = <String, String>{};
  final raw = j['props'];
  if (raw is Map) {
    raw.forEach((k, v) => props['$k'] = '$v');
  }
  return DeviceInfo(
    model: j['model'] as String? ?? props['ro.product.model'] ?? '',
    device: j['device'] as String? ?? props['ro.product.device'] ?? '',
    brand: props['ro.product.brand'] ?? '',
    manufacturer: props['ro.product.manufacturer'] ?? '',
    androidRelease: props['ro.build.version.release'] ?? '',
    sdkInt: int.tryParse(props['ro.build.version.sdk'] ?? '') ?? 0,
    buildId: props['ro.build.display.id'] ?? '',
    hibiscusVersion: props['ro.hibiscus.version'] ?? '',
    trackingMode: j['tracking'] == 'dof6'
        ? TrackingMode.dof6
        : j['tracking'] == 'dof3'
            ? TrackingMode.dof3
            : TrackingMode.unknown,
    address: address,
    batteryLevel: j['battery'] as int? ?? -1,
  );
}
