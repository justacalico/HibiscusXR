import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/src/envs/env_info.dart';
import 'package:pn2_settings/src/envs/env_source.dart';
import 'package:pn2_settings/src/envs/env_zip.dart';
import 'package:pn2_settings/src/models.dart';
import 'package:pn2_settings/src/persistence.dart';
import 'package:pn2_settings/src/platform/fake_settings_source.dart';
import 'package:pn2_settings/src/settings_controller.dart';
import 'package:pn2_settings/src/settings_store.dart';

// smallest legal zip: local headers + data, central dir, EOCD.
// method 0 stores, 8 carries raw deflate.
class _Ent {
  _Ent(this.name, this.data, this.method);
  final String name;
  final List<int> data;
  final int method;
}

void _w16(BytesBuilder b, int v) => b.add([v & 0xff, (v >> 8) & 0xff]);
void _w32(BytesBuilder b, int v) =>
    b.add([for (var i = 0; i < 4; ++i) (v >> (8 * i)) & 0xff]);

Uint8List _makeZip(List<_Ent> ents) {
  final b = BytesBuilder();
  final offs = <int>[];
  final payloads = <List<int>>[];
  for (final e in ents) {
    final payload = e.method == 8
        ? ZLibEncoder(raw: true).convert(e.data)
        : e.data;
    payloads.add(payload);
    offs.add(b.length);
    _w32(b, 0x04034b50);
    _w16(b, 20);
    _w16(b, 0x0800);
    _w16(b, e.method);
    _w16(b, 0);
    _w16(b, 0);
    _w32(b, 0);
    _w32(b, payload.length);
    _w32(b, e.data.length);
    _w16(b, e.name.length);
    _w16(b, 0);
    b.add(utf8.encode(e.name));
    b.add(payload);
  }
  final cdOff = b.length;
  for (var i = 0; i < ents.length; ++i) {
    final e = ents[i];
    _w32(b, 0x02014b50);
    _w16(b, 20);
    _w16(b, 20);
    _w16(b, 0x0800);
    _w16(b, e.method);
    _w16(b, 0);
    _w16(b, 0);
    _w32(b, 0);
    _w32(b, payloads[i].length);
    _w32(b, e.data.length);
    _w16(b, e.name.length);
    _w16(b, 0);
    _w16(b, 0);
    _w16(b, 0);
    _w16(b, 0);
    _w32(b, 0);
    _w32(b, offs[i]);
    b.add(utf8.encode(e.name));
  }
  final cdSize = b.length - cdOff;
  _w32(b, 0x06054b50);
  _w16(b, 0);
  _w16(b, 0);
  _w16(b, ents.length);
  _w16(b, ents.length);
  _w32(b, cdSize);
  _w32(b, cdOff);
  _w16(b, 0);
  return b.toBytes();
}

Uint8List _envZip({
  String obj = 'o Floor\nv 0 0 0\nv 1 0 0\nv 0 0 1\nf 1 2 3\n',
  String? json,
  List<int>? png,
}) => _makeZip([
  if (obj.isNotEmpty) _Ent('map.obj', utf8.encode(obj), 8),
  if (json != null) _Ent('map.json', utf8.encode(json), 0),
  if (png != null) _Ent('map.png', png, 0),
]);

void main() {
  test('zip reader lists and extracts stored and deflated entries', () {
    final zip = _makeZip([
      _Ent('a.txt', utf8.encode('hello'), 0),
      _Ent('b.txt', List.filled(4096, 120), 8),
    ]);
    final entries = zipEntries(zip)!;
    expect(entries.map((e) => e.name), ['a.txt', 'b.txt']);
    expect(entries[0].method, 0);
    expect(entries[1].method, 8);
    expect(utf8.decode(zipEntryData(zip, 'a.txt')!), 'hello');
    expect(zipEntryData(zip, 'b.txt')!.length, 4096);
    expect(zipEntryData(zip, 'missing'), isNull);
    expect(zipEntries(Uint8List.fromList(utf8.encode('junk'))), isNull);
  });

  test('env ids accept filenames and reject path tricks', () {
    expect(envIdValid('skyloft'), isTrue);
    expect(envIdValid('my-room_v2.1'), isTrue);
    expect(envIdValid(''), isFalse);
    expect(envIdValid('has space'), isFalse);
    expect(envIdValid('../evil'), isFalse);
    expect(envIdValid('a/b'), isFalse);
    expect(envIdValid('x..y'), isFalse);
    expect(envIdFromZipName('/tmp/skyloft.zip'), 'skyloft');
    expect(envIdFromZipName('ROOM.ZIP'), 'ROOM');
    expect(envSelOr(null), kEnvPassthrough);
    expect(envSelOr(''), kEnvPassthrough);
    expect(envSelOr('skyloft'), 'skyloft');
  });

  test('envFromZip reads map.json, thumbnail and map presence', () {
    const json =
        '{"name":"Sky Loft","version":"1.2","license":"CC0",'
        '"git":"https://example.com/repo","homepage":"https://example.com",'
        '"created":"2026-09-01"}';
    final env = envFromZip(
      'skyloft',
      _envZip(json: json, png: [1, 2, 3]),
    );
    expect(env.name, 'Sky Loft');
    expect(env.label, 'Sky Loft');
    expect(env.version, '1.2');
    expect(env.license, 'CC0');
    expect(env.git, 'https://example.com/repo');
    expect(env.homepage, 'https://example.com');
    expect(env.created, '2026-09-01');
    expect(env.thumb, [1, 2, 3]);
    expect(env.hasMap, isTrue);
    expect(env.selectable, isTrue);
  });

  test('envFromZip degrades: no json, no map, bad zip, bad name', () {
    final plain = envFromZip('bare', _envZip());
    expect(plain.label, 'bare');
    expect(plain.thumb, isNull);
    expect(plain.selectable, isTrue);

    final noMap = envFromZip('empty', _envZip(obj: ''));
    expect(noMap.hasMap, isFalse);
    expect(noMap.selectable, isFalse);

    final junk = envFromZip('junk', Uint8List.fromList(utf8.encode('x')));
    expect(junk.readable, isFalse);
    expect(junk.selectable, isFalse);

    final spaced = envFromZip('my env', _envZip());
    expect(spaced.selectable, isFalse);
  });

  test('DirEnvSource lists, parses and removes zips in a real dir', () async {
    final dir = await Directory.systemTemp.createTemp('envs');
    addTearDown(() => dir.delete(recursive: true));
    final source = DirEnvSource(dir.path);

    expect(await source.list(), isEmpty);

    await File('${dir.path}/skyloft.zip').writeAsBytes(
      _envZip(json: '{"name":"Sky Loft"}'),
    );
    await File('${dir.path}/note.txt').writeAsString('ignored');

    final envs = await source.list();
    expect(envs.map((e) => e.id), ['skyloft']);
    expect(envs.single.name, 'Sky Loft');

    expect(await source.remove('skyloft'), isTrue);
    expect(await File('${dir.path}/skyloft.zip').exists(), isFalse);
    expect(await source.remove('skyloft'), isFalse);
    expect(await source.remove('../evil'), isFalse);
  });

  test('controller selects and deletes environments', () async {
    final dir = await Directory.systemTemp.createTemp('envs');
    addTearDown(() => dir.delete(recursive: true));
    await File('${dir.path}/skyloft.zip').writeAsBytes(_envZip());

    final source = FakeSettingsSource();
    addTearDown(source.dispose);
    final c = SettingsController(
      source: source,
      persistence: MemoryPersistence(),
      envs: DirEnvSource(dir.path),
    );
    await c.refreshEnvs();
    expect(c.store.envOptions.map((e) => e.id), ['skyloft']);

    await c.selectEnv('skyloft');
    expect(source.textsSet, contains((ItemId.homeEnv, 'skyloft')));
    expect(c.store.homeEnv, 'skyloft');

    // deleting the active pick falls back to passthrough
    await c.deleteEnv('skyloft');
    expect(source.textsSet, contains((ItemId.homeEnv, kEnvPassthrough)));
    expect(c.store.homeEnv, kEnvPassthrough);
    expect(c.store.envOptions, isEmpty);
  });

  test('environment section rescans on entry', () async {
    final dir = await Directory.systemTemp.createTemp('envs');
    addTearDown(() => dir.delete(recursive: true));
    await File('${dir.path}/a.zip').writeAsBytes(_envZip());

    final source = FakeSettingsSource();
    addTearDown(source.dispose);
    final c = SettingsController(
      source: source,
      persistence: MemoryPersistence({'section': 'environment'}),
      envs: DirEnvSource(dir.path),
      store: SettingsStore(),
    );
    await c.start();
    // start() rescans the restored section; the dir read lands async
    for (var i = 0; i < 100 && c.store.envOptions.isEmpty; ++i) {
      await Future<void>.delayed(const Duration(milliseconds: 10));
    }
    expect(c.store.envOptions.map((e) => e.id), ['a']);
    await c.dispose();
  });
}
