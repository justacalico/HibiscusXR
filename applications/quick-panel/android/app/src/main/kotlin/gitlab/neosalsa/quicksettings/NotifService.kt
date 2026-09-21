package gitlab.neosalsa.quicksettings

import android.app.Notification
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log

// Mirrors the active shade into the panel. The service owns the list;
// MainActivity reads it for the load snapshot and subscribes for
// changes. Dismissal runs through cancelNotification/cancelAll.
class NotifService : NotificationListenerService() {

    override fun onListenerConnected() {
        instance = this
        Log.i(TAG, "notif listener bound")
        publish()
    }

    override fun onListenerDisconnected() {
        instance = null
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) = publish()
    override fun onNotificationRemoved(sbn: StatusBarNotification) = publish()

    private fun publish() {
        val list = snapshot()
        lastList = list
        sink?.invoke(list)
    }

    private fun snapshot(): List<Map<String, Any>> {
        val pm = packageManager
        return try {
            activeNotifications
                ?.sortedByDescending { it.postTime }
                ?.map { sbn -> row(sbn, pm) }
                ?: emptyList()
        } catch (e: SecurityException) {
            emptyList()
        }
    }

    private fun row(sbn: StatusBarNotification, pm: android.content.pm.PackageManager): Map<String, Any> {
        val n = sbn.notification
        val e = n?.extras
        val app = try {
            pm.getApplicationLabel(
                pm.getApplicationInfo(sbn.packageName, 0),
            ).toString()
        } catch (ex: Exception) {
            sbn.packageName
        }
        return mapOf(
            "key" to sbn.key,
            "app" to app,
            "title" to (e?.getCharSequence(Notification.EXTRA_TITLE)?.toString() ?: ""),
            "text" to (e?.getCharSequence(Notification.EXTRA_TEXT)?.toString() ?: ""),
            "postMs" to sbn.postTime,
            "clearable" to (sbn.isClearable),
        )
    }

    fun dismiss(key: String) {
        try {
            cancelNotification(key)
        } catch (e: Exception) {
            Log.w(TAG, "dismiss failed", e)
        }
    }

    fun dismissAll() {
        try {
            cancelAllNotifications()
        } catch (e: Exception) {
            Log.w(TAG, "dismissAll failed", e)
        }
    }

    companion object {
        private const val TAG = "pn2qs.notif"

        // Panel-visible copy of the shade. Written on the listener
        // thread, read on the platform thread.
        @Volatile var lastList: List<Map<String, Any>> = emptyList()
            private set

        @Volatile var instance: NotifService? = null
            private set

        // Wired by MainActivity's EventChannel listen/cancel.
        @Volatile var sink: ((List<Map<String, Any>>) -> Unit)? = null
    }
}
