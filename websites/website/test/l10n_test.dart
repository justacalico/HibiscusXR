import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';

void main() {
  test('zh covers every en message key', () {
    final en = jsonDecode(
      File('lib/l10n/app_en.arb').readAsStringSync(),
    ) as Map<String, dynamic>;
    final zh = jsonDecode(
      File('lib/l10n/app_zh.arb').readAsStringSync(),
    ) as Map<String, dynamic>;

    final enKeys =
        en.keys.where((k) => !k.startsWith('@') && k != '@@locale').toSet();
    final zhKeys =
        zh.keys.where((k) => !k.startsWith('@') && k != '@@locale').toSet();

    expect(zhKeys, containsAll(enKeys),
        reason: 'zh is missing: ${enKeys.difference(zhKeys)}');
  });
}
