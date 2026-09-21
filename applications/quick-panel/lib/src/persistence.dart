/// Where the persisted toggle map lives. The platform layer supplies the
/// real implementation; tests use [MemoryPersistence].
abstract class SettingsPersistence {
  Future<Map<String, dynamic>?> load();
  Future<void> save(Map<String, dynamic> snapshot);
}

class MemoryPersistence implements SettingsPersistence {
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
