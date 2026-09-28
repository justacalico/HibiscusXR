import 'package:flutter/material.dart';

import '../l10n/app_localizations.dart';
import '../src/app_state.dart';
import '../src/models.dart';
import '../src/theme.dart';

class OverviewPage extends StatelessWidget {
  const OverviewPage({super.key, required this.state});

  final AppState state;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return ListenableBuilder(
      listenable: state,
      builder: (context, _) {
        final info = state.deviceInfo;
        if (info == null) {
          return Center(child: Text(l10n.debugEmpty));
        }
        return ListView(
          padding: const EdgeInsets.all(24),
          children: [
            Wrap(
              spacing: 16,
              runSpacing: 16,
              children: [
                _HeadsetCard(l10n: l10n, info: info),
                _ControllersCard(l10n: l10n, ctrls: state.ctrls),
                _LinkCard(l10n: l10n, state: state),
              ],
            ),
          ],
        );
      },
    );
  }
}

class _Card extends StatelessWidget {
  const _Card({required this.title, required this.child});
  final String title;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: 340,
      child: Card(
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(title, style: Theme.of(context).textTheme.titleSmall),
              const SizedBox(height: 12),
              child,
            ],
          ),
        ),
      ),
    );
  }
}

class _Row extends StatelessWidget {
  const _Row(this.k, this.v);
  final String k;
  final String v;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(k, style: Theme.of(context).textTheme.bodySmall),
          const SizedBox(width: 12),
          Flexible(
            child: Text(v,
                textAlign: TextAlign.right,
                style: Theme.of(context)
                    .textTheme
                    .bodySmall
                    ?.copyWith(color: Colors.white70)),
          ),
        ],
      ),
    );
  }
}

class _HeadsetCard extends StatelessWidget {
  const _HeadsetCard({required this.l10n, required this.info});
  final AppLocalizations l10n;
  final DeviceInfo info;

  @override
  Widget build(BuildContext context) {
    String or(String s) => s.isEmpty ? l10n.valueUnknown : s;
    return _Card(
      title: l10n.overviewHeadset,
      child: Column(
        children: [
          _Row('', info.headsetName),
          _Row(l10n.overviewAndroid,
              '${or(info.androidRelease)} (sdk ${info.sdkInt})'),
          _Row(l10n.overviewHibiscus, or(info.hibiscusVersion)),
          _Row(l10n.overviewBuild, or(info.buildId)),
          _Row(l10n.overviewTrackingMode, info.trackingMode.name),
          _Row(l10n.overviewBattery,
              info.batteryLevel >= 0 ? '${info.batteryLevel}%' : l10n.valueUnknown),
        ],
      ),
    );
  }
}

class _ControllersCard extends StatelessWidget {
  const _ControllersCard({required this.l10n, required this.ctrls});
  final AppLocalizations l10n;
  final List<CtrlState> ctrls;

  @override
  Widget build(BuildContext context) {
    return _Card(
      title: l10n.overviewControllers,
      child: Column(
        children: [
          for (final c in ctrls)
            _CtrlRow(
              label: c.index == 0 ? l10n.ctrlLeft : l10n.ctrlRight,
              ctrl: c,
              l10n: l10n,
            ),
        ],
      ),
    );
  }
}

class _CtrlRow extends StatelessWidget {
  const _CtrlRow(
      {required this.label, required this.ctrl, required this.l10n});
  final String label;
  final CtrlState ctrl;
  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final on = ctrl.connected;
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 6),
      child: Row(
        children: [
          Icon(Icons.sports_esports,
              size: 20, color: on ? CteTheme.accent : Colors.white24),
          const SizedBox(width: 10),
          Expanded(child: Text(label)),
          if (on) ...[
            if (ctrl.battery >= 0)
              Text(l10n.ctrlBattery(ctrl.battery),
                  style: Theme.of(context).textTheme.bodySmall),
            const SizedBox(width: 8),
            Icon(
              ctrl.tracked ? Icons.gps_fixed : Icons.gps_not_fixed,
              size: 14,
              color: Colors.white54,
            ),
          ] else
            Text(l10n.ctrlAbsent,
                style: Theme.of(context)
                    .textTheme
                    .bodySmall
                    ?.copyWith(color: Colors.white38)),
        ],
      ),
    );
  }
}

class _LinkCard extends StatelessWidget {
  const _LinkCard({required this.l10n, required this.state});
  final AppLocalizations l10n;
  final AppState state;

  @override
  Widget build(BuildContext context) {
    final info = state.deviceInfo;
    return _Card(
      title: l10n.overviewLink,
      child: Column(
        children: [
          _Row('', state.link?.description ?? ''),
          _Row(l10n.overviewSerial, info?.serial.isNotEmpty == true ? info!.serial : l10n.valueUnknown),
          _Row(l10n.overviewAddress, info?.address.isNotEmpty == true ? info!.address : l10n.valueUnknown),
        ],
      ),
    );
  }
}
