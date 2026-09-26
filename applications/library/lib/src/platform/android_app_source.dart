import 'package:flutter/services.dart';

import '../models.dart';
import 'app_source.dart';

/// MethodChannel/EventChannel glue to the Kotlin side in MainActivity.
/// Thin on purpose: arguments go in, parsed values come out.
class AndroidAppSource implements AppSource {
  AndroidAppSource({MethodChannel? apps, EventChannel? changes})
    : _apps = apps ?? const MethodChannel('gitlab.neosalsa.library/apps'),
      _changes =
          changes ?? const EventChannel('gitlab.neosalsa.library/changes');

  final MethodChannel _apps;
  final EventChannel _changes;

  // One platform subscription; package broadcasts and theme pushes both
  // land on it as maps, and each getter filters its own key out.
  Stream<dynamic>? _events;
  Stream<dynamic> get _eventFeed =>
      _events ??= _changes.receiveBroadcastStream();

  @override
  Future<List<AppEntry>> listApps() async {
    final raw = await _apps.invokeListMethod<Object?>('getApps');
    if (raw == null) return const [];
    return [
      for (final item in raw)
        if (item is Map) _parse(item),
    ];
  }

  AppEntry _parse(Map m) => AppEntry(
    packageName: '${m['packageName']}',
    label: '${m['label'] ?? m['packageName']}',
    activityName: m['activityName'] as String?,
    isSystem: m['isSystem'] == true,
    versionName: m['versionName'] as String?,
    firstInstallTime: (m['firstInstallTime'] as num?)?.toInt() ?? 0,
    lastUpdateTime: (m['lastUpdateTime'] as num?)?.toInt() ?? 0,
  );

  @override
  Future<Uint8List?> icon(String packageName) =>
      _apps.invokeMethod<Uint8List>('getIcon', {'package': packageName});

  @override
  Future<bool> launch(AppEntry app) => _bool('launch', app.packageName);

  @override
  Future<bool> uninstall(AppEntry app) => _bool('uninstall', app.packageName);

  @override
  Future<bool> openAppInfo(AppEntry app) =>
      _bool('openAppInfo', app.packageName);

  @override
  Future<bool> pickAndInstallApk() async =>
      await _apps.invokeMethod<bool>('pickAndInstallApk') ?? false;

  Future<bool> _bool(String method, String packageName) async =>
      await _apps.invokeMethod<bool>(method, {'package': packageName}) ?? false;

  @override
  Future<String> theme() async =>
      await _apps.invokeMethod<String>('getTheme') ?? 'dark';

  @override
  Stream<void> get changes => _eventFeed
      .where((e) => e is Map && e.containsKey('package'))
      .map((_) {});

  @override
  Stream<String> get themes => _eventFeed
      .where((e) => e is Map && e['theme'] is String)
      .map((e) => (e as Map)['theme'] as String);
}
