import 'dart:async';

import 'package:flutter/foundation.dart';

import 'models.dart';
import 'persistence.dart';
import 'platform/app_source.dart';
import 'platform/icon_cache.dart';
import 'library_store.dart';

enum LoadState { loading, ready, failed }

/// Glues [AppSource], [LibraryPersistence] and [LibraryStore] together.
/// The UI only talks to this object: it owns the store, tracks the load
/// state and persists every mutation of pins/order/groups.
class LibraryController extends ChangeNotifier {
  LibraryController({
    required AppSource source,
    required this._persistence,
    LibraryStore? store,
  }) : _source = source,
       store = store ?? LibraryStore(),
       icons = IconCache(source);

  final AppSource _source;
  final LibraryPersistence _persistence;
  final LibraryStore store;
  final IconCache icons;

  LoadState _state = LoadState.loading;
  Object? _error;
  StreamSubscription<void>? _sub;
  bool _restored = false;
  int _generation = 0;

  LoadState get state => _state;
  Object? get error => _error;

  /// Bumped every time the catalog is re-read. Tiles key on it so stale
  /// icons get re-decoded after installs and removals.
  int get generation => _generation;

  /// Restores the snapshot, loads the catalog, then keeps the list in sync
  /// with package changes. Safe to call again after a failure.
  Future<void> init() async {
    _state = LoadState.loading;
    _error = null;
    notifyListeners();
    try {
      if (!_restored) {
        final snap = await _persistence.load();
        if (snap != null) store.restore(snap);
        _restored = true;
        store.addListener(_persist);
      }
      store.setApps(await _source.listApps());
      icons.invalidate();
      _generation++;
      _sub ??= _source.changes.listen((_) => refresh());
      _state = LoadState.ready;
    } catch (e) {
      _error = e;
      _state = LoadState.failed;
    }
    notifyListeners();
  }

  /// Re-reads the catalog after a package broadcast. Failures are kept
  /// quiet - the last good list stays on screen.
  Future<void> refresh() async {
    try {
      store.setApps(await _source.listApps());
      icons.invalidate();
      _generation++;
      notifyListeners();
    } catch (_) {
      // keep the stale list; a later broadcast will retry
    }
  }

  Future<bool> launch(AppEntry app) => _source.launch(app);

  Future<bool> uninstall(AppEntry app) => _source.uninstall(app);

  Future<bool> openAppInfo(AppEntry app) => _source.openAppInfo(app);

  Future<bool> installApk() => _source.pickAndInstallApk();

  void _persist() {
    unawaited(_persistence.save(store.snapshot()));
  }

  @override
  void dispose() {
    _sub?.cancel();
    store.removeListener(_persist);
    super.dispose();
  }
}
