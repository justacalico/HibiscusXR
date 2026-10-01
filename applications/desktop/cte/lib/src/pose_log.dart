import 'models.dart';

/// Tags the app watches on logcat for live pose data.
/// `pn2pose` is the pn2 driver's raw/served dump; `hibiscuspose` is the
/// generic single-line feed any driver can emit while posedump is on.
const kPoseLogTags = ['pn2pose', 'hibiscuspose'];

/// Parses pose log lines into [PoseSample]s.
///
/// Two formats:
///  - pn2pose three-line dump (raw qvr / served rel / imu), emitted ~4Hz:
///      qvr  q=(.. .. .. ..) p=(.. .. ..) st=3 q=0.95
///           rel q=(.. .. .. ..) p=(.. .. ..) w=(.. .. ..) ts=123
///           imu q=(.. .. .. ..) a=(.. .. ..) g=(.. .. ..)
///    the `rel` line is what the runtime serves, so it yields the sample.
///  - hibiscuspose single line:
///      HMD ts=123 qx=.. qy=.. qz=.. qw=.. px=.. py=.. pz=.. st=3
class PoseLogParser {
  int _rawState = 0;

  /// Feed one logcat line (already stripped of the logcat header, or raw).
  /// Returns a sample when the line completes one.
  PoseSample? feed(String line) {
    final body = _stripHeader(line);
    if (body == null) return null;

    final hmd = _parseHibiscusLine(body);
    if (hmd != null) return hmd;

    if (body.startsWith('qvr')) {
      // raw qvr line carries the tracking state the served pose implies
      _rawState = _parseInt(body, 'st') ?? 0;
      return null;
    }
    if (body.startsWith('rel')) {
      final rel = _parseQP(body);
      if (rel == null) return null;
      final ts = _parseInt(body, 'ts') ?? 0;
      return PoseSample(
        pose: rel,
        timestampNs: ts,
        angularVelocity: _parseVec(body, 'w') ?? const [0, 0, 0],
        trackingState: _rawState,
        hasPosition: _rawState == 3,
      );
    }
    return null;
  }

  /// Drop the `MM-DD HH:MM:SS.mmm pid tid lvl tag:` logcat prefix when present.
  static String? _stripHeader(String line) {
    final tagIdx = line.indexOf('pn2pose');
    final hibIdx = line.indexOf('hibiscuspose');
    final idx = tagIdx >= 0 ? tagIdx : hibIdx;
    if (idx < 0) {
      // may already be a bare line from the CTE socket feed
      final t = line.trimLeft();
      return (t.startsWith('qvr') || t.startsWith('rel') || t.startsWith('imu') || t.startsWith('HMD'))
          ? t
          : null;
    }
    final colon = line.indexOf(':', idx);
    if (colon < 0) return null;
    return line.substring(colon + 1).trimLeft();
  }

  PoseSample? _parseHibiscusLine(String body) {
    if (!body.startsWith('HMD')) return null;
    final qx = _parseNum(body, 'qx');
    final qy = _parseNum(body, 'qy');
    final qz = _parseNum(body, 'qz');
    final qw = _parseNum(body, 'qw');
    final px = _parseNum(body, 'px');
    final py = _parseNum(body, 'py');
    final pz = _parseNum(body, 'pz');
    if (qx == null || qy == null || qz == null || qw == null) return null;
    final st = _parseInt(body, 'st') ?? 0;
    final hasPos = px != null && py != null && pz != null;
    return PoseSample(
      pose: Pose(
        qx: qx,
        qy: qy,
        qz: qz,
        qw: qw,
        x: px ?? 0,
        y: py ?? 0,
        z: pz ?? 0,
      ),
      timestampNs: _parseInt(body, 'ts') ?? 0,
      trackingState: st,
      hasPosition: hasPos && st == 3,
    );
  }

  static final _parenRe = RegExp(r'([a-z]+)=\(([^)]*)\)');

  /// Parse `q=(a b c d) p=(x y z)` style fields out of a dump line.
  Pose? _parseQP(String body) {
    final fields = {for (final m in _parenRe.allMatches(body)) m.group(1)!: m.group(2)!};
    final q = fields['q']?.split(RegExp(r'\s+')).where((s) => s.isNotEmpty).toList();
    if (q == null || q.length < 4) return null;
    final p = fields['p']?.split(RegExp(r'\s+')).where((s) => s.isNotEmpty).toList() ?? [];
    double at(List<String> l, int i) => double.tryParse(l[i]) ?? 0;
    return Pose(
      qx: at(q, 0),
      qy: at(q, 1),
      qz: at(q, 2),
      qw: at(q, 3),
      x: p.isNotEmpty ? at(p, 0) : 0,
      y: p.length > 1 ? at(p, 1) : 0,
      z: p.length > 2 ? at(p, 2) : 0,
    );
  }

  List<double>? _parseVec(String body, String name) {
    final m = RegExp('$name=\\(([^)]*)\\)').firstMatch(body);
    if (m == null) return null;
    return m
        .group(1)!
        .split(RegExp(r'\s+'))
        .where((s) => s.isNotEmpty)
        .map((s) => double.tryParse(s) ?? 0)
        .toList();
  }

  static double? _parseNum(String body, String name) {
    final m = RegExp('$name=(-?[0-9.eE+-]+)').firstMatch(body);
    return m == null ? null : double.tryParse(m.group(1)!);
  }

  static int? _parseInt(String body, String name) {
    final m = RegExp('$name=(-?\\d+)').firstMatch(body);
    return m == null ? null : int.tryParse(m.group(1)!);
  }
}
