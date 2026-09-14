import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_library/src/models.dart';
import 'package:pn2_library/src/platform/android_app_source.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  const appsChannel = MethodChannel('gitlab.neosalsa.library/apps');
  const changesChannel = MethodChannel('gitlab.neosalsa.library/changes');
  final messenger =
      TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger;

  final calls = <MethodCall>[];

  setUp(() {
    calls.clear();
    messenger.setMockMethodCallHandler(appsChannel, (call) async {
      calls.add(call);
      switch (call.method) {
        case 'getApps':
          return [
            {
              'packageName': 'com.a',
              'label': 'Alpha',
              'activityName': 'com.a.Main',
              'isSystem': true,
              'versionName': '1.0',
              'firstInstallTime': 42,
              'lastUpdateTime': 43,
            },
            {
              'packageName': 'com.b',
              'label': 'Beta',
              // optional fields missing on purpose
            },
          ];
        case 'getIcon':
          return Uint8List.fromList(const [1, 2, 3]);
        case 'launch':
        case 'uninstall':
        case 'openAppInfo':
        case 'pickAndInstallApk':
          return true;
      }
      return null;
    });
    messenger.setMockMethodCallHandler(changesChannel, (call) async => null);
  });

  tearDown(() {
    messenger.setMockMethodCallHandler(appsChannel, null);
    messenger.setMockMethodCallHandler(changesChannel, null);
  });

  test('listApps parses the platform maps', () async {
    final src = AndroidAppSource();
    final apps = await src.listApps();
    expect(apps, hasLength(2));
    final a = apps[0];
    expect(a.packageName, 'com.a');
    expect(a.label, 'Alpha');
    expect(a.activityName, 'com.a.Main');
    expect(a.isSystem, isTrue);
    expect(a.versionName, '1.0');
    expect(a.firstInstallTime, 42);
    final b = apps[1];
    expect(b.isSystem, isFalse);
    expect(b.versionName, isNull);
  });

  test('actions send the package argument', () async {
    final src = AndroidAppSource();
    const entry = AppEntry(packageName: 'com.a', label: 'Alpha');
    await src.launch(entry);
    await src.uninstall(entry);
    await src.openAppInfo(entry);
    await src.pickAndInstallApk();
    expect(calls.map((c) => c.method), [
      'launch',
      'uninstall',
      'openAppInfo',
      'pickAndInstallApk',
    ]);
    expect(calls[0].arguments, {'package': 'com.a'});
  });

  test('getIcon returns bytes', () async {
    final src = AndroidAppSource();
    expect(await src.icon('com.a'), [1, 2, 3]);
  });

  test('changes stream subscribes the event channel', () async {
    var listened = false;
    messenger.setMockMethodCallHandler(changesChannel, (call) async {
      if (call.method == 'listen') listened = true;
      return null;
    });
    final src = AndroidAppSource();
    final sub = src.changes.listen((_) {});
    addTearDown(sub.cancel);
    await Future<void>.delayed(Duration.zero);
    expect(listened, isTrue);
  });
}
