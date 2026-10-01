package gitlab.neosalsa.store

import android.app.PendingIntent
import android.content.Intent
import android.content.pm.PackageInstaller
import android.content.pm.PackageManager
import android.os.Handler
import android.os.Looper
import androidx.core.content.FileProvider
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import java.io.File

class MainActivity : FlutterActivity() {
    private val channelName = "gitlab.neosalsa.store/installer"
    private val main = Handler(Looper.getMainLooper())

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, channelName)
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "installApk" -> installApk(call.argument("path"), result)
                    "openApp" -> result.success(openApp(call.argument("package")))
                    else -> result.notImplemented()
                }
            }
    }

    private fun openApp(pkg: String?): Boolean {
        if (pkg.isNullOrEmpty()) return false
        val intent = packageManager.getLaunchIntentForPackage(pkg) ?: return false
        return try {
            startActivity(intent)
            true
        } catch (e: Exception) {
            false
        }
    }

    private fun installApk(path: String?, result: MethodChannel.Result) {
        val file = path?.let(::File)
        if (file == null || !file.exists()) {
            result.success("failed")
            return
        }
        // checkCallingOrSelfPermission works on any API level - this app
        // picks up INSTALL_PACKAGES when the image signs it platform-side
        if (checkCallingOrSelfPermission(android.Manifest.permission.INSTALL_PACKAGES) ==
            PackageManager.PERMISSION_GRANTED
        ) {
            // The copy is tens of MB; channel handlers run on the main
            // thread, so a plain copyTo would ANR. Work on a thread,
            // answer on the main one.
            Thread {
                installSilent(file) { outcome ->
                    main.post { result.success(outcome) }
                }
            }.start()
        } else {
            promptInstall(file, result)
        }
    }

    private fun promptInstall(file: File, result: MethodChannel.Result) {
        // Sideloaded debug builds get the system confirm sheet. Matches
        // the library app's path: INSTALL_PACKAGE first, plain VIEW when
        // nothing resolves it.
        val uri = FileProvider.getUriForFile(this, "$packageName.apkprovider", file)
        val mime = "application/vnd.android.package-archive"
        val install = Intent(Intent.ACTION_INSTALL_PACKAGE)
            .setDataAndType(uri, mime)
            .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_ACTIVITY_NEW_TASK)
            .putExtra(Intent.EXTRA_NOT_UNKNOWN_SOURCE, true)
        val answered = try {
            startActivity(install)
            true
        } catch (e: Exception) {
            try {
                startActivity(
                    Intent(Intent.ACTION_VIEW)
                        .setDataAndType(uri, mime)
                        .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_ACTIVITY_NEW_TASK)
                )
                true
            } catch (e2: Exception) {
                false
            }
        }
        result.success(if (answered) "prompted" else "failed")
    }

    private fun installSilent(file: File, done: (String) -> Unit) {
        val installer = packageManager.packageInstaller
        var sessionId = -1
        var session: PackageInstaller.Session? = null
        try {
            val params = PackageInstaller.SessionParams(
                PackageInstaller.SessionParams.MODE_FULL_INSTALL,
            )
            sessionId = installer.createSession(params)
            session = installer.openSession(sessionId)
            session.openWrite("base.apk", 0, file.length()).use { out ->
                file.inputStream().use { it.copyTo(out) }
                session.fsync(out)
            }
            val intent = Intent(this, InstallResultReceiver::class.java)
                .setAction(InstallResultReceiver.ACTION)
            val sender = PendingIntent.getBroadcast(
                this,
                sessionId,
                intent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE,
            ).intentSender
            // InstallResultReceiver owns the result from here.
            InstallResultReceiver.pending[sessionId] = { outcome -> done(outcome) }
            session.commit(sender)
        } catch (e: Exception) {
            session?.abandon()
            if (sessionId >= 0) {
                runCatching { installer.abandonSession(sessionId) }
                InstallResultReceiver.pending.remove(sessionId)
            }
            done("failed")
        } finally {
            session?.close()
        }
    }
}
