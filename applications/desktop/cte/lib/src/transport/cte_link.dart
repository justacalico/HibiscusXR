import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import '../debug_dump.dart';
import '../link.dart';
import '../models.dart';
import '../pose_log.dart';
import '../proto.dart';
import '../props.dart';

class _Channel {
  const _Channel(this.sock, this.messages);
  final Socket sock;
  final Stream<CteMsg> messages;
}

/// HeadsetLink over a plain TCP socket to the on-device cted daemon -
/// the wireless path that needs no adb on the host at all.
///
/// One command per connection: connect, read the `CTE/1` banner, write a
/// command line, then read typed messages until close.
class CteLink extends HeadsetLink {
  CteLink(this.host, {this.port = CteProto.port});

  final String host;
  final int port;

  @override
  String get description => 'cte://$host:$port';

  /// Check whether a cted answers on host:port (reads the banner).
  static Future<CteLink?> probe(String host,
      {int port = CteProto.port,
      Duration timeout = const Duration(seconds: 3)}) async {
    Socket? sock;
    try {
      sock = await Socket.connect(host, port, timeout: timeout);
      final first = await sock
          .cast<List<int>>()
          .transform(utf8.decoder)
          .transform(const LineSplitter())
          .first
          .timeout(timeout);
      await sock.close();
      return first.trim().startsWith(CteProto.banner)
          ? CteLink(host, port: port)
          : null;
    } on SocketException {
      await sock?.close().catchError((_) {});
      return null;
    } on TimeoutException {
      await sock?.close().catchError((_) {});
      return null;
    }
  }

  /// Open one command channel. The banner line is consumed (and dropped)
  /// by the wire reader along with everything else.
  Future<_Channel> _open(String cmd,
      {Uint8List? blob, Duration timeout = const Duration(seconds: 5)}) async {
    final sock = await Socket.connect(host, port, timeout: timeout);
    final reader = CteWireReader(sock);
    sock.writeln(cmd);
    if (blob != null) {
      sock.add(blob);
      await sock.flush();
    }
    return _Channel(sock, reader.messages);
  }

  Future<CteMsg> _first(String cmd,
      {Duration timeout = const Duration(seconds: 8)}) async {
    final ch = await _open(cmd);
    try {
      return await ch.messages.first.timeout(timeout);
    } finally {
      await ch.sock.close();
    }
  }

  @override
  Future<DeviceInfo> fetchInfo() async {
    final msg = await _first(CteProto.cmdInfo);
    if (msg is CteInfoMsg) {
      return deviceInfoFromJson(msg.json, address: '$host:$port');
    }
    if (msg is CteErrorMsg) throw Exception(msg.message);
    throw Exception('no info reply from cted');
  }

  @override
  Future<Map<String, String>> fetchProps() async {
    final msg = await _first(CteProto.cmdProps);
    if (msg is CteTextMsg) return parseGetprop(msg.text);
    return {};
  }

  @override
  Stream<Uint8List> frames() async* {
    final ch = await _open(CteProto.cmdFrames);
    try {
      await for (final m in ch.messages) {
        if (m is CteFrameMsg) yield m.png;
        if (m is CteErrorMsg) throw Exception(m.message);
      }
    } finally {
      await ch.sock.close();
    }
  }

  @override
  Stream<String> logLines() async* {
    final ch = await _open(CteProto.cmdLog);
    try {
      await for (final m in ch.messages) {
        if (m is CteLogMsg) yield m.line;
        if (m is CteErrorMsg) throw Exception(m.message);
      }
    } finally {
      await ch.sock.close();
    }
  }

  @override
  Stream<PoseSample> poses() async* {
    final ch = await _open(CteProto.cmdPose);
    final parser = PoseLogParser();
    try {
      await for (final m in ch.messages) {
        if (m is CtePoseLogMsg) {
          final s = parser.feed(m.line);
          if (s != null) yield s;
        }
        if (m is CteErrorMsg) throw Exception(m.message);
      }
    } finally {
      await ch.sock.close();
    }
  }

  @override
  Stream<List<CtrlState>> controllers() async* {
    final ch = await _open(CteProto.cmdCtrl);
    final latest = <int, CtrlState>{};
    try {
      await for (final m in ch.messages) {
        if (m is CteCtrlMsg) {
          latest[m.state.index] = m.state;
          yield [
            latest[0] ?? const CtrlState(index: 0),
            latest[1] ?? const CtrlState(index: 1),
          ];
        }
        if (m is CteErrorMsg) throw Exception(m.message);
      }
    } finally {
      await ch.sock.close();
    }
  }

  /// New cted builds run the on-device collector and hand back one tar.gz
  /// blob. An older cted answers +ERR unknown command - the partial dump
  /// then pulls whatever the fixed command set already serves.
  @override
  Stream<DumpSection> debugDump() async* {
    Uint8List? blob;
    try {
      final ch = await _open(CteProto.cmdDump,
          timeout: const Duration(seconds: 60));
      try {
        await for (final m in ch.messages) {
          if (m is CteBlobMsg) blob = m.bytes;
        }
      } finally {
        await ch.sock.close();
      }
    } on SocketException {
      rethrow;
    }
    if (blob != null) {
      final sections = decodeTarGz(blob);
      if (sections.isNotEmpty) {
        yield* Stream.fromIterable(sections);
        return;
      }
    }
    yield* _partialDump();
  }

  /// What an old cted can still produce: identity, props, a logcat tail,
  /// controller snapshots and a screenshot.
  Stream<DumpSection> _partialDump() async* {
    yield DumpSection(
        'PARTIAL.txt', Uint8List.fromList(_partialNote.codeUnits));
    try {
      final info = await fetchInfo();
      yield DumpSection(
          'info.json',
          Uint8List.fromList(utf8.encode(const JsonEncoder.withIndent('  ')
              .convert({
                'model': info.model,
                'device': info.device,
                'brand': info.brand,
                'android': info.androidRelease,
                'sdk': info.sdkInt,
                'build': info.buildId,
                'hibiscus': info.hibiscusVersion,
                'tracking': info.trackingMode.name,
                'battery': info.batteryLevel,
              }))));
    } on SocketException {
      // info is optional; keep collecting what still answers
    }
    final props = await fetchProps();
    if (props.isNotEmpty) {
      yield DumpSection(
          'getprop.txt',
          Uint8List.fromList(utf8.encode(
              props.entries.map((e) => '[${e.key}]: [${e.value}]\n').join())));
    }
    // bounded capture: ~4s of silence ends the tail, busy logcat ends at
    // 800 lines
    final log = StringBuffer();
    final ch = await _open(CteProto.cmdLog);
    try {
      await for (final m in ch.messages.take(800).timeout(
          const Duration(seconds: 4),
          onTimeout: (s) => s.close())) {
        if (m is CteLogMsg) log.writeln(m.line);
      }
    } finally {
      await ch.sock.close();
    }
    if (log.isNotEmpty) {
      yield DumpSection('logcat.txt',
          Uint8List.fromList(utf8.encode(log.toString())));
    }
    final ctrls = StringBuffer();
    try {
      final cch = await _open(CteProto.cmdCtrl);
      try {
        await for (final m in cch.messages.timeout(
            const Duration(seconds: 3),
            onTimeout: (s) => s.close())) {
          if (m is CteCtrlMsg) {
            final c = m.state;
            ctrls.writeln(
                'ctrl ${c.index} live=${c.connected} batt=${c.battery} '
                'trk=${c.tracked} pos=(${c.pose.x},${c.pose.y},${c.pose.z}) '
                'quat=(${c.pose.qx},${c.pose.qy},${c.pose.qz},${c.pose.qw}) '
                'stick=(${c.stickX},${c.stickY}) a=${c.buttonA} b=${c.buttonB} '
                'trig=${c.trigger} grip=${c.grip}');
          }
        }
      } finally {
        await cch.sock.close();
      }
    } on SocketException {
      // no controller channel on this build
    }
    if (ctrls.isNotEmpty) {
      yield DumpSection('controllers.txt',
          Uint8List.fromList(utf8.encode(ctrls.toString())));
    }
    try {
      await for (final f in frames()) {
        yield DumpSection('screenshot.png', f);
        break;
      }
    } catch (_) {
      // screencap is best effort
    }
  }

  static const _partialNote =
      'This headset runs an older cted without the DUMP command, so this\n'
      'is a partial bundle collected over the fixed commands. Flash an\n'
      'image with cte-dump for the full dump.\n';

  @override
  Stream<String> installApk(String path, {Uint8List? bytes}) async* {
    final data = bytes ?? await File(path).readAsBytes();
    final ch = await _open('${CteProto.cmdInstall} ${data.length}', blob: data);
    try {
      await for (final m in ch.messages) {
        if (m is CteResultMsg) {
          yield m.message.isEmpty ? 'ok' : m.message;
          return;
        }
        if (m is CteLogMsg) yield m.line;
        if (m is CteErrorMsg) {
          yield 'error: ${m.message}';
          return;
        }
      }
    } finally {
      await ch.sock.close();
    }
  }

  @override
  Future<void> dispose() async {}
}
