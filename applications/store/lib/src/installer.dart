import 'package:flutter/services.dart';

/// What the platform did with an install request.
enum InstallOutcome {
  /// PackageInstaller committed and confirmed the session.
  installed,

  /// The system put up its own confirm dialog - the platform falls back
  /// to this when the app does not hold INSTALL_PACKAGES.
  prompted,

  /// The session or the launch intent failed.
  failed,
}

/// The only piece that must run on device. Kept deliberately thin so the
/// whole decision tree above it stays host-testable.
abstract class ApkInstaller {
  Future<InstallOutcome> installApk(String path);
  Future<bool> openApp(String packageName);
}

/// Talks to the Kotlin side over the installer MethodChannel. The
/// channel name is fixed in AndroidManifest-facing MainActivity.
class ChannelInstaller implements ApkInstaller {
  const ChannelInstaller({
    this.channel = const MethodChannel('gitlab.neosalsa.store/installer'),
  });

  final MethodChannel channel;

  @override
  Future<InstallOutcome> installApk(String path) async {
    final result = await channel.invokeMethod<String>(
      'installApk',
      {'path': path},
    );
    return switch (result) {
      'installed' => InstallOutcome.installed,
      'prompted' => InstallOutcome.prompted,
      _ => InstallOutcome.failed,
    };
  }

  @override
  Future<bool> openApp(String packageName) async {
    final opened = await channel.invokeMethod<bool>(
      'openApp',
      {'package': packageName},
    );
    return opened ?? false;
  }
}
