import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_hbsup/main.dart' as app;
import 'package:hibiscus_hbsup/src/app_state.dart';
import 'package:hibiscus_hbsup/ui/app.dart';

import 'fakes.dart';

void main() {
  testWidgets('main() boots the real app', (tester) async {
    // real adb + real zone - the fake zone would trap dart:io timers
    await tester.runAsync(() async {
      app.main();
      await Future<void>.delayed(const Duration(seconds: 1));
    });
    await tester.pump();
    expect(find.byType(HbsupApp), findsOneWidget);
  });

  testWidgets('connect page renders and lists devices', (tester) async {
    final s = fakeState();
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pumpAndSettle();
    expect(find.text('HBSUP'), findsOneWidget);
    expect(find.text('Connect a headset'), findsOneWidget);
    expect(find.text('Pico_Neo_2'), findsOneWidget);
    expect(find.text('SER9'), findsOneWidget);

    await tester.tap(find.text('Connect'));
    await tester.pumpAndSettle();
    expect(s.connected, isTrue);
    expect(find.text('Partitions'), findsOneWidget);
  });

  testWidgets('windows shows the unsupported banner, dismiss hides it',
      (tester) async {
    final s = fakeState(hostOs: 'windows');
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pumpAndSettle();
    expect(find.text('Unsupported host OS'), findsOneWidget);

    await tester.tap(find.text('Dismiss'));
    await tester.pumpAndSettle();
    expect(find.text('Unsupported host OS'), findsNothing);
  });

  testWidgets('linux host shows no banner', (tester) async {
    await tester.pumpWidget(HbsupApp(state: fakeState()));
    await tester.pumpAndSettle();
    expect(find.text('Unsupported host OS'), findsNothing);
  });

  testWidgets('connecting with an empty device list shows the hint',
      (tester) async {
    final s = fakeState(adb: FakeAdb());
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pumpAndSettle();
    expect(find.textContaining('No adb devices'), findsOneWidget);
    await tester.tap(find.byTooltip('Refresh'));
    await tester.pumpAndSettle();
  });

  testWidgets('adb errors surface as an error card', (tester) async {
    final s = fakeState(
        adb: FakeAdb(
            run: (args) async =>
                throw const ProcessException('adb', [])));
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pumpAndSettle();
    expect(find.textContaining('adb'), findsWidgets);
  });

  testWidgets('backup page lists partitions, picks folder, runs the dump',
      (tester) async {
    // only the boot partition, so the fake dump size matches
    final adb = FakeAdb(
      run: (args) async => ProcessResult(0, 0,
          'List of devices attached\nSER9\tdevice model:Pico_Neo_2\n',
          ''),
      shell: {
        'ls -l $byNameDir': '''
total 0
lrwxrwxrwx 1 root root 1970 boot -> /dev/block/sde17
''',
        'cat /sys/class/block/sde17/size': '2048',
      },
      exec: {
        'dd if=/dev/block/sde17 bs=4M 2>/dev/null':
            Stream.value(Uint8List(2048 * 512)),
      },
    );
    final s = await connect(tester, adb: adb);
    expect(find.text('boot'), findsOneWidget);
    expect(find.text('Full dump: 1.0 MB'), findsOneWidget);
    expect(find.text('No folder picked yet'), findsOneWidget);


    await s.setDestination('/tmp/hbsup-ui');
    await tester.pumpAndSettle();
    expect(find.textContaining('free'), findsWidgets);
    expect(find.text('Enough free space for the full dump'),
        findsOneWidget);

    await tester.tap(find.text('Start backup'));
    await tester.pumpAndSettle();
    expect(
        find.textContaining('Backup finished'), findsOneWidget);
    expect(find.text('Log'), findsOneWidget);
    expect(s.manifestWritten, isTrue);
    expect(
        String.fromCharCodes(
            fakeSinks['/tmp/hbsup-ui/manifest.txt']!.bytes),
        contains('boot.img'));

    await tester.tap(find.text('Disconnect'));
    await tester.pumpAndSettle();
    expect(find.text('Connect a headset'), findsOneWidget);
  });

  testWidgets('short on space shows the shortfall and blocks start',
      (tester) async {
    final s = AppState(
      adb: fakeAdb(),
      hostOs: 'linux',
      freeSpace: (_) async => 8,
    );
    bigView(tester);
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pumpAndSettle();
    await s.connect('SER9');
    await tester.pumpAndSettle();
    await s.setDestination('/tmp/x');
    await tester.pumpAndSettle();
    expect(find.textContaining('Not enough space'), findsOneWidget);
    final start = tester.widget<FilledButton>(
        find.widgetWithText(FilledButton, 'Start backup'));
    expect(start.onPressed, isNull);
  });

  testWidgets('unknown free space still allows starting',
      (tester) async {
    final s = AppState(
      adb: fakeAdb(),
      hostOs: 'linux',
      freeSpace: (_) async => null,
    );
    bigView(tester);
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pumpAndSettle();
    await s.connect('SER9');
    await tester.pumpAndSettle();
    await s.setDestination('/tmp/x');
    await tester.pumpAndSettle();
    expect(find.text('Free space unknown'), findsOneWidget);
  });

  testWidgets('failed dump shows the failure banner', (tester) async {
    final adb = FakeAdb(
      run: (args) async => ProcessResult(0, 0,
          'List of devices attached\nSER9\tdevice\n', ''),
      shell: {
        'ls -l $byNameDir': '''
total 0
lrwxrwxrwx 1 root root 1970 boot -> /dev/block/sde17
''',
        'cat /sys/class/block/sde17/size': '8',
      },
      exec: {
        'dd if=/dev/block/sde17 bs=4M 2>/dev/null':
            Stream.error(StateError('usb dropped')),
      },
    );
    final s = await connect(tester, adb: adb);
    await s.setDestination('/tmp/hbsup-ui');
    await tester.pumpAndSettle();
    await tester.tap(find.text('Start backup'));
    await tester.pumpAndSettle();
    expect(find.textContaining('Backup failed'), findsOneWidget);
    expect(s.backupState, BackupState.failed);
  });

  testWidgets('cancel during a run reports cancelled', (tester) async {
    final adb = FakeAdb(
      run: (args) async => ProcessResult(0, 0,
          'List of devices attached\nSER9\tdevice\n', ''),
      shell: {
        'ls -l $byNameDir': '''
total 0
lrwxrwxrwx 1 root root 1970 boot -> /dev/block/sde17
''',
        'cat /sys/class/block/sde17/size': '2048',
      },
    );
    final s = fakeState(adb: adb);
    // the stream cancels the run after the first chunk lands
    adb.setExec('dd if=/dev/block/sde17 bs=4M 2>/dev/null',
        cancellingStream(s.cancel));
    bigView(tester);
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pumpAndSettle();
    await s.connect('SER9');
    await tester.pumpAndSettle();
    await s.setDestination('/tmp/hbsup-ui');
    await tester.pumpAndSettle();
    await tester.tap(find.text('Start backup'));
    await tester.pumpAndSettle();
    expect(find.text('Backup cancelled'), findsOneWidget);
    expect(s.backupState, BackupState.cancelled);
  });

  testWidgets('picker success stores the folder', (tester) async {
    final s = fakeState(pickDir: () async => '/tmp/picked');
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pumpAndSettle();
    await s.connect('SER9');
    await tester.pumpAndSettle();
    await tester.tap(find.text('Choose folder'));
    await tester.pumpAndSettle();
    expect(s.destDir, '/tmp/picked');
  });

  testWidgets('scanning shows the spinner', (tester) async {
    final gate = Completer<ProcessResult>();
    final s =
        fakeState(adb: FakeAdb(run: (a) => gate.future));
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pump();
    expect(find.byType(CircularProgressIndicator), findsOneWidget);
    gate.complete(ProcessResult(0, 0, '', ''));
    await tester.pumpAndSettle();
  });

  testWidgets('partition load shows the reading state', (tester) async {
    final gate = Completer<void>();
    final s = fakeState(
        adb: FakeAdb(
          run: (a) async => ProcessResult(
              0, 0, 'List of devices attached\nS9\tdevice\n', ''),
          shell: adbShell,
          shellDelay: () => gate.future,
        ));
    bigView(tester);
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pumpAndSettle();
    final connecting = s.connect('S9');
    await tester.pump();
    expect(find.text('Reading the partition table...'), findsOneWidget);
    gate.complete();
    await connecting;
    await tester.pumpAndSettle();
  });

  testWidgets('a running dump shows progress and can cancel',
      (tester) async {
    final gate = Completer<void>();
    late AppState s;
    final adb = FakeAdb(
      run: (a) async => ProcessResult(
          0, 0, 'List of devices attached\nS9\tdevice\n', ''),
      shell: {
        'ls -l $byNameDir': '''
total 0
lrwxrwxrwx 1 root root 1970 boot -> /dev/block/sde17
''',
        'cat /sys/class/block/sde17/size': '2',
      },
    );
    s = fakeState(adb: adb);
    adb.setExec(
        'dd if=/dev/block/sde17 bs=4M 2>/dev/null',
        () async* {
          yield Uint8List(512);
          await gate.future;
          yield Uint8List(512);
        }());
    bigView(tester);
    await tester.pumpWidget(HbsupApp(state: s));
    await tester.pumpAndSettle();
    await s.connect('S9');
    await s.setDestination('/tmp/hbsup-ui');
    await tester.pumpAndSettle();

    await tester.tap(find.text('Start backup'));
    await tester.pump();
    expect(find.text('Backing up...'), findsOneWidget);
    expect(find.text('0 of 1 partitions'), findsOneWidget);
    expect(find.text('Cancel'), findsOneWidget);

    await tester.tap(find.text('Cancel'));
    gate.complete();
    await tester.pumpAndSettle();
    expect(find.text('Backup cancelled'), findsOneWidget);
  });

  testWidgets('zh locale renders both pages', (tester) async {
    final s = fakeState();
    await tester.pumpWidget(HbsupApp(state: s, locale: const Locale('zh')));
    await tester.pumpAndSettle();
    expect(find.text('HBSUP'), findsOneWidget);
    expect(find.text('连接头显'), findsOneWidget);
    bigView(tester);
    await s.connect('SER9');
    await tester.pumpAndSettle();
    await s.setDestination('/tmp/x');
    await tester.pumpAndSettle();
    expect(find.text('分区'), findsOneWidget);
    expect(find.text('开始备份'), findsOneWidget);
  });
}
