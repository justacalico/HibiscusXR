package gitlab.neosalsa.library

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodChannel

class MainActivity : FlutterActivity() {

    private var catalog: AppCatalog? = null
    private var changeSink: EventChannel.EventSink? = null
    private var packageReceiver: BroadcastReceiver? = null
    private var pendingInstall: MethodChannel.Result? = null

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        val c = AppCatalog(this)
        catalog = c
        val messenger = flutterEngine.dartExecutor.binaryMessenger

        MethodChannel(messenger, APPS_CHANNEL).setMethodCallHandler { call, result ->
            when (call.method) {
                "getApps" -> result.success(c.listApps())
                "getIcon" -> {
                    val pkg = call.argument<String>("package")
                    if (pkg == null) {
                        result.error("bad_args", "missing package", null)
                    } else {
                        result.success(c.iconPng(pkg))
                    }
                }
                "launch" -> withPackage(call, result) { c.launch(it) }
                "uninstall" -> withPackage(call, result) { c.uninstall(it) }
                "openAppInfo" -> withPackage(call, result) { c.openAppInfo(it) }
                "pickAndInstallApk" -> pickApk(result)
                else -> result.notImplemented()
            }
        }

        EventChannel(messenger, CHANGES_CHANNEL).setStreamHandler(
            object : EventChannel.StreamHandler {
                override fun onListen(arguments: Any?, events: EventChannel.EventSink) {
                    changeSink = events
                    registerPackageReceiver()
                }

                override fun onCancel(arguments: Any?) {
                    changeSink = null
                    unregisterPackageReceiver()
                }
            }
        )
    }

    override fun cleanUpFlutterEngine(flutterEngine: FlutterEngine) {
        unregisterPackageReceiver()
        changeSink = null
        catalog = null
        super.cleanUpFlutterEngine(flutterEngine)
    }

    private fun withPackage(
        call: io.flutter.plugin.common.MethodCall,
        result: MethodChannel.Result,
        block: (String) -> Boolean
    ) {
        val pkg = call.argument<String>("package")
        if (pkg == null) {
            result.error("bad_args", "missing package", null)
        } else {
            result.success(block(pkg))
        }
    }

    private fun pickApk(result: MethodChannel.Result) {
        if (pendingInstall != null) {
            pendingInstall?.success(false)
        }
        pendingInstall = result
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/vnd.android.package-archive"
        }
        try {
            @Suppress("DEPRECATION")
            startActivityForResult(intent, REQ_PICK_APK)
        } catch (e: Exception) {
            pendingInstall = null
            result.success(false)
        }
    }

    @Deprecated("startActivityForResult pair")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != REQ_PICK_APK) return
        val result = pendingInstall
        pendingInstall = null
        val uri = data?.data
        if (resultCode != RESULT_OK || uri == null) {
            result?.success(false)
            return
        }
        result?.success(catalog?.installApk(uri) ?: false)
    }

    private fun registerPackageReceiver() {
        if (packageReceiver != null) return
        val r = object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                changeSink?.success(
                    mapOf("package" to intent.data?.schemeSpecificPart)
                )
            }
        }
        val filter = IntentFilter().apply {
            addAction(Intent.ACTION_PACKAGE_ADDED)
            addAction(Intent.ACTION_PACKAGE_REMOVED)
            addAction(Intent.ACTION_PACKAGE_CHANGED)
            addAction(Intent.ACTION_PACKAGE_REPLACED)
            addDataScheme("package")
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(r, filter, Context.RECEIVER_NOT_EXPORTED)
        } else {
            @Suppress("DEPRECATION", "UnspecifiedRegisterReceiverFlag")
            registerReceiver(r, filter)
        }
        packageReceiver = r
    }

    private fun unregisterPackageReceiver() {
        val r = packageReceiver ?: return
        packageReceiver = null
        try {
            unregisterReceiver(r)
        } catch (e: IllegalArgumentException) {
            // not registered anymore
        }
    }

    companion object {
        private const val APPS_CHANNEL = "gitlab.neosalsa.library/apps"
        private const val CHANGES_CHANNEL = "gitlab.neosalsa.library/changes"
        private const val REQ_PICK_APK = 0x4C49
    }
}
