import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import '../adb_runner.dart';
import '../ctrl_share.dart';
import '../link.dart';
import '../models.dart';
import '../pose_log.dart';
import '../props.dart';

/// HeadsetLink over the `adb` CLI: USB devices and wireless adb both land
/// here - `adb connect` handles the wireless side.
class AdbLink extends HeadsetLink {
  AdbLink(this._adb, this.serial);

  final AdbRunner _adb;
  final String serial;

  @override
  String get description => 'adb:$serial';

  /// `adb connect host:port` - returns the daemon's own reply line.
  static Future<String> connectWireless(AdbRunner adb, String host,
      {int port = 5555}) async {
    final r = await adb.run(['connect', '$host:$port']);
    return ((r.stdout as String? ?? '') + (r.stderr as String? ?? '')).trim();
  }

  /// Prepare the device for pose streaming - the posedump prop and the
  /// drop-in flag file the pn2 driver also polls.
  Future<void> enablePoseFeed() async {
    await _adb.run([
      '-s',
      serial,
      'shell',
      'setprop debug.pn2.posedump 1;'
          ' mkdir -p /data/local/tmp/xr;'
          ' touch /data/local/tmp/xr/posedump',
    ]);
  }

  @override
  Future<DeviceInfo> fetchInfo() async {
    final props = parseGetprop(await _adb.shellText(serial, 'getprop'));
    final batt = parseBatteryLevel(
        await _adb.shellText(serial, 'dumpsys battery 2>/dev/null'));
    return deviceInfoFromProps(props, serial: serial, batteryLevel: batt);
  }

  @override
  Future<Map<String, String>> fetchProps() async =>
      parseGetprop(await _adb.shellText(serial, 'getprop'));

  @override
  Stream<String> installApk(String path, {Uint8List? bytes}) async* {
    final p = await _adb.spawn(['-s', serial, 'install', '-r', path]);
    yield* p.stdout
        .transform(utf8.decoder)
        .transform(const LineSplitter());
    final code = await p.exitCode;
    if (code != 0) {
      yield 'adb install exited $code';
    }
  }

  /// screencap exits per frame, so poll it. exec-out keeps the PNG bytes
  /// raw (no PTY mangling).
  @override
  Stream<Uint8List> frames({Duration interval = const Duration(milliseconds: 400)}) async* {
    while (true) {
      final buf = BytesBuilder(copy: false);
      await for (final chunk in _adb.execOut(serial, ['screencap', '-p'])) {
        buf.add(chunk);
      }
      final png = buf.takeBytes();
      if (png.isNotEmpty) yield png;
      await Future<void>.delayed(interval);
    }
  }

  @override
  Stream<String> logLines() => _adb.streamLines(serial, ['logcat', '-v', 'brief']);

  @override
  Stream<PoseSample> poses() async* {
    await enablePoseFeed();
    final parser = PoseLogParser();
    final filter = '${kPoseLogTags.map((t) => '$t:I').join(' ')} *:S';
    await for (final line in _adb
        .streamLines(serial, ['shell', 'logcat -v brief $filter'])) {
      final s = parser.feed(line);
      if (s != null) yield s;
    }
  }

  /// CtrlShareMem is a fixed 1024-byte file - `cat` snapshots it fine.
  @override
  Stream<List<CtrlState>> controllers(
      {Duration interval = const Duration(milliseconds: 120)}) async* {
    final liveness = [CtrlLiveness(), CtrlLiveness()];
    final sw = Stopwatch()..start();
    while (true) {
      final buf = BytesBuilder(copy: false);
      await for (final chunk
          in _adb.execOut(serial, ['cat', CtrlShare.path])) {
        buf.add(chunk);
      }
      final raw = buf.takeBytes();
      if (raw.length >= CtrlShare.size) {
        final wireEdge = CtrlShare.midWrite(raw);
        final now = sw.elapsedMicroseconds * 1000;
        yield [
          for (var i = 0; i < 2; i++)
            decodeCtrlBlock(raw, i).toCtrlState(
              i,
              liveness[i].feed(CtrlShare.blockHash(raw, i), wireEdge, now),
            ),
        ];
      }
      await Future<void>.delayed(interval);
    }
  }

  @override
  Future<void> dispose() async {}
}
