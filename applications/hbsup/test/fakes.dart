import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_hbsup/src/adb_runner.dart';
import 'package:hibiscus_hbsup/src/app_state.dart';
import 'package:hibiscus_hbsup/ui/app.dart';

/// The by-name dir the fake partition table pretends to live under.
const byNameDir = '/dev/block/bootdevice/by-name';
const byNameLs = '''
total 0
lrwxrwxrwx 1 root root 1970 boot -> /dev/block/sde17
lrwxrwxrwx 1 root root 1970 system -> /dev/block/sde32
''';

/// IOSink that just counts bytes - keeps the engine off real file IO
/// inside the fake test zone.
class FakeSink implements IOSink {
  final bytes = <int>[];
  var closed = false;

  @override
  void add(List<int> data) => bytes.addAll(data);

  @override
  Future<void> flush() async {}

  @override
  Future<void> close() async {
    closed = true;
  }

  @override
  dynamic noSuchMethod(Invocation invocation) => super.noSuchMethod(invocation);
}

class FakeAdb extends AdbRunner {
  FakeAdb({
    AdbRun? run,
    Map<String, String>? shell,
    Map<String, Stream<Uint8List>>? exec,
    this.shellDelay,
  })  : _shell = shell ?? const {},
        _exec = exec ?? {},
        super(
          run: run ?? (_) async => ProcessResult(0, 0, '', ''),
          spawn: (_) => Process.start('echo', const []),
        );

  final Map<String, String> _shell;
  final Map<String, Stream<Uint8List>> _exec;

  /// Gate shell answers - lets a test watch the loading UI.
  final Future<void> Function()? shellDelay;

  void setExec(String command, Stream<Uint8List> stream) {
    _exec[command] = stream;
  }

  @override
  Future<String> shellText(String serial, String command) async {
    await shellDelay?.call();
    return _shell[command] ?? '';
  }

  @override
  Stream<Uint8List> execOut(String serial, String command) =>
      _exec[command] ?? const Stream.empty();
}

/// Streams that cancel the run after the first chunk lands - exercises
/// the mid-flight cancel path deterministically.
Stream<Uint8List> cancellingStream(void Function() cancel) async* {
  yield Uint8List(512);
  cancel();
  yield Uint8List(512);
}

const adbShell = <String, String>{
  'ls -l /dev/block/bootdevice/by-name': byNameLs,
  'cat /sys/class/block/sde17/size': '2048',
  'cat /sys/class/block/sde32/size': '8388608',
};

FakeAdb fakeAdb({Map<String, Stream<Uint8List>>? exec}) => FakeAdb(
      run: (args) async => ProcessResult(0, 0,
          'List of devices attached\nSER9\tdevice model:Pico_Neo_2\n', ''),
      shell: adbShell,
      exec: exec ?? {},
    );

final fakeSinks = <String, FakeSink>{};

AppState fakeState({
  String? hostOs,
  FakeAdb? adb,
  Future<String?> Function()? pickDir,
}) =>
    AppState(
      adb: adb ?? fakeAdb(),
      hostOs: hostOs ?? 'linux',
      freeSpace: (_) async => 1 << 40,
      openSink: (p) => fakeSinks[p] = FakeSink(),
      pickDir: pickDir,
    );

/// Tall viewport so every card on the page builds - ListView is lazy.
void bigView(WidgetTester tester) {
  tester.view.physicalSize = const Size(1200, 2000);
  tester.view.devicePixelRatio = 1.0;
  addTearDown(tester.view.reset);
}

Future<AppState> connect(WidgetTester tester,
    {String? hostOs, FakeAdb? adb}) async {
  bigView(tester);
  final s = fakeState(hostOs: hostOs, adb: adb);
  await tester.pumpWidget(HbsupApp(state: s));
  await tester.pumpAndSettle();
  await s.connect('SER9');
  await tester.pumpAndSettle();
  return s;
}
