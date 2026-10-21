import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:hibiscus_cte/src/proto.dart';
import 'package:hibiscus_cte/src/transport/cte_link.dart';

import 'tar_fixture.dart';

/// Loopback cted: speaks the wire protocol so CteLink is exercised end to
/// end - banner, per-command sockets, framed blobs, install round-trip.
/// `dumpBlob` set = a new cted that answers DUMP; null = old build that
/// replies +ERR, exercising the partial-dump fallback.
class FakeCted {
  FakeCted._(this.server, this.dumpBlob);

  final ServerSocket server;
  final Uint8List? dumpBlob;
  final List<String> commands = [];

  int get port => server.port;

  static Future<FakeCted> start({Uint8List? dumpBlob}) async {
    final server = await ServerSocket.bind('127.0.0.1', 0);
    final fake = FakeCted._(server, dumpBlob);
    server.listen(fake._handle);
    return fake;
  }

  void _handle(Socket client) {
    client.write('${CteProto.banner}\n');
    utf8.decoder
        .bind(client.cast<List<int>>())
        .transform(const LineSplitter())
        .listen((line) {
      commands.add(line);
      try {
      if (line == CteProto.cmdPing) {
        client.write('${CteProto.pong}\n');
      } else if (line == 'INFO') {
        final body = utf8.encode(jsonEncode({
          'model': 'Pico Neo 2',
          'device': 'PICOA7B10',
          'tracking': 'dof6',
          'battery': 91,
        }));
        client.write('+JSON ${body.length}\n');
        client.add(body);
      } else if (line == 'PROPS') {
        final body = utf8.encode('[ro.product.model]: [Pico Neo 2]\n');
        client.write('+TEXT ${body.length}\n');
        client.add(body);
      } else if (line == 'FRAMES') {
        final png = List<int>.filled(8, 7);
        client.write('FRAME ${png.length}\n');
        client.add(Uint8List.fromList(png));
        client.write('FRAME ${png.length}\n');
        client.add(Uint8List.fromList(png));
      } else if (line == 'POSE') {
        client.write(
            'POSELOG HMD ts=1 qx=0 qy=0 qz=0 qw=1 px=1 py=2 pz=3 st=3\n');
        client.write(
            'POSELOG HMD ts=2 qx=0 qy=0 qz=0 qw=1 px=1.1 py=2 pz=3 st=3\n');
      } else if (line == 'CTRL') {
        client.write('CTRL 0 live=1 batt=88 px=0 py=0 pz=0 '
            'qx=0 qy=0 qz=0 qw=1 trk=1 sx=128 sy=128 '
            'a=0 b=0 menu=0 sys=0 trig=0 grip=0\n');
      } else if (line == 'LOG') {
        client.write('LOG I test: hello\nLOG D test: world\n');
      } else if (line.startsWith('INSTALL ')) {
        // consume the blob asynchronously; reply ok
        client.write('LOG reading apk\n+OK Success\n');
      } else if (line == 'DUMP' && dumpBlob != null) {
        client.write('+BLOB ${dumpBlob!.length}\n');
        client.add(dumpBlob!);
      } else if (!line.startsWith('INSTALL')) {
        client.write('+ERR unknown command\n');
      }
      // one-shot commands get closed like real cted; FRAMES/POSE/CTRL/LOG
      // keep streaming until the client walks away, and INSTALL still has
      // an incoming blob to swallow (decoded as stray lines - guarded so a
      // write to a half-closed socket never crashes the fake)
      const streaming = ['FRAMES', 'POSE', 'CTRL', 'LOG', 'INSTALL'];
      if (!streaming.contains(line.split(' ').first)) {
        Future<void>.delayed(Duration.zero)
            .then((_) => client.flush().then((_) => client.close()));
      }
      } catch (_) {
        // socket went away mid-reply - the blob bytes keep decoding as
        // junk lines, ignore writes to the dead socket
      }
    });
  }

  Future<void> close() => server.close();
}

void main() {
  late FakeCted cted;
  late CteLink link;

  setUp(() async {
    cted = await FakeCted.start();
    link = CteLink('127.0.0.1', port: cted.port);
  });
  tearDown(() => cted.close());

  test('probe detects a cted banner', () async {
    final found = await CteLink.probe('127.0.0.1', port: cted.port);
    expect(found, isNotNull);
    expect(cted.commands, contains(CteProto.cmdPing));
    final missing = await CteLink.probe('127.0.0.1', port: 1);
    expect(missing, isNull);
  });

  test('probe rejects a banner that never answers PING', () async {
    final server = await ServerSocket.bind('127.0.0.1', 0);
    addTearDown(server.close);
    server.listen((client) => client.write('${CteProto.banner}\n'));
    final found = await CteLink.probe('127.0.0.1',
        port: server.port, timeout: const Duration(milliseconds: 300));
    expect(found, isNull);
  });

  test('fetchInfo maps the json reply', () async {
    final info = await link.fetchInfo();
    expect(info.headsetName, 'Pico Neo 2');
    expect(info.trackingMode.name, 'dof6');
    expect(info.batteryLevel, 91);
    expect(info.address, '127.0.0.1:${cted.port}');
    expect(cted.commands, contains('INFO'));
  });

  test('fetchProps parses the text blob', () async {
    final props = await link.fetchProps();
    expect(props['ro.product.model'], 'Pico Neo 2');
  });

  test('frames yields PNG blobs', () async {
    final frames = await link.frames().take(2).toList();
    expect(frames.length, 2);
    expect(frames.first.length, 8);
  });

  test('poses parses forwarded log lines', () async {
    final poses = await link.poses().take(2).toList();
    expect(poses.length, 2);
    expect(poses.last.pose.x, closeTo(1.1, 1e-6));
    expect(poses.last.mode.name, 'dof6');
  });

  test('controllers collects per-index state', () async {
    final list = await link.controllers().first;
    expect(list[0].connected, isTrue);
    expect(list[0].battery, 88);
    expect(list[1].connected, isFalse);
  });

  test('logLines streams', () async {
    final lines = await link.logLines().take(2).toList();
    expect(lines, ['I test: hello', 'D test: world']);
  });

  test('installApk streams progress then result', () async {
    // write a fake apk to disk
    final f = File('${Directory.systemTemp.path}/cte_test.apk');
    await f.writeAsBytes(List.filled(16, 65));
    final out = await link.installApk(f.path).toList();
    expect(out, contains('reading apk'));
    expect(out.last, 'Success');
  });

  test('debugDump decodes the DUMP blob on new cted', () async {
    final blob = tarArchive({
      'meta.txt': utf8.encode('model=Pico Neo 2\n'),
      'getprop.txt': utf8.encode('[ro.x]: [y]\n'),
    });
    final cted2 = await FakeCted.start(dumpBlob: blob);
    addTearDown(cted2.close);
    final link2 = CteLink('127.0.0.1', port: cted2.port);

    final sections = await link2.debugDump().toList();
    expect(cted2.commands, contains('DUMP'));
    expect(sections.map((s) => s.name),
        containsAll(['meta.txt', 'getprop.txt']));
    expect(utf8.decode(sections.first.bytes), contains('Pico Neo 2'));
  });

  test('debugDump falls back to a partial bundle on old cted', () async {
    final sections = await link.debugDump().toList();
    expect(cted.commands, contains('DUMP'));
    final names = sections.map((s) => s.name).toList();
    expect(names, contains('PARTIAL.txt'));
    expect(names, contains('getprop.txt'));
    expect(names, contains('logcat.txt'));
    // the fake serves one frame too
    expect(names, contains('screenshot.png'));
    expect(utf8.decode(sections
        .firstWhere((s) => s.name == 'getprop.txt')
        .bytes), contains('Pico Neo 2'));
  });
}
