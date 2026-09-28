import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/src/ctrl_share.dart';

/// Build a 1024-byte sharemem snapshot with the given field values written
/// big-endian, matching the layout CVService publishes.
Uint8List makeShareMem({
  int poseFlagL = 2,
  int keyFlagL = 2,
  int poseFlagR = 2,
  int keyFlagR = 2,
  double px = 0.1,
  double py = -0.2,
  double pz = 0.3,
  double qw = 0.9,
  double qx = 0.1,
  double qy = 0.2,
  double qz = 0.3,
  int status = 1,
  int batteryL = 76,
  int batteryR = 54,
  int touchX = 128,
  int touchY = 140,
  int trigger = 0,
  int buttonA = 0,
}) {
  final buf = Uint8List(CtrlShare.size);
  void putPose(int which, int off) {
    final d = ByteData.sublistView(buf, which * 512 + off);
    d.setFloat32(0, 1.0, Endian.big); // fixed.x - arm model, unused
    d.setFloat32(40, px, Endian.big); // fuse.x
    d.setFloat32(44, py, Endian.big);
    d.setFloat32(48, pz, Endian.big);
    d.setFloat32(52, qw, Endian.big);
    d.setFloat32(56, qx, Endian.big);
    d.setFloat32(60, qy, Endian.big);
    d.setFloat32(64, qz, Endian.big);
    d.setInt32(68, status, Endian.big);
    d.setInt64(72, 0x1122334455, Endian.big);
    d.setFloat32(80, 1.7, Endian.big); // head_y
  }

  void putKeys(int which, int battery) {
    final d = ByteData.sublistView(buf, which * 512 + 104);
    d.setInt32(0, touchX, Endian.big);
    d.setInt32(4, touchY, Endian.big);
    d.setInt32(20, trigger, Endian.big);
    d.setInt32(24, battery, Endian.big);
    d.setInt32(28, buttonA, Endian.big);
  }

  buf[CtrlShare.left * 512] = poseFlagL;
  buf[CtrlShare.left * 512 + 100] = keyFlagL;
  buf[CtrlShare.right * 512] = poseFlagR;
  buf[CtrlShare.right * 512 + 100] = keyFlagR;
  putPose(CtrlShare.left, 4);
  putPose(CtrlShare.right, 4);
  putKeys(CtrlShare.left, batteryL);
  putKeys(CtrlShare.right, batteryR);
  return buf;
}

void main() {
  test('decodes pose and keys big-endian', () {
    final buf = makeShareMem();
    final l = decodeCtrlBlock(buf, CtrlShare.left);
    expect(l.poseOk, isTrue);
    expect(l.keysOk, isTrue);
    expect(l.pose.x, closeTo(0.1, 1e-6));
    expect(l.pose.y, closeTo(-0.2, 1e-6));
    expect(l.pose.z, closeTo(0.3, 1e-6));
    expect(l.pose.qw, closeTo(0.9, 1e-6));
    expect(l.tracked, isTrue);
    expect(l.battery, 76);
    expect(l.stickX, 128);
    expect(l.stickY, 140);
  });

  test('right block is independent', () {
    final buf = makeShareMem(batteryR: 54);
    final r = decodeCtrlBlock(buf, CtrlShare.right);
    expect(r.battery, 54);
  });

  test('flags other than DONE mark sections invalid', () {
    final buf = makeShareMem(poseFlagL: 0, keyFlagL: 1);
    final l = decodeCtrlBlock(buf, CtrlShare.left);
    expect(l.poseOk, isFalse);
    expect(l.keysOk, isFalse);
    expect(l.battery, -1);
  });

  test('all-zero block has no data', () {
    final buf = Uint8List(CtrlShare.size);
    expect(CtrlShare.blockHasData(buf, CtrlShare.left), isFalse);
    expect(CtrlShare.blockHasData(makeShareMem(), CtrlShare.left), isTrue);
  });

  test('midWrite detects flag bytes in WRITING state', () {
    final quiet = makeShareMem();
    expect(CtrlShare.midWrite(quiet), isFalse);
    final busy = makeShareMem(poseFlagR: 1);
    expect(CtrlShare.midWrite(busy), isTrue);
  });

  test('blockHash moves when the block moves', () {
    final a = makeShareMem(batteryL: 50);
    final b = makeShareMem(batteryL: 51);
    expect(CtrlShare.blockHash(a, 0), isNot(CtrlShare.blockHash(b, 0)));
    // stable for identical content
    expect(CtrlShare.blockHash(a, 0), CtrlShare.blockHash(makeShareMem(batteryL: 50), 0));
  });

  test('liveness: baseline is not live, a change inside the window is', () {
    final live = CtrlLiveness();
    expect(live.feed(111, false, 0), isFalse); // baseline only
    expect(live.feed(222, false, 1000000), isTrue); // moved hash
    // goes quiet after the 2.5s window
    expect(live.feed(222, false, 1000000 + 3000000000), isFalse);
    // a caught flag edge revives it even with a still hash
    expect(live.feed(222, true, 4000000000), isTrue);
  });

  test('toCtrlState maps wire fields', () {
    final buf = makeShareMem(batteryL: 80, trigger: 1, buttonA: 1);
    final c = decodeCtrlBlock(buf, CtrlShare.left).toCtrlState(0, true);
    expect(c.connected, isTrue);
    expect(c.battery, 80);
    expect(c.trigger, isTrue);
    expect(c.buttonA, isTrue);
    expect(c.label, 'L');
    final dead = decodeCtrlBlock(buf, CtrlShare.left).toCtrlState(0, false);
    expect(dead.connected, isFalse);
  });
}
