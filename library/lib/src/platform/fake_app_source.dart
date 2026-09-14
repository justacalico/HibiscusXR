import 'dart:async';
import 'dart:typed_data';

import '../models.dart';
import 'app_source.dart';

/// In-memory source for tests and for running the UI without a device.
/// The tile grid falls back to letter icons when [icon] returns null.
class FakeAppSource implements AppSource {
  FakeAppSource({List<AppEntry>? apps, this.icons = const {}})
      : apps = apps ?? _demoApps;

  List<AppEntry> apps;

  /// Optional PNG bytes per package for tests that exercise real icons.
  final Map<String, Uint8List> icons;

  int iconCalls = 0;
  int listCalls = 0;
  final List<String> launched = [];
  final List<String> uninstalled = [];
  final List<String> infoOpened = [];
  var installPicked = false;
  var failNextList = false;

  final _changes = StreamController<void>.broadcast();

  @override
  Stream<void> get changes => _changes.stream;

  /// Pushes a package-change event, like a real install/uninstall would.
  void emitChange() => _changes.add(null);

  void dispose() => _changes.close();

  @override
  Future<List<AppEntry>> listApps() async {
    listCalls++;
    if (failNextList) {
      failNextList = false;
      throw StateError('listApps failed');
    }
    return List.of(apps);
  }

  @override
  Future<Uint8List?> icon(String packageName) async {
    iconCalls++;
    return icons[packageName];
  }

  @override
  Future<bool> launch(AppEntry app) async {
    launched.add(app.packageName);
    return true;
  }

  @override
  Future<bool> uninstall(AppEntry app) async {
    uninstalled.add(app.packageName);
    apps = apps.where((a) => a.packageName != app.packageName).toList();
    emitChange();
    return true;
  }

  @override
  Future<bool> openAppInfo(AppEntry app) async {
    infoOpened.add(app.packageName);
    return true;
  }

  @override
  Future<bool> pickAndInstallApk() async {
    if (!installPicked) return false;
    apps = [
      ...apps,
      AppEntry(
        packageName: 'dev.sideloaded.${apps.length}',
        label: 'Sideloaded ${apps.length}',
        firstInstallTime: DateTime.now().millisecondsSinceEpoch,
      ),
    ];
    emitChange();
    return true;
  }

  static final List<AppEntry> _demoApps = [
    const AppEntry(
        packageName: 'com.vr.home', label: 'VR Home', firstInstallTime: 100),
    const AppEntry(
        packageName: 'com.vr.player', label: 'Video Player', firstInstallTime: 90),
    const AppEntry(
        packageName: 'com.vr.browser', label: 'Browser', firstInstallTime: 80),
    const AppEntry(
        packageName: 'com.android.settings',
        label: 'Settings',
        isSystem: true,
        firstInstallTime: 1),
  ];
}
