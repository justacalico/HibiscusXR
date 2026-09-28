import 'dart:typed_data';

import 'models.dart';

/// Decoder for the controller sharemem file at /sdcard/CtrlShareMem.
///
/// Dart port of applications/vrhome/src/input/ctrl_state.c - CVService
/// rewrites two 512-byte blocks per packet (left at 0, right at 512), all
/// fields big-endian, each section bracketed by a flag byte that reads
/// 1 while writing and 2 when done.
abstract final class CtrlShare {
  static const size = 1024;
  static const path = '/sdcard/CtrlShareMem';
  static const left = 0;
  static const right = 1;

  static const _flagWriting = 1;
  static const _flagDone = 2;

  static int _poseFlag(int which) => which * 512;
  static int _keyFlag(int which) => which * 512 + 100;

  /// Nonzero while the block carries controller data - the service keeps
  /// flapping the flags even for a never-linked slot but the data stays 0.
  static bool blockHasData(Uint8List buf, int which) {
    final base = which * 512;
    for (var i = 1; i < 512; i++) {
      if (i != 100 && buf[base + i] != 0) return true;
    }
    return false;
  }

  /// FNV-1a over one 512-byte block - a moved hash means a write landed.
  static int blockHash(Uint8List buf, int which) {
    var h = 0xcbf29ce484222325;
    const prime = 0x100000001b3;
    final base = which * 512;
    for (var i = 0; i < 512; i++) {
      h ^= buf[base + i];
      h = (h * prime) & 0xFFFFFFFFFFFFFFFF;
    }
    return h;
  }

  /// Whether the flags show a write mid-flight (snapshot was racy).
  static bool midWrite(Uint8List buf) =>
      buf[_poseFlag(left)] == _flagWriting ||
      buf[_keyFlag(left)] == _flagWriting ||
      buf[_poseFlag(right)] == _flagWriting ||
      buf[_keyFlag(right)] == _flagWriting;
}

/// Decoded contents of one controller block.
class CtrlBlock {
  const CtrlBlock({
    required this.poseOk,
    required this.keysOk,
    required this.hasData,
    required this.pose,
    required this.tracked,
    required this.battery,
    required this.stickX,
    required this.stickY,
    required this.buttons,
  });

  final bool poseOk, keysOk, hasData;
  final Pose pose;
  final bool tracked;
  final int battery;
  final int stickX, stickY;

  /// a/b, menu, system(home), trigger, grip, stick-click
  final Map<String, bool> buttons;

  CtrlState toCtrlState(int which, bool live) => CtrlState(
        index: which,
        connected: live && hasData,
        battery: keysOk ? battery : -1,
        pose: pose,
        tracked: tracked,
        stickX: stickX,
        stickY: stickY,
        buttonA: buttons['a'] ?? false,
        buttonB: buttons['b'] ?? false,
        menu: buttons['menu'] ?? false,
        system: buttons['sys'] ?? false,
        trigger: buttons['trig'] ?? false,
        grip: buttons['grip'] ?? false,
      );
}

/// Decode one controller's 512-byte block out of a full 1024-byte snapshot.
CtrlBlock decodeCtrlBlock(Uint8List buf, int which) {
  assert(buf.length >= CtrlShare.size);
  final poseFlag = CtrlShare._poseFlag(which);
  final keyFlag = CtrlShare._keyFlag(which);
  final poseOk = buf[poseFlag] == CtrlShare._flagDone;
  final keysOk = buf[keyFlag] == CtrlShare._flagDone;

  var pose = const Pose();
  var tracked = false;
  if (poseOk) {
    final p = ByteData.sublistView(buf, poseFlag + 4);
    // fused pose sits at +40; the arm-model "fixed" pose at +0 is unused
    pose = Pose(
      x: p.getFloat32(40, Endian.big),
      y: p.getFloat32(44, Endian.big),
      z: p.getFloat32(48, Endian.big),
      qw: p.getFloat32(52, Endian.big),
      qx: p.getFloat32(56, Endian.big),
      qy: p.getFloat32(60, Endian.big),
      qz: p.getFloat32(64, Endian.big),
    );
    // MCU tracking state - sentinel poses report 0
    tracked = p.getInt32(68, Endian.big) != 0;
  }

  var battery = -1;
  var sx = 0, sy = 0;
  final buttons = <String, bool>{};
  if (keysOk) {
    final k = ByteData.sublistView(buf, keyFlag + 4);
    sx = k.getInt32(0, Endian.big);
    sy = k.getInt32(4, Endian.big);
    buttons['sys'] = k.getInt32(8, Endian.big) != 0;
    buttons['menu'] = k.getInt32(12, Endian.big) != 0;
    buttons['stick'] = k.getInt32(16, Endian.big) != 0;
    buttons['trig'] = k.getInt32(20, Endian.big) != 0;
    battery = k.getInt32(24, Endian.big);
    buttons['a'] = k.getInt32(28, Endian.big) != 0;
    buttons['b'] = k.getInt32(32, Endian.big) != 0;
    buttons['grip'] = k.getInt32(36, Endian.big) != 0 || k.getInt32(40, Endian.big) != 0;
  }

  return CtrlBlock(
    poseOk: poseOk,
    keysOk: keysOk,
    hasData: CtrlShare.blockHasData(buf, which),
    pose: pose,
    tracked: tracked,
    battery: battery,
    stickX: sx,
    stickY: sy,
    buttons: buttons,
  );
}

/// Per-controller liveness: a block counts live while a content change or
/// flag write was seen inside the window (CVService rewrites every ~30 ms).
/// Port of ctrl_live_feed - the flag-edge probe needs mmap semantics the
/// snapshot path can't do, so callers pass `wireEdge` from midWrite checks.
class CtrlLiveness {
  static const windowNs = 2500000000; // CTRL_LIVE_NS

  int _hash = 0;
  int _changeNs = 0;
  bool _seen = false;

  bool feed(int hash, bool wireEdge, int nowNs) {
    if (hash != _hash) {
      _hash = hash;
      // first read is only a baseline - the file can sit stale for hours
      if (_seen) _changeNs = nowNs;
      _seen = true;
    }
    if (wireEdge) {
      _seen = true;
      _changeNs = nowNs;
    }
    return _changeNs != 0 && nowNs - _changeNs < windowNs;
  }
}
