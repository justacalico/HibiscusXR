import 'models.dart';

/// Parses pose log lines into [PoseSample]s. The parser only cares about
/// line shape - which tags carry pose lines comes from the connected
/// device's profile, so it works for any driver feed.
///
/// Two formats:
///  - three-line driver dump (raw / served rel / imu), emitted ~4Hz:
///      qvr  q=(.. .. .. ..) p=(.. .. ..) st=3 q=0.95
///           rel q=(.. .. .. ..) p=(.. .. ..) w=(.. .. ..) ts=123
///           imu q=(.. .. .. ..) a=(.. .. ..) g=(.. .. ..)
///    the `rel` line is what the runtime serves, so it yields the sample.
///  - hibiscuspose single line, the generic feed any driver can emit:
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

  /// Drop the logcat prefix when present: the timestamped
  /// `MM-DD HH:MM:SS.mmm pid tid lvl tag:` threadtime form or the
  /// `lvl/tag(pid):` brief form. Any tag is stripped - the feed was
  /// already filtered to pose tags by whoever opened it.
  static final _timeRe = RegExp(
      r'^\d{2}-\d{2} +\d{2}:\d{2}:\d{2}\.\d{3} +\d+ +\d+ +[VDIWEF] +[^:]+: (.*)$');
  static final _briefRe = RegExp(
      r'^[VDIWEF]/[^(:]+\(\s*\d+\): (.*)$');

  static String? _stripHeader(String line) {
    final m = _timeRe.firstMatch(line) ?? _briefRe.firstMatch(line);
    if (m != null) return m.group(1)!.trimLeft();
    // may already be a bare line from the CTE socket feed
    final t = line.trimLeft();
    return (t.startsWith('qvr') || t.startsWith('rel') ||
            t.startsWith('imu') || t.startsWith('HMD'))
        ? t
        : null;
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
    final m = RegExp('(?:^|\\s)$name=\\(([^)]*)\\)').firstMatch(body);
    if (m == null) return null;
    return m
        .group(1)!
        .split(RegExp(r'\s+'))
        .where((s) => s.isNotEmpty)
        .map((s) => double.tryParse(s) ?? 0)
        .toList();
  }

  static double? _parseNum(String body, String name) {
    final m = RegExp('(?:^|\\s)$name=(-?[0-9.eE+-]+)').firstMatch(body);
    return m == null ? null : double.tryParse(m.group(1)!);
  }

  static int? _parseInt(String body, String name) {
    final m = RegExp('(?:^|\\s)$name=(-?\\d+)').firstMatch(body);
    return m == null ? null : int.tryParse(m.group(1)!);
  }
}
