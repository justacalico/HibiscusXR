package gitlab.neosalsa.store

import android.app.PendingIntent
import android.content.Intent
import android.content.pm.PackageInstaller
import android.content.pm.PackageManager
import androidx.core.content.FileProvider
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import java.io.File

class MainActivity : FlutterActivity() {
    private val channelName = "gitlab.neosalsa.store/installer"

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
        startActivity(intent)
        return true
    }

    private fun installApk(path: String?, result: MethodChannel.Result) {
        val file = path?.let(::File)
        if (file == null || !file.exists()) {
            result.success("failed")
            return
        }
        if (checkSelfPermission(android.Manifest.permission.INSTALL_PACKAGES) ==
            PackageManager.PERMISSION_GRANTED
        ) {
            installSilent(file, result)
        } else {
            // Sideloaded debug builds fall back to the system sheet.
            promptInstall(file, result)
        }
    }

    private fun promptInstall(file: File, result: MethodChannel.Result) {
        val uri = FileProvider.getUriForFile(this, "$packageName.apkprovider", file)
        val intent = Intent(Intent.ACTION_VIEW).apply {
            setDataAndType(uri, "application/vnd.android.package-archive")
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_ACTIVITY_NEW_TASK)
        }
        startActivity(intent)
        result.success("prompted")
    }

    private fun installSilent(file: File, result: MethodChannel.Result) {
        val installer = packageManager.packageInstaller
        val params = PackageInstaller.SessionParams(
            PackageInstaller.SessionParams.MODE_FULL_INSTALL,
        )
        var session: PackageInstaller.Session? = null
        try {
            val sessionId = installer.createSession(params)
            session = installer.openSession(sessionId)
            session.openWrite("base.apk", 0, file.length()).use { out ->
                file.inputStream().use { it.copyTo(out) }
                session.fsync(out)
            }
            InstallResultReceiver.pending[sessionId] = result
            val intent = Intent(this, InstallResultReceiver::class.java)
                .setAction(InstallResultReceiver.ACTION)
            val sender = PendingIntent.getBroadcast(
                this,
                sessionId,
                intent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE,
            ).intentSender
            session.commit(sender)
        } catch (e: Exception) {
            session?.abandon()
            result.success("failed")
        } finally {
            session?.close()
        }
    }
}
