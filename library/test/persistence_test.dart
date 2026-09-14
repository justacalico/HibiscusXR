import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_library/src/models.dart';
import 'package:pn2_library/src/persistence.dart';

void main() {
  test('snapshot json roundtrip', () {
    const snap = LibrarySnapshot(
      pinned: ['a', 'b'],
      order: ['b', 'a', 'c'],
      groups: [
        AppGroup(id: 'g1', name: 'Games', members: ['a', 'c']),
      ],
    );
    final back = LibrarySnapshot.fromJson(snap.toJson());
    expect(back.version, 1);
    expect(back.pinned, ['a', 'b']);
    expect(back.order, ['b', 'a', 'c']);
    expect(back.groups.single.name, 'Games');
    expect(back.groups.single.members, ['a', 'c']);
  });

  test('fromJson tolerates missing and wrong-typed fields', () {
    final snap = LibrarySnapshot.fromJson({'pinned': 'nope', 'groups': 5});
    expect(snap.pinned, isEmpty);
    expect(snap.groups, isEmpty);
    expect(snap.order, isEmpty);
  });

  test('memory persistence stores and counts saves', () async {
    final p = MemoryPersistence();
    expect(await p.load(), isNull);
    await p.save(const LibrarySnapshot(pinned: ['a']));
    await p.save(const LibrarySnapshot(pinned: ['b']));
    expect(p.saves, 2);
    expect((await p.load())!.pinned, ['b']);
  });
}
