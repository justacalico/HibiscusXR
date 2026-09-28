import 'dart:async';
import 'dart:collection';

import 'package:flutter/foundation.dart';

import 'adb_runner.dart';
import 'link.dart';
import 'models.dart';
import 'transport/adb_link.dart';
import 'transport/cte_link.dart';

enum ConnState { idle, scanning, connecting, connected, failed }

/// How the wire to the headset is made.
enum LinkKind { adbUsb, adbWireless, cteSocket }

/// Everything the UI reads lives here; created once above MaterialApp so
/// window resizes never touch it.
class AppState extends ChangeNotifier {
  AppState({
    AdbRunner? adb,
    Future<HeadsetLink?> Function(String host)? socketFactory,
    HeadsetLink? Function(String serial)? adbLinkFactory,
  })  : _adb = adb ?? AdbRunner(),
        _socketFactory = socketFactory ?? CteLink.probe,
        _adbLinkFactory = adbLinkFactory ?? ((s) => AdbLink(adb ?? AdbRunner(), s));

  final AdbRunner _adb;
  final Future<HeadsetLink?> Function(String host) _socketFactory;
  final HeadsetLink? Function(String serial) _adbLinkFactory;

  ConnState connState = ConnState.idle;
  LinkKind? linkKind;
  HeadsetLink? link;
  DeviceInfo? deviceInfo;
  String? lastError;

  List<AdbDeviceRow> adbDevices = const [];

  // live feeds
  final List<CtrlState> ctrls = [const CtrlState(index: 0), const CtrlState(index: 1)];
  final Queue<PoseSample> poseBuf = ListQueue(600);
  final Queue<String> logBuf = ListQueue(400);
  Uint8List? frame;
  int framesSeen = 0;
  double poseRate = 0;
  bool mirrorOn = true;
  StreamSubscription<dynamic>? _frameSub;

  // install
  bool installing = false;
  final List<String> installLog = [];

  Map<String, String>? props;

  final _subs = <StreamSubscription<dynamic>>[];
  int _poseStamp = 0;
  int _poseWindowStart = 0;

  bool get connected => connState == ConnState.connected && link != null;

  String get connectedLabel {
    final l = link;
    if (l == null) return '';
    return '${deviceInfo?.headsetName ?? 'headset'} via ${l.description}';
  }

  Future<void> scanAdb() async {
    connState = ConnState.scanning;
    lastError = null;
    notifyListeners();
    try {
      adbDevices = await _adb.devices();
      connState = ConnState.idle;
    } catch (e) {
      lastError = 'adb scan failed: $e';
      connState = ConnState.idle;
    }
    notifyListeners();
  }

  /// Wireless path: try cted first, fall back to `adb connect`.
  Future<void> connectWireless(String host) async {
    connState = ConnState.connecting;
    lastError = null;
    notifyListeners();
    try {
      final probed = await _socketFactory(host);
      if (probed != null) {
        await _attach(probed, LinkKind.cteSocket);
        return;
      }
      final reply = await AdbLink.connectWireless(_adb, host);
      if (!reply.contains('connected')) {
        throw Exception(reply.isEmpty ? 'no reply from adb connect' : reply);
      }
      final serial = '$host:5555';
      await _attach(_adbLinkFactory(serial)!, LinkKind.adbWireless);
    } catch (e) {
      connState = ConnState.failed;
      lastError = '$e';
      notifyListeners();
    }
  }

  Future<void> connectAdb(String serial, {bool wireless = false}) async {
    connState = ConnState.connecting;
    lastError = null;
    notifyListeners();
    try {
      await _attach(
        _adbLinkFactory(serial)!,
        wireless ? LinkKind.adbWireless : LinkKind.adbUsb,
      );
    } catch (e) {
      connState = ConnState.failed;
      lastError = '$e';
      notifyListeners();
    }
  }

  Future<void> _attach(HeadsetLink l, LinkKind kind) async {
    await _detachFeeds();
    link = l;
    linkKind = kind;
    deviceInfo = await l.fetchInfo();
    connState = ConnState.connected;
    notifyListeners();
    // streams restart against the live link
    _subs.add(l.controllers().listen(_onCtrls, onError: _onFeedError));
    _subs.add(l.poses().listen(_onPose, onError: _onFeedError));
    _subs.add(l.logLines().listen(_onLog, onError: _onFeedError));
    if (mirrorOn) {
      _frameSub = l.frames().listen(_onFrame, onError: _onFeedError);
      _subs.add(_frameSub!);
    }
    unawaited(l.fetchProps().then((p) {
      props = p;
      notifyListeners();
    }).catchError((_) {}));
  }

  void _onCtrls(List<CtrlState> next) {
    for (final c in next) {
      if (c.index < ctrls.length) ctrls[c.index] = c;
    }
    notifyListeners();
  }

  void _onPose(PoseSample s) {
    poseBuf.addLast(s);
    while (poseBuf.length > 600) {
      poseBuf.removeFirst();
    }
    // rolling 2s rate window
    final now = DateTime.now().millisecondsSinceEpoch;
    _poseStamp++;
    if (_poseWindowStart == 0) _poseWindowStart = now;
    final span = now - _poseWindowStart;
    if (span >= 2000) {
      poseRate = _poseStamp * 1000.0 / span;
      _poseStamp = 0;
      _poseWindowStart = now;
    }
    notifyListeners();
  }

  void _onFrame(Uint8List png) {
    frame = png;
    framesSeen++;
    notifyListeners();
  }

  void _onLog(String line) {
    logBuf.addLast(line);
    while (logBuf.length > 400) {
      logBuf.removeFirst();
    }
    notifyListeners();
  }

  void _onFeedError(Object e) {
    lastError = 'feed error: $e';
    notifyListeners();
  }

  Future<void> install(String path) async {
    final l = link;
    if (l == null || installing) return;
    installing = true;
    installLog.clear();
    notifyListeners();
    try {
      await for (final line in l.installApk(path)) {
        installLog.add(line);
        notifyListeners();
      }
    } catch (e) {
      installLog.add('install failed: $e');
    }
    installing = false;
    notifyListeners();
  }

  /// Mirror is a paid stream (screencap churn) - let the user park it.
  Future<void> setMirror(bool on) async {
    mirrorOn = on;
    final old = _frameSub;
    _frameSub = null;
    if (old != null) {
      _subs.remove(old);
      await old.cancel();
    }
    final l = link;
    if (on && l != null) {
      _frameSub = l.frames().listen(_onFrame, onError: _onFeedError);
      _subs.add(_frameSub!);
    } else {
      frame = null;
    }
    notifyListeners();
  }

  Future<void> disconnect() async {
    await _detachFeeds();
    connState = ConnState.idle;
    notifyListeners();
  }

  Future<void> _detachFeeds() async {
    for (final s in _subs) {
      await s.cancel();
    }
    _subs.clear();
    await link?.dispose();
    link = null;
    linkKind = null;
    deviceInfo = null;
    props = null;
    frame = null;
    framesSeen = 0;
    poseBuf.clear();
    logBuf.clear();
    ctrls[0] = const CtrlState(index: 0);
    ctrls[1] = const CtrlState(index: 1);
  }

  @override
  Future<void> dispose() async {
    await _detachFeeds();
    super.dispose();
  }
}
