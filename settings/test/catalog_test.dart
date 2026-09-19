import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_settings/src/catalog.dart';
import 'package:pn2_settings/src/models.dart';

void main() {
  test('every section has a def in display order', () {
    expect(kSections.map((s) => s.id), SectionId.values);
  });

  test('every item has a kind', () {
    for (final id in ItemId.values) {
      expect(kItemKinds.containsKey(id), isTrue, reason: '$id missing');
    }
  });

  test('every item lives in exactly one section', () {
    final placed = [for (final s in kSections) ...s.items];
    for (final id in ItemId.values) {
      expect(placed, contains(id), reason: '$id not placed');
    }
    expect(placed.toSet(), hasLength(placed.length));
  });

  test('choice rows declare options', () {
    for (final e in kItemKinds.entries) {
      if (e.value == ItemKind.choice) {
        expect(optionsOf(e.key), isNotEmpty, reason: '${e.key} has none');
      }
    }
  });

  test('sectionDef returns the matching def', () {
    for (final s in kSections) {
      expect(sectionDef(s.id), same(s));
    }
  });

  test('kindOf falls back to info for unknown ids', () {
    expect(kindOf(ItemId.wifiToggle), ItemKind.toggle);
    expect(kindOf(ItemId.tipsBody), ItemKind.info);
  });
}
