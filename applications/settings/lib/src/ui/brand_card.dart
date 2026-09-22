import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';

import 'theme.dart';

/// Header card for the About section: the project mark and name.
class BrandCard extends StatelessWidget {
  const BrandCard({super.key, required this.name, required this.caption});

  final String name;
  final String caption;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
      child: Material(
        color: PanelTheme.surface,
        borderRadius: BorderRadius.circular(16),
        child: Padding(
          padding: const EdgeInsets.symmetric(vertical: 26),
          child: Column(
            children: [
              SvgPicture.asset(
                'assets/hibiscusxr_icon.svg',
                width: 96,
                height: 96,
              ),
              const SizedBox(height: 14),
              Text(
                name,
                style: const TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.w600,
                  color: PanelTheme.textPrimary,
                ),
              ),
              const SizedBox(height: 3),
              Text(
                caption,
                style: const TextStyle(
                  fontSize: 12,
                  color: PanelTheme.textSecondary,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
