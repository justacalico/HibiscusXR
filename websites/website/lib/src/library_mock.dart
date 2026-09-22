import 'package:flutter/material.dart';

import 'package:hibiscusxr_website/l10n/app_localizations.dart';

import 'theme.dart';

/// Glyph painted inside a tile's white disc.
enum LibraryMark {
  calendar,
  camera,
  chrome,
  clock,
  contacts,
  drive,
  files,
  gemini,
  glasses,
}

/// One app tile in the hero's fake library window.
class LibraryTile {
  const LibraryTile({
    required this.name,
    required this.mark,
    required this.tint,
    this.pinned = false,
  });

  final String name;
  final LibraryMark mark;
  final Color tint;
  final bool pinned;
}

/// Total library size shown on the filter chip. The grid only shows nine.
const libraryTotal = 21;

/// The nine tiles shown in the hero mock, in row-major order.
List<LibraryTile> libraryTiles(AppLocalizations l10n) => [
      LibraryTile(
        name: l10n.shotAppCalendar,
        mark: LibraryMark.calendar,
        tint: AppColors.shotPine,
        pinned: true,
      ),
      LibraryTile(
        name: l10n.shotAppCamera,
        mark: LibraryMark.camera,
        tint: AppColors.shotBlue,
      ),
      LibraryTile(
        name: l10n.shotAppChrome,
        mark: LibraryMark.chrome,
        tint: AppColors.shotOlive,
      ),
      LibraryTile(
        name: l10n.shotAppClock,
        mark: LibraryMark.clock,
        tint: AppColors.shotBlue,
      ),
      LibraryTile(
        name: l10n.shotAppContacts,
        mark: LibraryMark.contacts,
        tint: AppColors.shotBlue,
      ),
      LibraryTile(
        name: l10n.shotAppDrive,
        mark: LibraryMark.drive,
        tint: AppColors.shotOlive,
      ),
      LibraryTile(
        name: l10n.shotAppFiles,
        mark: LibraryMark.files,
        tint: AppColors.shotBlue,
      ),
      LibraryTile(
        name: l10n.shotAppGemini,
        mark: LibraryMark.gemini,
        tint: AppColors.shotBlue,
      ),
      LibraryTile(
        name: l10n.shotAppGlasses,
        mark: LibraryMark.glasses,
        tint: AppColors.shotSlate,
      ),
    ];
