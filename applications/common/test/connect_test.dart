import 'package:common/common.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

Widget wrap(Widget child) =>
    MaterialApp(home: Scaffold(body: child));

const device1 = AdbDeviceRow('SER1', 'device', 'Headset_One');
const device2 = AdbDeviceRow('SER2', 'offline', '');
const rows = [device1, device2];

void main() {
  group('AdbScanCard', () {
    testWidgets('renders title, devices and a working refresh button',
        (tester) async {
      var refreshed = 0;
      await tester.pumpWidget(wrap(AdbScanCard(
        title: 'USB',
        refreshTooltip: 'Refresh',
        noDevicesLabel: 'None',
        devices: rows,
        scanning: false,
        onRefresh: () => refreshed++,
        tileBuilder: (d) => Text(d.serial),
      )));
      expect(find.text('USB'), findsOneWidget);
      expect(find.text('SER1'), findsOneWidget);
      expect(find.text('SER2'), findsOneWidget);
      await tester.tap(find.byTooltip('Refresh'));
      expect(refreshed, 1);
    });

    testWidgets('empty list shows the no-devices label', (tester) async {
      await tester.pumpWidget(wrap(AdbScanCard(
        title: 'USB',
        refreshTooltip: 'Refresh',
        noDevicesLabel: 'No adb devices',
        devices: const [],
        scanning: false,
        onRefresh: () {},
        tileBuilder: (d) => Text(d.serial),
      )));
      expect(find.text('No adb devices'), findsOneWidget);
    });

    testWidgets('scanning without a label swaps refresh for a spinner',
        (tester) async {
      await tester.pumpWidget(wrap(AdbScanCard(
        title: 'USB',
        refreshTooltip: 'Refresh',
        noDevicesLabel: 'None',
        devices: rows,
        scanning: true,
        onRefresh: () {},
        tileBuilder: (d) => Text(d.serial),
      )));
      expect(find.byType(CircularProgressIndicator), findsOneWidget);
      expect(find.byType(IconButton), findsNothing);
      expect(find.text('SER1'), findsOneWidget);
    });

    testWidgets('scanning label replaces the device list', (tester) async {
      await tester.pumpWidget(wrap(AdbScanCard(
        title: 'USB',
        refreshTooltip: 'Refresh',
        noDevicesLabel: 'None',
        scanningLabel: 'Scanning...',
        devices: rows,
        scanning: true,
        busy: true,
        onRefresh: () {},
        tileBuilder: (d) => Text(d.serial),
      )));
      expect(find.text('Scanning...'), findsOneWidget);
      expect(find.text('SER1'), findsNothing);
      final button =
          tester.widget<IconButton>(find.byType(IconButton));
      expect(button.onPressed, isNull);
    });

    testWidgets('busy disables the refresh button', (tester) async {
      await tester.pumpWidget(wrap(AdbScanCard(
        title: 'USB',
        refreshTooltip: 'Refresh',
        noDevicesLabel: 'None',
        devices: const [],
        scanning: false,
        busy: true,
        onRefresh: () {},
        tileBuilder: (d) => Text(d.serial),
      )));
      final button =
          tester.widget<IconButton>(find.byType(IconButton));
      expect(button.onPressed, isNull);
    });

    testWidgets('titleGap and emptyPadding pad the layout',
        (tester) async {
      await tester.pumpWidget(wrap(AdbScanCard(
        title: 'USB',
        refreshTooltip: 'Refresh',
        noDevicesLabel: 'None',
        devices: const [],
        scanning: false,
        onRefresh: () {},
        tileBuilder: (d) => Text(d.serial),
        titleGap: 12,
        emptyPadding: const EdgeInsets.symmetric(vertical: 12),
      )));
      expect(find.text('None'), findsOneWidget);
    });
  });

  group('AdbDeviceTile', () {
    testWidgets('shows the model and serial-only subtitle', (tester) async {
      await tester.pumpWidget(wrap(const AdbDeviceTile(device: device1)));
      expect(find.text('Headset_One'), findsOneWidget);
      expect(find.text('SER1'), findsOneWidget);
    });

    testWidgets('showState appends the state to the subtitle',
        (tester) async {
      await tester.pumpWidget(
          wrap(const AdbDeviceTile(device: device1, showState: true)));
      expect(find.text('SER1 - device'), findsOneWidget);
    });

    testWidgets('unknownLabel wins over serial for a missing model',
        (tester) async {
      await tester.pumpWidget(wrap(const AdbDeviceTile(
          device: device2, unknownLabel: 'Unknown headset')));
      expect(find.text('Unknown headset'), findsOneWidget);
    });

    testWidgets('falls back to the serial without model or label',
        (tester) async {
      await tester.pumpWidget(wrap(const AdbDeviceTile(device: device2)));
      expect(find.text('SER2'), findsNWidgets(2));
    });

    testWidgets('onTap fires', (tester) async {
      var taps = 0;
      await tester.pumpWidget(wrap(AdbDeviceTile(
          device: device1, onTap: () => taps++)));
      await tester.tap(find.byType(ListTile));
      expect(taps, 1);
    });
  });

  testWidgets('ConnectErrorCard shows the message', (tester) async {
    await tester.pumpWidget(wrap(const ConnectErrorCard(
        message: 'adb scan failed', color: Colors.red)));
    expect(find.text('adb scan failed'), findsOneWidget);
  });

  testWidgets('ConnectScaffold rebuilds when the listenable fires',
      (tester) async {
    final source = ValueNotifier<int>(0);
    await tester.pumpWidget(wrap(ConnectScaffold(
      listenable: source,
      children: const [Text('inner child')],
    )));
    expect(find.text('inner child'), findsOneWidget);
    source.value = 1;
    await tester.pump();
    expect(find.text('inner child'), findsOneWidget);
  });

  testWidgets('scanAfterFirstFrame fires once after mount',
      (tester) async {
    var scans = 0;
    await tester.pumpWidget(wrap(_ScanOnMount(scan: () => scans++)));
    expect(scans, 1);
    await tester.pump();
    expect(scans, 1);
  });
}

class _ScanOnMount extends StatefulWidget {
  const _ScanOnMount({required this.scan});
  final VoidCallback scan;

  @override
  State<_ScanOnMount> createState() => _ScanOnMountState();
}

class _ScanOnMountState extends State<_ScanOnMount> {
  @override
  void initState() {
    super.initState();
    scanAfterFirstFrame(widget.scan);
  }

  @override
  Widget build(BuildContext context) => const SizedBox();
}
