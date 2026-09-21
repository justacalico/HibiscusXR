import 'models.dart';

/// Serializable form of everything the store owns: pins, manual order and
/// groups. Versioned so future migrations have a hook.
class LibrarySnapshot {
  const LibrarySnapshot({
    this.version = 1,
    this.pinned = const [],
    this.order = const [],
    this.groups = const [],
  });

  final int version;
  final List<String> pinned;
  final List<String> order;
  final List<AppGroup> groups;

  Map<String, dynamic> toJson() => {
    'version': version,
    'pinned': pinned,
    'order': order,
    'groups': [
      for (final g in groups)
        {'id': g.id, 'name': g.name, 'members': g.members},
    ],
  };

  static LibrarySnapshot fromJson(Map<String, dynamic> json) {
    final rawGroups = json['groups'];
    return LibrarySnapshot(
      version: (json['version'] as num?)?.toInt() ?? 1,
      pinned: _strList(json['pinned']),
      order: _strList(json['order']),
      groups: rawGroups is List
          ? [
              for (final g in rawGroups)
                if (g is Map)
                  AppGroup(
                    id: '${g['id']}',
                    name: '${g['name']}',
                    members: _strList(g['members']),
                  ),
            ]
          : const [],
    );
  }

  static List<String> _strList(Object? v) =>
      v is List ? v.map((e) => '$e').toList() : const [];
}

/// Where the snapshot lives. The platform layer supplies the real
/// implementation; tests use [MemoryPersistence].
abstract class LibraryPersistence {
  Future<LibrarySnapshot?> load();
  Future<void> save(LibrarySnapshot snapshot);
}

class MemoryPersistence implements LibraryPersistence {
  MemoryPersistence([this.stored]);

  LibrarySnapshot? stored;
  var saves = 0;

  @override
  Future<LibrarySnapshot?> load() async => stored;

  @override
  Future<void> save(LibrarySnapshot snapshot) async {
    saves++;
    stored = snapshot;
  }
}
