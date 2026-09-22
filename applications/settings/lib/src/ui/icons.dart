import 'package:flutter/material.dart';

import '../models.dart';

/// Sidebar glyph per section. Lives in the UI layer so the pure modules
/// never touch IconData.
IconData iconFor(SectionId id) {
  switch (id) {
    case SectionId.wifi:
      return Icons.wifi;
    case SectionId.bluetooth:
      return Icons.bluetooth;
    case SectionId.controllers:
      return Icons.sports_esports_outlined;
    case SectionId.display:
      return Icons.brightness_6_outlined;
    case SectionId.sound:
      return Icons.volume_up_outlined;
    case SectionId.language:
      return Icons.language;
    case SectionId.time:
      return Icons.schedule;
    case SectionId.keyboard:
      return Icons.keyboard_outlined;
    case SectionId.developer:
      return Icons.developer_mode;
    case SectionId.about:
      return Icons.info_outline;
  }
}
