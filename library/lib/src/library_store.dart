import 'package:flutter/foundation.dart';

import 'models.dart';
import 'persistence.dart';
import 'text_norm.dart';

/// How the grid arranges apps. [custom] is the user-arranged order: the
/// manual order list for flat views, member order inside a group.
enum LibrarySort { custom, nameAsc, nameDesc, newestFirst, recentlyUpdated }

/// Which slice of the catalog is shown.
sealed class LibraryFilter {
  const LibraryFilter();
}

class FilterAll extends LibraryFilter {
  const FilterAll();
}

class FilterPinned extends LibraryFilter {
  const FilterPinned();
}

class FilterUserApps extends LibraryFilter {
  const FilterUserApps();
}

class FilterSystemApps extends LibraryFilter {
  const FilterSystemApps();
}

class FilterGroup extends LibraryFilter {
  const FilterGroup(this.groupId);
  final String groupId;

  @override
  bool operator ==(Object other) =>
      other is FilterGroup && other.groupId == groupId;

  @override
  int get hashCode => groupId.hashCode;
}

/// All app-library state: catalog, pins, groups, manual order, query,
/// sort and active filter. Pure Dart, no platform calls - every decision
/// the UI shows is computed here so tests can reach it.
class LibraryStore extends ChangeNotifier {
  LibraryStore({String Function()? idGen})
      : _idGen = idGen ?? _defaultIdGen;

  static String _defaultIdGen() =>
      DateTime.now().microsecondsSinceEpoch.toRadixString(36);

  final String Function() _idGen;

  List<AppEntry> _apps = const [];
  final Set<String> _pinned = {};
  List<AppGroup> _groups = [];
  List<String> _order = [];
  String _query = '';
  LibrarySort _sort = LibrarySort.nameAsc;
  LibraryFilter _filter = const FilterAll();

  List<AppEntry> get apps => _apps;
  List<AppGroup> get groups => List.unmodifiable(_groups);
  Set<String> get pinned => Set.unmodifiable(_pinned);
  String get query => _query;
  LibrarySort get sort => _sort;
  LibraryFilter get filter => _filter;

  bool isPinned(String packageName) => _pinned.contains(packageName);

  AppGroup? groupById(String id) {
    for (final g in _groups) {
      if (g.id == id) return g;
    }
    return null;
  }

  /// Apps matching the active filter and query, arranged per the active
  /// sort. Pinned apps float to the top outside of group views.
  List<AppEntry> get visible {
    var base = _baseFor(_filter);
    final q = normalizeForSearch(_query.trim());
    if (q.isNotEmpty) {
      base = base
          .where((a) =>
              normalizeForSearch(a.label).contains(q) ||
              a.packageName.toLowerCase().contains(q))
          .toList();
    }
    final sorted = _applySort(base);
    if (_filter is FilterGroup) return sorted;
    return _pinFirst(sorted);
  }

  /// Count without the search query - feeds the "All (N)" style labels.
  int countFor(LibraryFilter f) => _baseFor(f).length;

  List<AppEntry> _baseFor(LibraryFilter f) {
    switch (f) {
      case FilterAll():
        return List.of(_apps);
      case FilterPinned():
        return _apps.where((a) => _pinned.contains(a.packageName)).toList();
      case FilterUserApps():
        return _apps.where((a) => !a.isSystem).toList();
      case FilterSystemApps():
        return _apps.where((a) => a.isSystem).toList();
      case FilterGroup(groupId: final id):
        final g = groupById(id);
        if (g == null) return const [];
        final byPkg = {for (final a in _apps) a.packageName: a};
        return [
          for (final p in g.members)
            if (byPkg[p] != null) byPkg[p]!,
        ];
    }
  }

  List<AppEntry> _applySort(List<AppEntry> list) {
    switch (_sort) {
      case LibrarySort.custom:
        final positions = _positionsFor(_filter);
        final copy = List.of(list);
        copy.sort((a, b) {
          final ai = positions[a.packageName] ?? 1 << 30;
          final bi = positions[b.packageName] ?? 1 << 30;
          return ai.compareTo(bi);
        });
        return copy;
      case LibrarySort.nameAsc:
        return _byName(list, false);
      case LibrarySort.nameDesc:
        return _byName(list, true);
      case LibrarySort.newestFirst:
        final copy = List.of(list);
        copy.sort((a, b) => b.firstInstallTime.compareTo(a.firstInstallTime));
        return copy;
      case LibrarySort.recentlyUpdated:
        final copy = List.of(list);
        copy.sort((a, b) => b.lastUpdateTime.compareTo(a.lastUpdateTime));
        return copy;
    }
  }

  Map<String, int> _positionsFor(LibraryFilter f) {
    final src = f is FilterGroup
        ? (groupById(f.groupId)?.members ?? const <String>[])
        : _order;
    return {for (var i = 0; i < src.length; i++) src[i]: i};
  }

  List<AppEntry> _byName(List<AppEntry> list, bool desc) {
    final copy = List.of(list);
    copy.sort((a, b) {
      final c = normalizeForSearch(a.label)
          .compareTo(normalizeForSearch(b.label));
      if (c != 0) return desc ? -c : c;
      final p = a.packageName.compareTo(b.packageName);
      return desc ? -p : p;
    });
    return copy;
  }

  List<AppEntry> _pinFirst(List<AppEntry> list) {
    if (list.length < 2) return list;
    final pinnedApps = <AppEntry>[];
    final rest = <AppEntry>[];
    for (final a in list) {
      (_pinned.contains(a.packageName) ? pinnedApps : rest).add(a);
    }
    return [...pinnedApps, ...rest];
  }

  /// Replaces the catalog, pruning pins, order and group members that no
  /// longer resolve, and appending newly seen packages to the manual order.
  void setApps(List<AppEntry> apps) {
    _apps = List.unmodifiable(apps);
    final pkgs = apps.map((a) => a.packageName).toSet();
    _pinned.removeWhere((p) => !pkgs.contains(p));
    final known = _order.where(pkgs.contains).toList();
    final knownSet = known.toSet();
    final fresh = apps
        .where((a) => !knownSet.contains(a.packageName))
        .map((a) => a.packageName)
        .toList();
    _order = [...known, ...fresh];
    _groups = [
      for (final g in _groups)
        g.copyWith(members: g.members.where(pkgs.contains).toList()),
    ];
    final f = _filter;
    if (f is FilterGroup && groupById(f.groupId) == null) {
      _filter = const FilterAll();
    }
    notifyListeners();
  }

  void setQuery(String query) {
    if (query == _query) return;
    _query = query;
    notifyListeners();
  }

  void setSort(LibrarySort sort) {
    if (sort == _sort) return;
    _sort = sort;
    notifyListeners();
  }

  void setFilter(LibraryFilter filter) {
    if (filter == _filter) return;
    _filter = filter;
    notifyListeners();
  }

  void togglePin(String packageName) {
    if (!_pinned.add(packageName)) {
      _pinned.remove(packageName);
    }
    notifyListeners();
  }

  /// Moves [packageName] to [index] inside the current visible list and
  /// writes the arrangement back to the backing order (manual order for
  /// flat views, member list inside a group). Manual moves switch the sort
  /// to [LibrarySort.custom] so the arrangement stays put.
  void moveApp(String packageName, int index) {
    final vis = visible;
    final from = vis.indexWhere((a) => a.packageName == packageName);
    if (from < 0) return;
    final to = index.clamp(0, vis.length - 1);
    if (from == to) return;
    final moved = vis.removeAt(from);
    vis.insert(to, moved);
    final visPkgs = vis.map((a) => a.packageName).toList();
    final f = _filter;
    if (f is FilterGroup) {
      final g = groupById(f.groupId);
      if (g == null) return;
      _replaceOrder(g, visPkgs);
    } else {
      _order = _mergedOrder(_order, visPkgs);
    }
    _sort = LibrarySort.custom;
    notifyListeners();
  }

  /// Splices the reordered visible package names back into [backing] at the
  /// slots they occupied, so items hidden by a query keep their positions.
  List<String> _mergedOrder(List<String> backing, List<String> visPkgs) {
    final visSet = visPkgs.toSet();
    final it = visPkgs.iterator;
    final out = <String>[];
    for (final p in backing) {
      if (visSet.contains(p)) {
        it.moveNext();
        out.add(it.current);
      } else {
        out.add(p);
      }
    }
    return out;
  }

  void _replaceOrder(AppGroup group, List<String> newMembers) {
    final visibleSet = newMembers.toSet();
    final it = newMembers.iterator;
    final merged = <String>[];
    for (final p in group.members) {
      if (visibleSet.contains(p)) {
        it.moveNext();
        merged.add(it.current);
      } else {
        merged.add(p);
      }
    }
    _groups = [
      for (final g in _groups)
        g.id == group.id ? g.copyWith(members: merged) : g,
    ];
  }

  String createGroup(String name) {
    final id = _idGen();
    _groups = [..._groups, AppGroup(id: id, name: name)];
    notifyListeners();
    return id;
  }

  void renameGroup(String id, String name) {
    _groups = [
      for (final g in _groups) g.id == id ? g.copyWith(name: name) : g,
    ];
    notifyListeners();
  }

  void deleteGroup(String id) {
    _groups = _groups.where((g) => g.id != id).toList();
    final f = _filter;
    if (f is FilterGroup && f.groupId == id) {
      _filter = const FilterAll();
    }
    notifyListeners();
  }

  void addToGroup(String id, String packageName) {
    final g = groupById(id);
    if (g == null || g.members.contains(packageName)) return;
    _groups = [
      for (final x in _groups)
        x.id == id ? x.copyWith(members: [...x.members, packageName]) : x,
    ];
    notifyListeners();
  }

  void removeFromGroup(String id, String packageName) {
    _groups = [
      for (final x in _groups)
        x.id == id
            ? x.copyWith(
                members: x.members.where((m) => m != packageName).toList())
            : x,
    ];
    notifyListeners();
  }

  LibrarySnapshot snapshot() => LibrarySnapshot(
        pinned: _pinned.toList(),
        order: List.of(_order),
        groups: List.of(_groups),
      );

  void restore(LibrarySnapshot snap) {
    _pinned
      ..clear()
      ..addAll(snap.pinned);
    _order = List.of(snap.order);
    _groups = List.of(snap.groups);
    // Reconcile against whatever is loaded so stale entries cannot linger.
    setApps(_apps);
  }
}
