/// Where the repo address and friends persist. The platform layer
/// supplies the SharedPreferences-backed version; tests use
/// [MemoryPersistence].
abstract class StorePersistence {
  Future<Map<String, dynamic>?> load();
  Future<void> save(Map<String, dynamic> snapshot);
}

class MemoryPersistence implements StorePersistence {
  MemoryPersistence([this.stored]);

  Map<String, dynamic>? stored;
  var saves = 0;

  @override
  Future<Map<String, dynamic>?> load() async => stored;

  @override
  Future<void> save(Map<String, dynamic> snapshot) async {
    saves++;
    stored = snapshot;
  }
}
