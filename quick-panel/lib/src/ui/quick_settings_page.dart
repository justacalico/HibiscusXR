import 'dart:async';

import 'package:flutter/material.dart';

import '../../l10n/app_localizations.dart';
import '../catalog.dart';
import '../models.dart';
import '../settings_controller.dart';
import 'notifications.dart';
import 'panel_slider.dart';
import 'status_bar.dart';
import 'theme.dart';
import 'tiles.dart';

/// The whole quick settings window: status bar, two sliders, large
/// radio tiles, small tiles and the footer, on a rounded dark card.
class QuickSettingsPage extends StatefulWidget {
  const QuickSettingsPage({super.key, required this.controller});

  final SettingsController controller;

  @override
  State<QuickSettingsPage> createState() => _QuickSettingsPageState();
}

class _QuickSettingsPageState extends State<QuickSettingsPage> {
  Timer? _clockTimer;

  SettingsController get controller => widget.controller;

  @override
  void initState() {
    super.initState();
    // Keep the status bar clock fresh; the store notifies listeners.
    _clockTimer = Timer.periodic(const Duration(seconds: 30), (_) {
      controller.store.tick();
    });
  }

  @override
  void dispose() {
    _clockTimer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Scaffold(
      backgroundColor: PanelTheme.background,
      body: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 880),
          child: ListenableBuilder(
            listenable: controller.store,
            builder: (context, _) {
              return Container(
                margin: const EdgeInsets.all(14),
                clipBehavior: Clip.antiAlias,
                decoration: BoxDecoration(
                  color: PanelTheme.panel,
                  borderRadius: BorderRadius.circular(PanelTheme.panelRadius),
                ),
                child: SingleChildScrollView(
                  padding: const EdgeInsets.fromLTRB(20, 12, 20, 10),
                  child: FocusTraversalGroup(
                    policy: ReadingOrderTraversalPolicy(),
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                    children: [
                      PanelStatusBar(
                        batteryLevel: controller.store.batteryLevel,
                        now: controller.store.now,
                        onSettings: () =>
                            controller.runAction(ActionId.openSettings),
                      ),
                      const SizedBox(height: 10),
                      PanelSlider(
                        value: controller.store.volume,
                        icon: Icons.volume_up,
                        label: l10n.sliderVolume,
                        onChanged: controller.setVolume,
                      ),
                      const SizedBox(height: 10),
                      PanelSlider(
                        value: controller.store.brightness,
                        icon: Icons.light_mode,
                        label: l10n.sliderBrightness,
                        onChanged: controller.setBrightness,
                      ),
                      const SizedBox(height: 12),
                      Row(
                        children: [
                          for (var i = 0; i < largeTiles.length; i++) ...[
                            if (i > 0) const SizedBox(width: 12),
                            Expanded(
                              child: AspectRatio(
                                aspectRatio: 1.85,
                                child: SettingTile(
                                  spec: largeTiles[i],
                                  store: controller.store,
                                  autofocus: i == 0,
                                  onTap: () =>
                                      _onTileTap(largeTiles[i]),
                                ),
                              ),
                            ),
                          ],
                        ],
                      ),
                      const SizedBox(height: 10),
                      GridView.count(
                        crossAxisCount: 6,
                        mainAxisSpacing: 10,
                        crossAxisSpacing: 10,
                        childAspectRatio: 1.5,
                        shrinkWrap: true,
                        physics: const NeverScrollableScrollPhysics(),
                        children: [
                          for (final spec in smallTiles)
                            SettingTile(
                              spec: spec,
                              store: controller.store,
                              onTap: () => _onTileTap(spec),
                            ),
                        ],
                      ),
                      const SizedBox(height: 12),
                      NotificationSection(
                        notifications: controller.store.notifications,
                        onDismiss: controller.dismissNotification,
                        onDismissAll: controller.dismissAllNotifications,
                      ),
                    ],
                  ),
                ),
                ),
              );
            },
          ),
        ),
      ),
    );
  }

  void _onTileTap(TileSpec spec) {
    switch (spec.kind) {
      case TileKind.toggle:
        controller.toggleTile(spec.toggleId!);
      case TileKind.action:
        controller.runAction(spec.actionId!);
    }
  }
}
