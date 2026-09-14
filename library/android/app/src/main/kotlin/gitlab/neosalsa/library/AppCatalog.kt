package gitlab.neosalsa.library

import android.content.Context
import android.content.Intent
import android.content.pm.ApplicationInfo
import android.content.pm.PackageManager
import android.content.pm.ResolveInfo
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.drawable.Drawable
import android.net.Uri
import android.os.Build
import android.provider.Settings
import java.io.ByteArrayOutputStream

// Thin bridge over PackageManager. No state, no decisions - it answers
// the platform channel's questions and fires the intents it is told to.
class AppCatalog(private val context: Context) {

    private val pm: PackageManager get() = context.packageManager

    fun listApps(): List<Map<String, Any?>> {
        val intent = Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_LAUNCHER)
        val resolved = queryActivities(intent)
        val seen = HashSet<String>()
        val out = ArrayList<Map<String, Any?>>(resolved.size)
        for (ri in resolved) {
            val pkg = ri.activityInfo.packageName ?: continue
            if (!seen.add(pkg)) continue
            val info = packageInfo(pkg)
            out.add(
                mapOf(
                    "packageName" to pkg,
                    "label" to ri.loadLabel(pm).toString(),
                    "activityName" to ri.activityInfo.name,
                    "isSystem" to
                        ((ri.activityInfo.applicationInfo?.flags ?: 0) and
                            ApplicationInfo.FLAG_SYSTEM != 0),
                    "versionName" to info?.versionName,
                    "firstInstallTime" to (info?.firstInstallTime ?: 0L),
                    "lastUpdateTime" to (info?.lastUpdateTime ?: 0L),
                )
            )
        }
        return out
    }

    fun iconPng(pkg: String): ByteArray? {
        val drawable = try {
            pm.getApplicationIcon(pkg)
        } catch (e: PackageManager.NameNotFoundException) {
            return null
        }
        return drawableToPng(drawable)
    }

    fun launch(pkg: String): Boolean {
        val intent = pm.getLaunchIntentForPackage(pkg)
            ?: Intent(Intent.ACTION_MAIN)
                .addCategory(Intent.CATEGORY_LAUNCHER)
                .setPackage(pkg)
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        return try {
            context.startActivity(intent)
            true
        } catch (e: Exception) {
            false
        }
    }

    fun uninstall(pkg: String): Boolean {
        val intent = Intent(Intent.ACTION_UNINSTALL_PACKAGE, Uri.parse("package:$pkg"))
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        return try {
            context.startActivity(intent)
            true
        } catch (e: Exception) {
            false
        }
    }

    fun openAppInfo(pkg: String): Boolean {
        val intent = Intent(
            Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
            Uri.parse("package:$pkg")
        ).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        return try {
            context.startActivity(intent)
            true
        } catch (e: Exception) {
            false
        }
    }

    fun installApk(uri: Uri): Boolean {
        val base = Intent(Intent.ACTION_INSTALL_PACKAGE)
            .setDataAndType(uri, APK_MIME)
            .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            .putExtra(Intent.EXTRA_NOT_UNKNOWN_SOURCE, true)
        return try {
            context.startActivity(base)
            true
        } catch (e: Exception) {
            // Some builds only answer ACTION_VIEW for package uris.
            try {
                context.startActivity(
                    Intent(Intent.ACTION_VIEW)
                        .setDataAndType(uri, APK_MIME)
                        .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                        .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                )
                true
            } catch (e2: Exception) {
                false
            }
        }
    }

    private fun queryActivities(intent: Intent): List<ResolveInfo> =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            pm.queryIntentActivities(
                intent,
                PackageManager.ResolveInfoFlags.of(0)
            )
        } else {
            @Suppress("DEPRECATION")
            pm.queryIntentActivities(intent, 0)
        }

    private fun packageInfo(pkg: String) = try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            pm.getPackageInfo(pkg, PackageManager.PackageInfoFlags.of(0))
        } else {
            @Suppress("DEPRECATION")
            pm.getPackageInfo(pkg, 0)
        }
    } catch (e: PackageManager.NameNotFoundException) {
        null
    }

    private fun drawableToPng(drawable: Drawable): ByteArray {
        val size = ICON_SIZE
        val bitmap = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        drawable.setBounds(0, 0, size, size)
        drawable.draw(canvas)
        val out = ByteArrayOutputStream()
        bitmap.compress(Bitmap.CompressFormat.PNG, 100, out)
        bitmap.recycle()
        return out.toByteArray()
    }

    companion object {
        private const val ICON_SIZE = 144
        private const val APK_MIME = "application/vnd.android.package-archive"
    }
}
