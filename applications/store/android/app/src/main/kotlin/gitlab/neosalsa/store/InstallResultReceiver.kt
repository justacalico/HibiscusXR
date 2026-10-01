package gitlab.neosalsa.store

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.pm.PackageInstaller
import io.flutter.plugin.common.MethodChannel
import java.util.concurrent.ConcurrentHashMap

/// Replies to committed PackageInstaller sessions. Dart installs block
/// on the MethodChannel result until a broadcast lands here.
class InstallResultReceiver : BroadcastReceiver() {
    companion object {
        const val ACTION = "gitlab.neosalsa.store.INSTALL_RESULT"
        val pending = ConcurrentHashMap<Int, MethodChannel.Result>()
    }

    override fun onReceive(context: Context, intent: Intent) {
        val sessionId = intent.getIntExtra(PackageInstaller.EXTRA_SESSION_ID, -1)
        val result = pending.remove(sessionId) ?: return
        when (intent.getIntExtra(
            PackageInstaller.EXTRA_STATUS,
            PackageInstaller.STATUS_FAILURE,
        )) {
            PackageInstaller.STATUS_SUCCESS -> result.success("installed")
            PackageInstaller.STATUS_PENDING_USER_ACTION -> {
                val confirm = intent.getParcelableExtra<Intent>(Intent.EXTRA_INTENT)
                if (confirm == null) {
                    result.success("failed")
                } else {
                    confirm.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    context.startActivity(confirm)
                    result.success("prompted")
                }
            }
            else -> result.success("failed")
        }
    }
}
