import 'package:flutter/material.dart';

import '../l10n/app_localizations.dart';
import '../src/app_state.dart';
import '../src/models.dart';
import '../src/theme.dart';

/// Live 6DoF/3DoF feed: latest pose numbers, a top-down position trail,
/// and the raw sample stream.
class TrackingPage extends StatelessWidget {
  const TrackingPage({super.key, required this.state});

  final AppState state;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return ListenableBuilder(
      listenable: state,
      builder: (context, _) {
        final samples = state.poseBuf.toList();
        final latest = samples.isEmpty ? null : samples.last;
        return Row(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            SizedBox(
              width: 340,
              child: ListView(
                padding: const EdgeInsets.all(24),
                children: [
                  Row(
                    children: [
                      Text(l10n.trackingHead,
                          style: Theme.of(context).textTheme.titleSmall),
                      const Spacer(),
                      Text(
                        l10n.trackingRate(
                            state.poseRate.toStringAsFixed(1)),
                        style: Theme.of(context)
                            .textTheme
                            .bodySmall
                            ?.copyWith(color: Colors.white54),
                      ),
                    ],
                  ),
                  const SizedBox(height: 12),
                  if (latest == null)
                    Padding(
                      padding: const EdgeInsets.symmetric(vertical: 24),
                      child: Text(l10n.trackingIdle,
                          style: Theme.of(context)
                              .textTheme
                              .bodySmall
                              ?.copyWith(color: Colors.white38)),
                    )
                  else ...[
                    Chip(
                      label: Text(l10n.trackingMode(latest.mode.name)),
                      visualDensity: VisualDensity.compact,
                    ),
                    const SizedBox(height: 12),
                    _PoseNumbers(l10n: l10n, sample: latest),
                    const SizedBox(height: 16),
                    SizedBox(
                      height: 220,
                      child: Card(
                        child: Padding(
                          padding: const EdgeInsets.all(8),
                          child: CustomPaint(
                            painter: TrailPainter(
                                samples.map((s) => s.pose).toList()),
                            size: Size.infinite,
                          ),
                        ),
                      ),
                    ),
                  ],
                  const SizedBox(height: 16),
                  for (final c in state.ctrls) _CtrlLine(c: c, l10n: l10n),
                ],
              ),
            ),
            const VerticalDivider(width: 1),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Padding(
                    padding: const EdgeInsets.fromLTRB(20, 20, 20, 8),
                    child: Text(l10n.trackingLog,
                        style: Theme.of(context).textTheme.titleSmall),
                  ),
                  Expanded(
                    child: ListView.builder(
                      padding: const EdgeInsets.symmetric(horizontal: 20),
                      itemCount: samples.length,
                      itemBuilder: (context, i) {
                        final s = samples[i];
                        final p = s.pose;
                        return Text(
                          't=${s.timestampNs} '
                          'p=(${p.x.toStringAsFixed(3)}, ${p.y.toStringAsFixed(3)}, ${p.z.toStringAsFixed(3)}) '
                          'q=(${p.qx.toStringAsFixed(3)}, ${p.qy.toStringAsFixed(3)}, ${p.qz.toStringAsFixed(3)}, ${p.qw.toStringAsFixed(3)}) '
                          'st=${s.trackingState}',
                          style: Theme.of(context)
                              .textTheme
                              .bodySmall
                              ?.copyWith(
                                  fontFamily: 'monospace',
                                  color: Colors.white54,
                                  fontSize: 11),
                        );
                      },
                    ),
                  ),
                ],
              ),
            ),
          ],
        );
      },
    );
  }
}

class _PoseNumbers extends StatelessWidget {
  const _PoseNumbers({required this.l10n, required this.sample});
  final AppLocalizations l10n;
  final PoseSample sample;

  @override
  Widget build(BuildContext context) {
    final p = sample.pose;
    final (yaw, pitch, roll) = p.toEulerDeg();
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(l10n.trackingPosition,
            style: Theme.of(context).textTheme.labelSmall),
        Text(
          '${p.x.toStringAsFixed(3)}  ${p.y.toStringAsFixed(3)}  ${p.z.toStringAsFixed(3)} m',
          style: Theme.of(context)
              .textTheme
              .bodyMedium
              ?.copyWith(fontFamily: 'monospace'),
        ),
        const SizedBox(height: 8),
        Text(l10n.trackingOrientation,
            style: Theme.of(context).textTheme.labelSmall),
        Text(
          'yaw ${yaw.toStringAsFixed(1)}  pitch ${pitch.toStringAsFixed(1)}  roll ${roll.toStringAsFixed(1)}',
          style: Theme.of(context)
              .textTheme
              .bodyMedium
              ?.copyWith(fontFamily: 'monospace'),
        ),
      ],
    );
  }
}

class _CtrlLine extends StatelessWidget {
  const _CtrlLine({required this.c, required this.l10n});
  final CtrlState c;
  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final label = c.index == 0 ? l10n.ctrlLeft : l10n.ctrlRight;
    final p = c.pose;
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text('$label - ${c.connected ? l10n.ctrlConnected : l10n.ctrlAbsent}',
              style: Theme.of(context).textTheme.labelSmall),
          if (c.connected)
            Text(
              'p=(${p.x.toStringAsFixed(2)}, ${p.y.toStringAsFixed(2)}, ${p.z.toStringAsFixed(2)}) '
              'batt=${c.battery} ${c.tracked ? l10n.ctrlTracked : l10n.ctrlUntracked}',
              style: Theme.of(context)
                  .textTheme
                  .bodySmall
                  ?.copyWith(fontFamily: 'monospace', color: Colors.white54),
            ),
        ],
      ),
    );
  }
}

/// Top-down XZ trail of the head over the buffered samples.
class TrailPainter extends CustomPainter {
  TrailPainter(this.poses);
  final List<Pose> poses;

  @override
  void paint(Canvas canvas, Size size) {
    if (poses.isEmpty) return;
    // normalize the trail to the view rect with padding
    var minX = double.infinity, maxX = double.negativeInfinity;
    var minZ = double.infinity, maxZ = double.negativeInfinity;
    for (final p in poses) {
      minX = p.x < minX ? p.x : minX;
      maxX = p.x > maxX ? p.x : maxX;
      minZ = p.z < minZ ? p.z : minZ;
      maxZ = p.z > maxZ ? p.z : maxZ;
    }
    const pad = 0.1;
    final spanX = (maxX - minX).abs() < 1e-4 ? 1.0 : maxX - minX;
    final spanZ = (maxZ - minZ).abs() < 1e-4 ? 1.0 : maxZ - minZ;
    Offset map(Pose p) => Offset(
          pad * size.width + (p.x - minX) / spanX * size.width * (1 - 2 * pad),
          pad * size.height +
              (1 - (p.z - minZ) / spanZ) * size.height * (1 - 2 * pad),
        );
    final line = Paint()
      ..color = CteTheme.accent.withValues(alpha: 0.5)
      ..strokeWidth = 1.2
      ..style = PaintingStyle.stroke;
    final path = Path()..moveTo(map(poses.first).dx, map(poses.first).dy);
    for (final p in poses.skip(1)) {
      final o = map(p);
      path.lineTo(o.dx, o.dy);
    }
    canvas.drawPath(path, line);
    canvas.drawCircle(
        map(poses.last), 4, Paint()..color = CteTheme.accent);
  }

  @override
  bool shouldRepaint(TrailPainter old) => !identical(old.poses, poses);
}
