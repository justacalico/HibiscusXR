import 'package:flutter/material.dart';

import 'theme.dart';

/// Status summary for one controller slot on the scan card.
class ScanSlot {
  const ScanSlot({required this.name, required this.state});

  final String name;
  final String state;
}

/// Full-width card that doubles as the scan button: tap to start or
/// stop the station scan. While a scan runs the indicator spins and
/// each slot chip shows its live link state.
class ScanCard extends StatelessWidget {
  const ScanCard({
    super.key,
    required this.title,
    required this.status,
    required this.scanning,
    required this.slots,
    required this.onTap,
  });

  final String title;
  final String status;
  final bool scanning;
  final List<ScanSlot> slots;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
      child: Material(
        color: PanelTheme.surface,
        borderRadius: BorderRadius.circular(16),
        child: InkWell(
          borderRadius: BorderRadius.circular(16),
          onTap: onTap,
          child: Padding(
            padding: const EdgeInsets.all(20),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    SizedBox(
                      width: 26,
                      height: 26,
                      child: scanning
                          ? const CircularProgressIndicator(
                              strokeWidth: 2.4,
                              color: PanelTheme.accent,
                            )
                          : const Icon(
                              Icons.bluetooth_searching,
                              size: 24,
                              color: PanelTheme.accent,
                            ),
                    ),
                    const SizedBox(width: 14),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            title,
                            style: const TextStyle(
                              fontSize: 17,
                              color: PanelTheme.textPrimary,
                            ),
                          ),
                          const SizedBox(height: 3),
                          Text(
                            status,
                            style: TextStyle(
                              fontSize: 12,
                              color: scanning
                                  ? PanelTheme.accent
                                  : PanelTheme.textSecondary,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 14),
                Row(
                  children: [
                    for (var i = 0; i < slots.length; i++) ...[
                      if (i > 0) const SizedBox(width: 10),
                      Flexible(child: _SlotChip(slot: slots[i])),
                    ],
                  ],
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _SlotChip extends StatelessWidget {
  const _SlotChip({required this.slot});

  final ScanSlot slot;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 7),
      decoration: BoxDecoration(
        color: PanelTheme.surfaceHigh,
        borderRadius: BorderRadius.circular(10),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Flexible(
            child: Text(
              slot.name,
              overflow: TextOverflow.ellipsis,
              style: const TextStyle(
                fontSize: 13,
                color: PanelTheme.textSecondary,
              ),
            ),
          ),
          const SizedBox(width: 8),
          Flexible(
            child: Text(
              slot.state,
              overflow: TextOverflow.ellipsis,
              style: const TextStyle(
                fontSize: 13,
                fontWeight: FontWeight.w600,
                color: PanelTheme.textPrimary,
              ),
            ),
          ),
        ],
      ),
    );
  }
}
