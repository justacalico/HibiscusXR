import 'dart:typed_data';

import '../models.dart';

/// Everything the app needs from the platform it runs on. Implemented by
/// the Android MethodChannel bridge and by [FakeAppSource] in tests.
abstract class AppSource {
  /// All launchable apps on the device.
  Future<List<AppEntry>> listApps();

  /// PNG bytes for the app icon, or null when the platform has none.
  Future<Uint8List?> icon(String packageName);

  /// Launches the app. False when the platform refused.
  Future<bool> launch(AppEntry app);

  /// Starts the system uninstall flow. False when it could not start.
  Future<bool> uninstall(AppEntry app);

  /// Opens the system details page for the app.
  Future<bool> openAppInfo(AppEntry app);

  /// Lets the user pick an APK and hands it to the system installer.
  /// False when picking/installing could not start.
  Future<bool> pickAndInstallApk();

  /// Fires whenever the installed catalog changes (add/remove/replace).
  Stream<void> get changes;
}
