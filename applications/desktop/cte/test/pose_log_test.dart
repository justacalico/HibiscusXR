import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/src/models.dart';
import 'package:hibiscus_cte/src/pose_log.dart';

void main() {
  group('pn2pose dump', () {
    test('emits a sample on the rel line', () {
      final p = PoseLogParser();
      expect(
        p.feed('qvr  q=(0.1 0.2 0.3 0.9) p=(1.0 1.5 2.0) st=3 q=0.95'),
        isNull,
      );
      final s = p.feed(
          '     rel q=(0.11 0.22 0.33 0.88) p=(1.1 1.6 2.1) w=(0.01 0.02 0.03) ts=987654');
      expect(s, isNotNull);
      expect(s!.pose.qx, closeTo(0.11, 1e-6));
      expect(s.pose.qw, closeTo(0.88, 1e-6));
      expect(s.pose.x, closeTo(1.1, 1e-6));
      expect(s.timestampNs, 987654);
      expect(s.trackingState, 3);
      expect(s.hasPosition, isTrue);
      expect(s.mode, TrackingMode.dof6);
      expect(s.angularVelocity[0], closeTo(0.01, 1e-6));
    });

    test('st below 3 means orientation-only', () {
      final p = PoseLogParser();
      p.feed('qvr  q=(0 0 0 1) p=(0 0 0) st=1 q=0.1');
      final s = p.feed('rel q=(0 0 0 1) p=(0 0 0) w=(0 0 0) ts=5');
      expect(s!.mode, TrackingMode.dof3);
    });

    test('strips the logcat header', () {
      final p = PoseLogParser();
      p.feed('09-28 12:00:00.000 1234 5678 I pn2pose: qvr  q=(0 0 0 1) p=(0 0 0) st=3 q=0.9');
      final s = p.feed(
          '09-28 12:00:00.001 1234 5678 I pn2pose:      rel q=(0 0 0 1) p=(0 0 0) w=(0 0 0) ts=42');
      expect(s, isNotNull);
      expect(s!.timestampNs, 42);
    });
  });

  group('hibiscuspose line', () {
    test('parses the generic one-line feed', () {
      final p = PoseLogParser();
      final s = p.feed(
          'HMD ts=777 qx=0.1 qy=0.2 qz=0.3 qw=0.9 px=1.5 py=1.6 pz=-0.2 st=3');
      expect(s, isNotNull);
      expect(s!.pose.x, closeTo(1.5, 1e-6));
      expect(s.pose.qz, closeTo(0.3, 1e-6));
      expect(s.mode, TrackingMode.dof6);
    });

    test('missing position fields mean 3dof', () {
      final p = PoseLogParser();
      final s = p.feed('HMD ts=1 qx=0 qy=0 qz=0 qw=1 st=1');
      expect(s!.mode, TrackingMode.dof3);
    });
  });

  test('ignores unrelated lines', () {
    final p = PoseLogParser();
    expect(p.feed('D SomeTag: hello world'), isNull);
    expect(p.feed('random noise'), isNull);
    expect(p.feed(''), isNull);
  });
}
