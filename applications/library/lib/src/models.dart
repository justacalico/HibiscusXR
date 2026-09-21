/// One launchable app as reported by the platform bridge.
class AppEntry {
  const AppEntry({
    required this.packageName,
    required this.label,
    this.activityName,
    this.isSystem = false,
    this.versionName,
    this.firstInstallTime = 0,
    this.lastUpdateTime = 0,
  });

  final String packageName;
  final String label;
  final String? activityName;
  final bool isSystem;
  final String? versionName;
  final int firstInstallTime;
  final int lastUpdateTime;

  AppEntry copyWith({
    String? packageName,
    String? label,
    String? activityName,
    bool? isSystem,
    String? versionName,
    int? firstInstallTime,
    int? lastUpdateTime,
  }) {
    return AppEntry(
      packageName: packageName ?? this.packageName,
      label: label ?? this.label,
      activityName: activityName ?? this.activityName,
      isSystem: isSystem ?? this.isSystem,
      versionName: versionName ?? this.versionName,
      firstInstallTime: firstInstallTime ?? this.firstInstallTime,
      lastUpdateTime: lastUpdateTime ?? this.lastUpdateTime,
    );
  }

  @override
  bool operator ==(Object other) =>
      other is AppEntry &&
      other.packageName == packageName &&
      other.label == label &&
      other.activityName == activityName &&
      other.isSystem == isSystem &&
      other.versionName == versionName &&
      other.firstInstallTime == firstInstallTime &&
      other.lastUpdateTime == lastUpdateTime;

  @override
  int get hashCode => Object.hash(
    packageName,
    label,
    activityName,
    isSystem,
    versionName,
    firstInstallTime,
    lastUpdateTime,
  );
}

/// A named, ordered collection of package names.
class AppGroup {
  const AppGroup({
    required this.id,
    required this.name,
    this.members = const [],
  });

  final String id;
  final String name;
  final List<String> members;

  AppGroup copyWith({String? name, List<String>? members}) {
    return AppGroup(
      id: id,
      name: name ?? this.name,
      members: members ?? this.members,
    );
  }

  @override
  bool operator ==(Object other) =>
      other is AppGroup &&
      other.id == id &&
      other.name == name &&
      _listEquals(other.members, members);

  @override
  int get hashCode => Object.hash(id, name, Object.hashAll(members));

  static bool _listEquals(List<String> a, List<String> b) {
    if (a.length != b.length) return false;
    for (var i = 0; i < a.length; i++) {
      if (a[i] != b[i]) return false;
    }
    return true;
  }
}
