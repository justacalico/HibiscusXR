import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_library/src/library_store.dart';
import 'package:pn2_library/src/models.dart';
import 'package:pn2_library/src/persistence.dart';

AppEntry app(
  String pkg, {
  String? label,
  bool system = false,
  int installed = 0,
  int updated = 0,
}) => AppEntry(
  packageName: pkg,
  label: label ?? pkg,
  isSystem: system,
  firstInstallTime: installed,
  lastUpdateTime: updated,
);

LibraryStore store({int idSeq = 0}) {
  var n = idSeq;
  return LibraryStore(idGen: () => 'g${n++}');
}

void main() {
  group('setApps', () {
    test('sorts by name by default', () {
      final s = store()
        ..setApps([app('b', label: 'Beta'), app('a', label: 'Alpha')]);
      expect(s.visible.map((a) => a.packageName), ['a', 'b']);
    });

    test('prunes pins, order and group members for missing packages', () {
      final s = store();
      s.setApps([app('a'), app('b'), app('c')]);
      s.togglePin('a');
      final g = s.createGroup('favs');
      s.addToGroup(g, 'a');
      s.addToGroup(g, 'b');
      s.moveApp('c', 0);

      s.setApps([app('b'), app('c')]);
      expect(s.pinned, isEmpty);
      expect(s.groupById(g)!.members, ['b']);
      expect(s.visible.map((a) => a.packageName), isNot(contains('a')));
    });
  });

  group('query', () {
    test('matches label case-insensitively', () {
      final s = store()
        ..setApps([app('a', label: 'Camera'), app('b', label: 'Files')]);
      s.setQuery('cam');
      expect(s.visible.single.packageName, 'a');
    });

    test('matches package name', () {
      final s = store()
        ..setApps([
          app('com.x.camera', label: 'Cam'),
          app('y', label: 'Files'),
        ]);
      s.setQuery('com.x');
      expect(s.visible.single.packageName, 'com.x.camera');
    });

    test('folds diacritics', () {
      final s = store()
        ..setApps([app('a', label: 'Pokémon'), app('b', label: 'Other')]);
      s.setQuery('pokemon');
      expect(s.visible.single.packageName, 'a');
    });

    test('empty query returns everything', () {
      final s = store()..setApps([app('a'), app('b')]);
      s.setQuery('  ');
      expect(s.visible, hasLength(2));
    });
  });

  group('filters', () {
    test('pinned shows only pinned', () {
      final s = store()..setApps([app('a'), app('b')]);
      s.togglePin('b');
      s.setFilter(const FilterPinned());
      expect(s.visible.single.packageName, 'b');
    });

    test('user and system split on isSystem', () {
      final s = store()..setApps([app('a'), app('b', system: true)]);
      s.setFilter(const FilterUserApps());
      expect(s.visible.single.packageName, 'a');
      s.setFilter(const FilterSystemApps());
      expect(s.visible.single.packageName, 'b');
    });

    test('group shows members in member order', () {
      final s = store()
        ..setApps([app('a', label: 'Aaa'), app('b', label: 'Zzz')]);
      final g = s.createGroup('g');
      s.addToGroup(g, 'b');
      s.addToGroup(g, 'a');
      s.setFilter(FilterGroup(g));
      s.setSort(LibrarySort.custom);
      expect(s.visible.map((a) => a.packageName), ['b', 'a']);
    });

    test('deleting the active group falls back to all', () {
      final s = store()..setApps([app('a')]);
      final g = s.createGroup('x');
      s.setFilter(FilterGroup(g));
      s.deleteGroup(g);
      expect(s.filter, isA<FilterAll>());
    });

    test('countFor ignores the query', () {
      final s = store()..setApps([app('a'), app('b', system: true)]);
      s.setQuery('zzz');
      expect(s.countFor(const FilterAll()), 2);
      expect(s.countFor(const FilterSystemApps()), 1);
    });
  });

  group('sort', () {
    test('name desc reverses', () {
      final s = store()
        ..setApps([app('a', label: 'Alpha'), app('b', label: 'Beta')]);
      s.setSort(LibrarySort.nameDesc);
      expect(s.visible.map((a) => a.packageName), ['b', 'a']);
    });

    test('newest first uses install time', () {
      final s = store()
        ..setApps([app('a', installed: 5), app('b', installed: 9)]);
      s.setSort(LibrarySort.newestFirst);
      expect(s.visible.first.packageName, 'b');
    });

    test('recently updated uses update time', () {
      final s = store()..setApps([app('a', updated: 1), app('b', updated: 7)]);
      s.setSort(LibrarySort.recentlyUpdated);
      expect(s.visible.first.packageName, 'b');
    });

    test('custom follows the manual order', () {
      final s = store()..setApps([app('a'), app('b'), app('c')]);
      s.setSort(LibrarySort.custom);
      s.moveApp('c', 0);
      expect(s.visible.map((a) => a.packageName), ['c', 'a', 'b']);
    });
  });

  group('pins', () {
    test('pinned apps float to the top of flat views', () {
      final s = store()
        ..setApps([app('a', label: 'Alpha'), app('b', label: 'Zulu')]);
      s.togglePin('b');
      expect(s.visible.map((a) => a.packageName), ['b', 'a']);
    });

    test('pins do not reorder inside group views', () {
      final s = store()..setApps([app('a'), app('b')]);
      final g = s.createGroup('g');
      s.addToGroup(g, 'a');
      s.addToGroup(g, 'b');
      s.togglePin('b');
      s.setFilter(FilterGroup(g));
      s.setSort(LibrarySort.custom);
      expect(s.visible.map((a) => a.packageName), ['a', 'b']);
    });

    test('togglePin is symmetric', () {
      final s = store()..setApps([app('a')]);
      s.togglePin('a');
      expect(s.isPinned('a'), isTrue);
      s.togglePin('a');
      expect(s.isPinned('a'), isFalse);
    });
  });

  group('moveApp', () {
    test('switches sort to custom', () {
      final s = store()..setApps([app('a'), app('b')]);
      expect(s.sort, LibrarySort.nameAsc);
      s.moveApp('b', 0);
      expect(s.sort, LibrarySort.custom);
    });

    test('reorders group members when a group is active', () {
      final s = store()..setApps([app('a'), app('b'), app('c')]);
      final g = s.createGroup('g');
      for (final p in ['a', 'b', 'c']) {
        s.addToGroup(g, p);
      }
      s.setFilter(FilterGroup(g));
      s.setSort(LibrarySort.custom);
      s.moveApp('c', 0);
      expect(s.groupById(g)!.members, ['c', 'a', 'b']);
    });

    test('keeps hidden items in place while a query is active', () {
      final s = store()
        ..setApps([
          app('a', label: 'apple'),
          app('x', label: 'hidden'),
          app('b', label: 'apricot'),
        ]);
      s.setSort(LibrarySort.custom);
      s.setQuery('ap');
      // visible: apple, apricot -> move apricot above apple
      s.moveApp('b', 0);
      s.setQuery('');
      expect(s.visible.map((a) => a.packageName), ['b', 'x', 'a']);
    });
  });

  group('groups', () {
    test('rename updates the name', () {
      final s = store();
      final g = s.createGroup('old');
      s.renameGroup(g, 'new');
      expect(s.groupById(g)!.name, 'new');
    });

    test('addToGroup dedupes', () {
      final s = store()..setApps([app('a')]);
      final g = s.createGroup('g');
      s.addToGroup(g, 'a');
      s.addToGroup(g, 'a');
      expect(s.groupById(g)!.members, ['a']);
    });

    test('removeFromGroup drops only that member', () {
      final s = store()..setApps([app('a'), app('b')]);
      final g = s.createGroup('g');
      s.addToGroup(g, 'a');
      s.addToGroup(g, 'b');
      s.removeFromGroup(g, 'a');
      expect(s.groupById(g)!.members, ['b']);
    });
  });

  group('snapshot', () {
    test('roundtrips pins, order and groups', () {
      final s = store()..setApps([app('a'), app('b')]);
      s.togglePin('a');
      final g = s.createGroup('favs');
      s.addToGroup(g, 'b');
      s.setSort(LibrarySort.custom);
      s.moveApp('b', 0);

      final json = s.snapshot().toJson();
      final snap = LibrarySnapshot.fromJson(json);

      final s2 = store()..setApps([app('a'), app('b')]);
      s2.restore(snap);
      s2.setSort(LibrarySort.custom);
      expect(s2.isPinned('a'), isTrue);
      // 'a' is pinned so it floats above the custom order
      expect(s2.visible.map((a) => a.packageName), ['a', 'b']);
      expect(s2.groups.single.name, 'favs');
      expect(s2.groups.single.members, ['b']);
    });

    test('restore prunes packages that no longer exist', () {
      final snap = LibrarySnapshot.fromJson({
        'version': 1,
        'pinned': ['gone'],
        'order': ['gone', 'a'],
        'groups': [
          {
            'id': 'g',
            'name': 'x',
            'members': ['gone', 'a'],
          },
        ],
      });
      final s = store()..setApps([app('a')]);
      s.restore(snap);
      expect(s.pinned, isEmpty);
      expect(s.groups.single.members, ['a']);
    });
  });
}
