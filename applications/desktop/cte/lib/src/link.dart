import 'dart:typed_data';

import 'debug_dump.dart';
import 'models.dart';

/// One open channel to a headset - either the adb CLI or the on-device
/// cted socket. The UI only ever talks to this.
abstract class HeadsetLink {
  /// Short human label: `adb:SER9` or `cte://10.0.0.5`.
  String get description;

  /// Device properties + battery + tracking mode.
  Future<DeviceInfo> fetchInfo();

  /// Raw `getprop` table for the debug page.
  Future<Map<String, String>> fetchProps();

  /// Install an apk already on disk. Yields the installer output lines.
  Stream<String> installApk(String path, {Uint8List? bytes});

  /// PNG screen frames, streamed back to back as fast as the headset
  /// can screencap them.
  Stream<Uint8List> frames();

  /// logcat lines (all tags).
  Stream<String> logLines();

  /// Head pose samples parsed out of the pose log feed.
  Stream<PoseSample> poses();

  /// Controller snapshots - index 0 left, 1 right; absent entries when a
  /// transport can't read them.
  Stream<List<CtrlState>> controllers();

  /// The full debug bundle ("Dump Debug SSD"): yields each collected
  /// section as it lands so the UI can show progress. Transports degrade
  /// to a smaller section set on headsets whose image predates the
  /// on-device collector.
  Stream<DumpSection> debugDump();

  Future<void> dispose();
}
