import 'package:flutter/material.dart';

import 'adb_runner.dart';

/// Runs [scan] after the first frame - connect pages mount, then the
/// adb scan fires without blocking first paint.
void scanAfterFirstFrame(void Function() scan) {
  WidgetsBinding.instance.addPostFrameCallback((_) => scan());
}

/// The connect pages' outer frame: a centered 560-wide scrolling
/// column that rebuilds whenever [listenable] fires.
class ConnectScaffold extends StatelessWidget {
  const ConnectScaffold(
      {super.key, required this.listenable, required this.children});

  final Listenable listenable;
  final List<Widget> children;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 560),
          child: ListenableBuilder(
            listenable: listenable,
            builder: (context, _) => ListView(
              shrinkWrap: true,
              padding: const EdgeInsets.all(32),
              children: children,
            ),
          ),
        ),
      ),
    );
  }
}

/// Error banner the connect pages show for scan/connect failures.
class ConnectErrorCard extends StatelessWidget {
  const ConnectErrorCard(
      {super.key, required this.message, required this.color});

  final String message;
  final Color color;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Text(message, style: TextStyle(color: color)),
      ),
    );
  }
}

/// Device-scan card: a title row with a refresh affordance, then the
/// device list or an empty label. [scanningLabel] picks the scanning
/// presentation - null swaps the refresh button for a spinner, a label
/// replaces the device list instead.
class AdbScanCard extends StatelessWidget {
  const AdbScanCard({
    super.key,
    required this.title,
    this.titleStyle,
    required this.refreshTooltip,
    required this.noDevicesLabel,
    this.scanningLabel,
    required this.devices,
    required this.scanning,
    this.busy = false,
    required this.onRefresh,
    required this.tileBuilder,
    this.padding = const EdgeInsets.all(16),
    this.titleGap = 0,
    this.emptyPadding = EdgeInsets.zero,
  });

  final String title;
  final TextStyle? titleStyle;
  final String refreshTooltip;
  final String noDevicesLabel;
  final String? scanningLabel;
  final List<AdbDeviceRow> devices;
  final bool scanning;
  final bool busy;
  final VoidCallback onRefresh;
  final Widget Function(AdbDeviceRow) tileBuilder;
  final EdgeInsets padding;
  final double titleGap;
  final EdgeInsets emptyPadding;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: padding,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Expanded(child: Text(title, style: titleStyle)),
                if (scanning && scanningLabel == null)
                  const SizedBox(
                    width: 16,
                    height: 16,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                else
                  IconButton(
                    icon: const Icon(Icons.refresh, size: 18),
                    tooltip: refreshTooltip,
                    onPressed: busy || scanning ? null : onRefresh,
                  ),
              ],
            ),
            if (titleGap > 0) SizedBox(height: titleGap),
            if (scanning && scanningLabel != null)
              Text(scanningLabel!)
            else if (devices.isEmpty)
              Padding(
                padding: emptyPadding,
                child: Text(noDevicesLabel,
                    style: Theme.of(context).textTheme.bodySmall),
              )
            else
              for (final d in devices) tileBuilder(d),
          ],
        ),
      ),
    );
  }
}

/// One scanned adb device: usb icon, model (or the caller's fallback),
/// a serial line and an optional trailing action.
class AdbDeviceTile extends StatelessWidget {
  const AdbDeviceTile({
    super.key,
    required this.device,
    this.unknownLabel,
    this.showState = false,
    this.iconSize = 18,
    this.trailing,
    this.onTap,
  });

  final AdbDeviceRow device;

  /// Title when the row has no model - falls back to the serial.
  final String? unknownLabel;
  final bool showState;
  final double iconSize;
  final Widget? trailing;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    return ListTile(
      dense: true,
      contentPadding: EdgeInsets.zero,
      leading: Icon(Icons.usb, size: iconSize),
      title: Text(device.model.isNotEmpty
          ? device.model
          : (unknownLabel ?? device.serial)),
      subtitle: Text(
          showState ? '${device.serial} - ${device.state}' : device.serial),
      trailing: trailing,
      onTap: onTap,
    );
  }
}
