import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_library/src/library_controller.dart';
import 'package:pn2_library/src/models.dart';
import 'package:pn2_library/src/persistence.dart';
import 'package:pn2_library/src/platform/fake_app_source.dart';

void main() {
  test('init loads apps and reaches ready', () async {
    final src = FakeAppSource();
    final c = LibraryController(source: src, persistence: MemoryPersistence());
    addTearDown(c.dispose);
    await c.init();
    expect(c.state, LoadState.ready);
    expect(c.store.visible, isNotEmpty);
    expect(src.listCalls, 1);
  });

  test('init failure lands in failed state and retry recovers', () async {
    final src = FakeAppSource()..failNextList = true;
    final c = LibraryController(source: src, persistence: MemoryPersistence());
    addTearDown(c.dispose);
    await c.init();
    expect(c.state, LoadState.failed);
    await c.init();
    expect(c.state, LoadState.ready);
  });

  test('restores pins before the catalog arrives', () async {
    final mem = MemoryPersistence(
        const LibrarySnapshot(pinned: ['com.vr.home']));
    final c = LibraryController(source: FakeAppSource(), persistence: mem);
    addTearDown(c.dispose);
    await c.init();
    expect(c.store.isPinned('com.vr.home'), isTrue);
  });

  test('store mutations are persisted', () async {
    final mem = MemoryPersistence();
    final c = LibraryController(source: FakeAppSource(), persistence: mem);
    addTearDown(c.dispose);
    await c.init();
    c.store.togglePin('com.vr.home');
    await Future<void>.delayed(Duration.zero);
    expect(mem.saves, greaterThan(0));
    expect(mem.stored!.pinned, contains('com.vr.home'));
  });

  test('package change events refresh the catalog', () async {
    final src = FakeAppSource(apps: [
      const AppEntry(packageName: 'a', label: 'A'),
    ]);
    addTearDown(src.dispose);
    final c = LibraryController(source: src, persistence: MemoryPersistence());
    addTearDown(c.dispose);
    await c.init();
    expect(c.store.visible.single.packageName, 'a');

    src.apps = [
      const AppEntry(packageName: 'a', label: 'A'),
      const AppEntry(packageName: 'b', label: 'B'),
    ];
    src.emitChange();
    await Future<void>.delayed(Duration.zero);
    expect(c.store.visible, hasLength(2));
    expect(src.listCalls, 2);
  });

  test('icon cache dedupes platform calls', () async {
    final src = FakeAppSource();
    final c = LibraryController(source: src, persistence: MemoryPersistence());
    addTearDown(c.dispose);
    await c.init();
    await c.icons.get('a');
    await c.icons.get('a');
    expect(src.iconCalls, 1);
    c.icons.invalidate('a');
    await c.icons.get('a');
    expect(src.iconCalls, 2);
  });

  test('actions delegate to the source', () async {
    final src = FakeAppSource();
    addTearDown(src.dispose);
    final c = LibraryController(source: src, persistence: MemoryPersistence());
    addTearDown(c.dispose);
    await c.init();
    final app = c.store.visible.first;
    await c.launch(app);
    await c.openAppInfo(app);
    await c.uninstall(app);
    await Future<void>.delayed(Duration.zero);
    expect(src.launched, [app.packageName]);
    expect(src.infoOpened, [app.packageName]);
    expect(src.uninstalled, [app.packageName]);
    expect(c.store.visible.any((a) => a.packageName == app.packageName),
        isFalse);
  });
}
