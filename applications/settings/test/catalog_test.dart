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

  test('sectionDef returns the matching def', () {
    for (final s in kSections) {
      expect(sectionDef(s.id), same(s));
    }
  });

  test('kindOf falls back to info for unknown ids', () {
    expect(kindOf(ItemId.wifiToggle), ItemKind.toggle);
    expect(kindOf(ItemId.modelName), ItemKind.info);
  });

  test('implementedOf marks only the stub rows', () {
    const stubs = {
      ItemId.nightMode,
      ItemId.adbToggle,
      ItemId.stayAwake,
      ItemId.showTouches,
    };
    for (final id in ItemId.values) {
      expect(implementedOf(id), !stubs.contains(id), reason: '$id');
    }
    // every stub is a real catalog row, not a stray id
    expect(kUnimplemented, stubs);
  });

  test('requiresRebootOf marks only device mode', () {
    for (final id in ItemId.values) {
      expect(
        requiresRebootOf(id),
        id == ItemId.deviceMode,
        reason: '$id',
      );
    }
    // reboot-gated rows are still real implemented rows
    expect(kRequiresReboot, {ItemId.deviceMode});
    for (final id in kRequiresReboot) {
      expect(implementedOf(id), isTrue, reason: '$id');
    }
  });

  test('languageOptionFor maps dialects and rejects other locales', () {
    expect(languageOptionFor('en'), 'en');
    expect(languageOptionFor('en-US'), 'en');
    expect(languageOptionFor('zh'), 'zh-CN');
    expect(languageOptionFor('zh-CN'), 'zh-CN');
    expect(languageOptionFor('zh-Hans-SG'), 'zh-CN');
    expect(languageOptionFor('zh-MY'), 'zh-CN');
    // Traditional Chinese is not the Simplified option
    expect(languageOptionFor('zh-TW'), isNull);
    expect(languageOptionFor('zh-Hant'), isNull);
    expect(languageOptionFor('fr-FR'), isNull);
    expect(languageOptionFor(''), isNull);
    expect(languageOptionFor(null), isNull);
  });

  test('language options stay in sync with the generated locales', () {
    for (final tag in kLanguageOptions) {
      expect(languageOptionFor(tag), tag, reason: tag);
    }
  });
}
