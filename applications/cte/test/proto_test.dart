import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/src/models.dart';
import 'package:hibiscus_cte/src/proto.dart';

Stream<Uint8List> feed(List<String> chunks) =>
    Stream.fromIterable(chunks.map((c) => utf8.encode(c)));

void main() {
  test('CTRL line parses all fields', () {
    final s = parseCtrlLine(
        'CTRL 0 live=1 batt=77 px=0.1 py=-0.2 pz=0.3 qx=0.1 qy=0.2 qz=0.3 qw=0.9 trk=1 sx=128 sy=140 a=1 b=0 menu=0 sys=0 trig=1 grip=0');
    expect(s, isNotNull);
    expect(s!.index, 0);
    expect(s.connected, isTrue);
    expect(s.battery, 77);
    expect(s.pose.x, closeTo(0.1, 1e-6));
    expect(s.pose.qw, closeTo(0.9, 1e-6));
    expect(s.tracked, isTrue);
    expect(s.stickX, 128);
    expect(s.buttonA, isTrue);
    expect(s.trigger, isTrue);
    expect(s.grip, isFalse);
  });

  test('CTRL line rejects junk', () {
    expect(parseCtrlLine('hello'), isNull);
    expect(parseCtrlLine('CTRL'), isNull);
    expect(parseCtrlLine('CTRL x live=1'), isNull);
  });

  test('wire reader splits lines and frame blobs', () async {
    final png = List<int>.filled(5, 9);
    final wire = CteWireReader(feed([
      'CTE/1\n',
      'FRAME 5\n',
    ]));
    final msgs = <CteMsg>[];
    final sub = wire.messages.listen(msgs.add);
    // push the blob bytes after a tick
    await Future<void>.delayed(Duration.zero);
    // feed again through a combined stream instead
    await sub.cancel();
    await wire.dispose();

    final wire2 = CteWireReader(Stream.fromIterable([
      utf8.encode('CTE/1\nFRAME 5\n'),
      Uint8List.fromList(png),
      utf8.encode('\nCTRL 1 live=0 batt=10\n'),
    ]));
    final out = await wire2.messages.toList();
    expect(out.whereType<CteFrameMsg>().single.png.length, 5);
    final ctrl = out.whereType<CteCtrlMsg>().single.state;
    expect(ctrl.index, 1);
    expect(ctrl.connected, isFalse);
  });

  test('wire reader handles a blob split across chunks', () async {
    final c = StreamController<Uint8List>();
    final wire = CteWireReader(c.stream);
    final out = <CteMsg>[];
    wire.messages.listen(out.add);
    c.add(utf8.encode('FRAME 4\n'));
    c.add(Uint8List.fromList([1, 2]));
    await Future<void>.delayed(Duration.zero);
    expect(out, isEmpty); // still waiting for the rest of the blob
    c.add(Uint8List.fromList([3, 4]));
    c.add(utf8.encode('LOG hi\n'));
    await Future<void>.delayed(Duration.zero);
    await c.close();
    expect(out.whereType<CteFrameMsg>().single.png, [1, 2, 3, 4]);
    expect(out.whereType<CteLogMsg>().single.line, 'hi');
  });

  test('+JSON and +TEXT blobs decode', () async {
    final json = utf8.encode('{"model":"Pico Neo 2","tracking":"dof6"}');
    final wire = CteWireReader(Stream.fromIterable([
      utf8.encode('+JSON ${json.length}\n'),
      Uint8List.fromList(json),
      utf8.encode('+TEXT 4\n'),
      utf8.encode('abcd'),
    ]));
    final out = await wire.messages.toList();
    final info = out.whereType<CteInfoMsg>().single.json;
    expect(info['model'], 'Pico Neo 2');
    expect(out.whereType<CteTextMsg>().single.text, 'abcd');
  });

  test('+OK and +ERR surface', () async {
    final wire = CteWireReader(feed(['+OK done\n', '+ERR broken\n']));
    final out = await wire.messages.toList();
    expect(out.whereType<CteResultMsg>().single.message, 'done');
    expect(out.whereType<CteErrorMsg>().single.message, 'broken');
  });

  test('deviceInfoFromJson reads props table', () {
    final info = deviceInfoFromJson({
      'model': 'Pico Neo 2',
      'tracking': 'dof6',
      'battery': 66,
      'props': {'ro.product.device': 'PICOA7B10', 'ro.hibiscus.version': 'v1'}
    }, address: '10.0.0.2:7340');
    expect(info.headsetName, 'Pico Neo 2');
    expect(info.trackingMode, TrackingMode.dof6);
    expect(info.batteryLevel, 66);
    expect(info.address, '10.0.0.2:7340');
  });
}
