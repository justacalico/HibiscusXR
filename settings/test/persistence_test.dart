import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/src/persistence.dart';

void main() {
  test('memory persistence stores and counts', () async {
    final p = MemoryPersistence();
    expect(await p.load(), isNull);
    await p.save({'a': 1});
    await p.save({'a': 2});
    expect(p.saves, 2);
    expect(await p.load(), {'a': 2});
  });

  test('memory persistence seeds from constructor', () async {
    final p = MemoryPersistence({'x': true});
    expect(await p.load(), {'x': true});
  });
}
