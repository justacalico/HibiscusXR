import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_quicksettings/src/persistence.dart';
import 'package:pn2_quicksettings/src/platform/prefs_persistence.dart';
import 'package:shared_preferences/shared_preferences.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  test('memory persistence roundtrips', () async {
    final p = MemoryPersistence();
    expect(await p.load(), isNull);
    await p.save({
      'toggles': {'wifi': true},
    });
    expect(p.saves, 1);
    expect(await p.load(), {
      'toggles': {'wifi': true},
    });
  });

  test('prefs persistence survives a reload', () async {
    SharedPreferences.setMockInitialValues({});
    final p = await PrefsPersistence.open();
    await p.save({
      'version': 1,
      'toggles': {'nightMode': true},
    });
    final again = await PrefsPersistence.open();
    expect(await again.load(), {
      'version': 1,
      'toggles': {'nightMode': true},
    });
  });

  test('prefs persistence returns null on bad payloads', () async {
    SharedPreferences.setMockInitialValues({
      'quick_settings_snapshot': '"a string, not a map"',
    });
    final p = await PrefsPersistence.open();
    expect(await p.load(), isNull);
  });
}
