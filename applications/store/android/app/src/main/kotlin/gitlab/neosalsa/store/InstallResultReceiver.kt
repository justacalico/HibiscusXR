package gitlab.neosalsa.store

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.pm.PackageInstaller
import java.util.concurrent.ConcurrentHashMap

/// Replies to committed PackageInstaller sessions. Dart installs block
/// on the MethodChannel result until a broadcast lands here - the
/// pending map holds "return 'installed' to Dart" closures rather than
/// raw Results because the session work happens off the main thread.
class InstallResultReceiver : BroadcastReceiver() {
    companion object {
        const val ACTION = "gitlab.neosalsa.store.INSTALL_RESULT"
        val pending = ConcurrentHashMap<Int, (String) -> Unit>()
    }

    override fun onReceive(context: Context, intent: Intent) {
        val sessionId = intent.getIntExtra(PackageInstaller.EXTRA_SESSION_ID, -1)
        val result = pending.remove(sessionId) ?: return
        when (intent.getIntExtra(
            PackageInstaller.EXTRA_STATUS,
            PackageInstaller.STATUS_FAILURE,
        )) {
            PackageInstaller.STATUS_SUCCESS -> result("installed")
            PackageInstaller.STATUS_PENDING_USER_ACTION -> {
                val confirm = intent.getParcelableExtra<Intent>(Intent.EXTRA_INTENT)
                if (confirm == null) {
                    result("failed")
                } else {
                    confirm.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    try {
                        context.startActivity(confirm)
                        result("prompted")
                    } catch (e: Exception) {
                        result("failed")
                    }
                }
            }
            else -> result("failed")
        }
    }
}
