import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_library/src/text_norm.dart';

void main() {
  test('lowercases ascii', () {
    expect(normalizeForSearch('CAMERA'), 'camera');
  });

  test('folds latin-1 accents', () {
    expect(normalizeForSearch('Pokémon'), 'pokemon');
    expect(normalizeForSearch('Café'), 'cafe');
    expect(normalizeForSearch('Ábaco'), 'abaco');
  });

  test('folds extended latin', () {
    expect(normalizeForSearch('Łódź'), 'lodz');
    expect(normalizeForSearch('Škoda'), 'skoda');
  });

  test('expands ligatures and eszett', () {
    expect(normalizeForSearch('Æsir'), 'aesir');
    expect(normalizeForSearch('Straße'), 'strasse');
    expect(normalizeForSearch('Cœur'), 'coeur');
  });

  test('leaves unrelated scripts alone', () {
    expect(normalizeForSearch('相机'), '相机');
    expect(normalizeForSearch('Меню'), 'меню');
  });
}
