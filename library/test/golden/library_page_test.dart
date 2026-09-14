import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:pn2_library/main.dart';
import 'package:pn2_library/src/library_controller.dart';
import 'package:pn2_library/src/models.dart';
import 'package:pn2_library/src/persistence.dart';
import 'package:pn2_library/src/platform/fake_app_source.dart';

import '../png.dart';

void main() {
  testWidgets('library page golden', (tester) async {
    tester.view.physicalSize = const Size(1440, 900);
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    final icons = <String, Uint8List>{
      'com.vr.home': solidPng(96, 0xFF4E9CFF),
      'com.vr.player': solidPng(96, 0xFFFF5E7E),
      'com.vr.browser': solidPng(96, 0xFF2ED3A6),
      'com.android.settings': solidPng(96, 0xFF9AA7B4),
    };
    final apps = [
      const AppEntry(
          packageName: 'com.vr.home', label: 'VR Home', firstInstallTime: 100),
      const AppEntry(
          packageName: 'com.vr.player',
          label: 'Video Player',
          firstInstallTime: 90),
      const AppEntry(
          packageName: 'com.vr.browser',
          label: 'Browser',
          firstInstallTime: 80),
      const AppEntry(
          packageName: 'com.android.settings',
          label: 'Settings',
          isSystem: true,
          firstInstallTime: 1),
      const AppEntry(
          packageName: 'com.vr.store', label: 'Store', firstInstallTime: 70),
      const AppEntry(
          packageName: 'com.vr.files', label: 'Files', firstInstallTime: 60),
      const AppEntry(
          packageName: 'com.vr.photos', label: 'Photos', firstInstallTime: 50),
      const AppEntry(
          packageName: 'com.vr.music', label: 'Music', firstInstallTime: 40),
    ];
    final src = FakeAppSource(apps: apps, icons: icons);
    addTearDown(src.dispose);
    final c =
        LibraryController(source: src, persistence: MemoryPersistence());
    addTearDown(c.dispose);

    await tester.pumpWidget(LibraryApp(controller: c));
    // pumpAndSettle never settles while icons decode async; pump a fixed
    // number of frames instead so the golden stays deterministic.
    for (var i = 0; i < 30; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }
    c.store.togglePin('com.vr.browser');
    for (var i = 0; i < 10; i++) {
      await tester.pump(const Duration(milliseconds: 50));
    }

    await expectLater(
      find.byType(MaterialApp),
      matchesGoldenFile('goldens/library_page.png'),
    );
  });
}
