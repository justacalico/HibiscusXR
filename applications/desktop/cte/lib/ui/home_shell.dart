import 'package:flutter/material.dart';

import '../l10n/app_localizations.dart';
import '../src/app_state.dart';
import 'debug_page.dart';
import 'display_page.dart';
import 'install_page.dart';
import 'overview_page.dart';
import 'tracking_page.dart';

/// NavigationRail shell - the rail swaps pages, state lives above.
class HomeShell extends StatefulWidget {
  const HomeShell({super.key, required this.state});

  final AppState state;

  @override
  State<HomeShell> createState() => _HomeShellState();
}

class _HomeShellState extends State<HomeShell> {
  int _index = 0;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final pages = <Widget>[
      OverviewPage(state: widget.state),
      DisplayPage(state: widget.state),
      InstallPage(state: widget.state),
      TrackingPage(state: widget.state),
      DebugPage(state: widget.state),
    ];
    return Scaffold(
      body: Row(
        children: [
          NavigationRail(
            selectedIndex: _index,
            onDestinationSelected: (i) => setState(() => _index = i),
            labelType: NavigationRailLabelType.all,
            leading: Padding(
              padding: const EdgeInsets.symmetric(vertical: 12),
              child: Column(
                children: [
                  const Icon(Icons.developer_board, size: 28),
                  const SizedBox(height: 4),
                  Text(l10n.appTitle,
                      style: Theme.of(context).textTheme.labelSmall),
                ],
              ),
            ),
            trailing: Expanded(
              child: Align(
                alignment: Alignment.bottomCenter,
                child: Padding(
                  padding: const EdgeInsets.only(bottom: 12),
                  child: IconButton(
                    icon: const Icon(Icons.link_off),
                    tooltip: l10n.disconnect,
                    onPressed: widget.state.disconnect,
                  ),
                ),
              ),
            ),
            destinations: [
              NavigationRailDestination(
                  icon: const Icon(Icons.dashboard_outlined),
                  label: Text(l10n.navOverview)),
              NavigationRailDestination(
                  icon: const Icon(Icons.monitor_outlined),
                  label: Text(l10n.navDisplay)),
              NavigationRailDestination(
                  icon: const Icon(Icons.install_mobile_outlined),
                  label: Text(l10n.navInstall)),
              NavigationRailDestination(
                  icon: const Icon(Icons.threed_rotation),
                  label: Text(l10n.navTracking)),
              NavigationRailDestination(
                  icon: const Icon(Icons.bug_report_outlined),
                  label: Text(l10n.navDebug)),
            ],
          ),
          const VerticalDivider(width: 1),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                _TitleBar(state: widget.state),
                Expanded(child: pages[_index]),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _TitleBar extends StatelessWidget {
  const _TitleBar({required this.state});
  final AppState state;

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: state,
      builder: (context, _) => Container(
        padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 10),
        decoration: const BoxDecoration(
          border: Border(bottom: BorderSide(color: Color(0xFF2A2A33), width: 0.5)),
        ),
        child: Row(
          children: [
            Text(state.connectedLabel,
                style: Theme.of(context).textTheme.titleSmall),
            const Spacer(),
            if (state.deviceInfo != null)
              Text(AppLocalizations.of(context)
                  .trackingMode(state.deviceInfo!.trackingMode.name),
                  style: Theme.of(context)
                      .textTheme
                      .bodySmall
                      ?.copyWith(color: Colors.white54)),
          ],
        ),
      ),
    );
  }
}
