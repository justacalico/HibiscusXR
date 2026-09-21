package gitlab.neosalsa.quicksettings

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.media.AudioManager
import android.net.wifi.WifiManager
import android.os.BatteryManager
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodChannel

// android.media.AudioManager.VOLUME_CHANGED_ACTION is @hide
private const val VOLUME_CHANGED = "android.media.VOLUME_CHANGED_ACTION"

class MainActivity : FlutterActivity() {
    private var eventSink: EventChannel.EventSink? = null

    // Panel-only toggles our OS layer owns; broadcast so whoever holds
    // the real switch (qvrd, thermalserviced, ...) can react.
    private val seamToggles = setOf("seethrough", "boundary")

    // Names of currently connected bluetooth devices, kept warm by the
    // profile proxies below plus ACL connect/disconnect broadcasts.
    private val connectedBtDevices = mutableSetOf<String>()

    private val profileListener = object : BluetoothProfile.ServiceListener {
        override fun onServiceConnected(profile: Int, proxy: BluetoothProfile) {
            try {
                for (d in proxy.connectedDevices) {
                    d?.name?.let(connectedBtDevices::add)
                }
            } catch (_: SecurityException) {}
        }

        override fun onServiceDisconnected(profile: Int) {}
    }

    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val update = mutableMapOf<String, Any?>()
            when (intent.action) {
                Intent.ACTION_BATTERY_CHANGED ->
                    update["batteryLevel"] = batteryLevel()
                WifiManager.WIFI_STATE_CHANGED_ACTION,
                WifiManager.NETWORK_STATE_CHANGED_ACTION -> {
                    update["wifiSsid"] = wifiSsid()
                    update["toggles"] = mapOf("wifi" to wifiManager().isWifiEnabled)
                }
                BluetoothAdapter.ACTION_STATE_CHANGED ->
                    update["toggles"] = mapOf("bluetooth" to bluetoothOn())
                VOLUME_CHANGED ->
                    update["volume"] = volume()
                Intent.ACTION_AIRPLANE_MODE_CHANGED ->
                    update["toggles"] = mapOf("airplaneMode" to airplaneOn())
                BluetoothDevice.ACTION_ACL_CONNECTED,
                BluetoothDevice.ACTION_ACL_DISCONNECTED -> {
                    val dev = btDevice(intent)
                    try {
                        if (intent.action == BluetoothDevice.ACTION_ACL_CONNECTED) {
                            dev?.name?.let(connectedBtDevices::add)
                        } else {
                            dev?.name?.let(connectedBtDevices::remove)
                        }
                    } catch (_: SecurityException) {}
                    // empty string clears the tile's "connected" label
                    update["bluetoothDevice"] = connectedBtDevices.firstOrNull() ?: ""
                }
            }
            if (update.isNotEmpty()) eventSink?.success(update)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // SSID reads need location; device names need BLUETOOTH_CONNECT on S+
        val needed = buildList {
            add(Manifest.permission.ACCESS_FINE_LOCATION)
            if (Build.VERSION.SDK_INT >= 31) {
                add(Manifest.permission.BLUETOOTH_CONNECT)
            }
        }.filter {
            checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED
        }.toTypedArray()
        if (needed.isNotEmpty()) requestPermissions(needed, 0)
        enableNotifAccess()
    }

    // Privileged install path: add our listener to the enabled list so
    // the shade mirror binds without a trip through system settings.
    private fun enableNotifAccess() {
        try {
            val svc = "$packageName/${NotifService::class.java.name}"
            val cr = contentResolver
            val cur = Settings.Secure.getString(cr, "enabled_notification_listeners")
            if (cur == null || !cur.contains(svc)) {
                Settings.Secure.putString(
                    cr,
                    "enabled_notification_listeners",
                    if (cur.isNullOrEmpty()) svc else "$cur:$svc",
                )
            }
        } catch (_: Exception) {}
    }

    private fun btDevice(intent: Intent): BluetoothDevice? =
        if (Build.VERSION.SDK_INT >= 33) {
            intent.getParcelableExtra(
                BluetoothDevice.EXTRA_DEVICE, BluetoothDevice::class.java,
            )
        } else {
            @Suppress("DEPRECATION")
            intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE)
        }

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        val messenger = flutterEngine.dartExecutor.binaryMessenger
        MethodChannel(messenger, "gitlab.neosalsa.quicksettings/system")
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "load" -> result.success(snapshot())
                    "setVolume" -> {
                        setVolume(call.argument<Double>("volume") ?: 0.5)
                        result.success(null)
                    }
                    "setBrightness" -> {
                        setBrightness(call.argument<Double>("brightness") ?: 0.5)
                        result.success(null)
                    }
                    "requestToggle" -> {
                        val id = call.argument<String>("id") ?: ""
                        requestToggle(id, call.argument<Boolean>("on") ?: false)
                        result.success(null)
                    }
                    "performAction" -> {
                        performAction(call.argument<String>("id") ?: "")
                        result.success(null)
                    }
                    "dismissNotification" -> {
                        NotifService.instance?.dismiss(
                            call.argument<String>("key") ?: "",
                        )
                        result.success(null)
                    }
                    "dismissAllNotifications" -> {
                        NotifService.instance?.dismissAll()
                        result.success(null)
                    }
                    else -> result.notImplemented()
                }
            }
        EventChannel(messenger, "gitlab.neosalsa.quicksettings/events")
            .setStreamHandler(object : EventChannel.StreamHandler {
                override fun onListen(arguments: Any?, sink: EventChannel.EventSink) {
                    eventSink = sink
                    NotifService.sink = { list ->
                        runOnUiThread {
                            eventSink?.success(mapOf("notifications" to list))
                        }
                    }
                    registerReceiver(
                        receiver,
                        IntentFilter().apply {
                            addAction(Intent.ACTION_BATTERY_CHANGED)
                            addAction(WifiManager.WIFI_STATE_CHANGED_ACTION)
                            addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION)
                            addAction(BluetoothAdapter.ACTION_STATE_CHANGED)
                            addAction(VOLUME_CHANGED)
                            addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED)
                            addAction(BluetoothDevice.ACTION_ACL_CONNECTED)
                            addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED)
                        },
                    )
                }

                override fun onCancel(arguments: Any?) {
                    eventSink = null
                    NotifService.sink = null
                    unregisterReceiver(receiver)
                }
            })
        // Warm the connected-device names for devices that paired before
        // the panel opened.
        getSystemService(BluetoothManager::class.java)?.adapter?.let { adapter ->
            adapter.getProfileProxy(this, profileListener, BluetoothProfile.A2DP)
            adapter.getProfileProxy(this, profileListener, BluetoothProfile.HEADSET)
        }
    }

    private fun snapshot(): Map<String, Any?> = mapOf(
        "batteryLevel" to batteryLevel(),
        "wifiSsid" to wifiSsid(),
        "bluetoothDevice" to (connectedBtDevices.firstOrNull() ?: ""),
        "volume" to volume(),
        "brightness" to brightness(),
        "toggles" to mapOf(
            "wifi" to wifiManager().isWifiEnabled,
            "bluetooth" to bluetoothOn(),
            "airplaneMode" to airplaneOn(),
            "microphone" to !audio().isMicrophoneMute,
        ),
        "notifications" to NotifService.lastList,
    )

    private fun wifiManager() =
        applicationContext.getSystemService(WifiManager::class.java)

    private fun audio() = getSystemService(AudioManager::class.java)

    private fun batteryLevel(): Int {
        val bm = getSystemService(BatteryManager::class.java)
        return bm.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY)
            .coerceIn(0, 100)
    }

    private fun wifiSsid(): String? {
        val ssid = wifiManager().connectionInfo?.ssid ?: return null
        val clean = ssid.removeSurrounding("\"")
        return clean.takeIf { it.isNotEmpty() && it != "<unknown ssid>" }
    }

    private fun bluetoothOn(): Boolean {
        val bm = getSystemService(BluetoothManager::class.java)
        return bm?.adapter?.isEnabled == true
    }

    private fun airplaneOn(): Boolean = Settings.Global.getInt(
        contentResolver, Settings.Global.AIRPLANE_MODE_ON, 0,
    ) == 1

    private fun volume(): Double {
        val am = audio()
        val cur = am.getStreamVolume(AudioManager.STREAM_MUSIC)
        val max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC)
        return if (max > 0) cur.toDouble() / max else 0.0
    }

    private fun brightness(): Double {
        val v = Settings.System.getInt(
            contentResolver, Settings.System.SCREEN_BRIGHTNESS, 128,
        )
        return (v / 255.0).coerceIn(0.0, 1.0)
    }

    private fun setVolume(v: Double) {
        val am = audio()
        val max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC)
        am.setStreamVolume(
            AudioManager.STREAM_MUSIC,
            (v * max).toInt().coerceIn(0, max),
            0,
        )
    }

    private fun setBrightness(v: Double) {
        if (Settings.System.canWrite(this)) {
            Settings.System.putInt(
                contentResolver,
                Settings.System.SCREEN_BRIGHTNESS,
                (v * 255).toInt().coerceIn(0, 255),
            )
        } else {
            // No system write access: dim our own window instead.
            val attrs = window.attributes
            attrs.screenBrightness = v.toFloat().coerceIn(0f, 1f)
            window.attributes = attrs
        }
    }

    private fun requestToggle(id: String, on: Boolean) {
        when (id) {
            "wifi", "bluetooth", "airplaneMode" -> {
                // Radios can't be flipped programmatically on Android 10+;
                // open the system internet panel for the user.
                if (Build.VERSION.SDK_INT >= 29) {
                    startActivity(Intent(Settings.Panel.ACTION_INTERNET_CONNECTIVITY))
                } else {
                    startActivity(Intent(Settings.ACTION_WIRELESS_SETTINGS))
                }
            }
            "microphone" -> audio().isMicrophoneMute = !on
            "doNotDisturb" ->
                startActivity(Intent(Settings.ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS))
            "batterySaver" ->
                startActivity(Intent(Settings.ACTION_BATTERY_SAVER_SETTINGS))
            "nightMode" ->
                startActivity(Intent(Settings.ACTION_DISPLAY_SETTINGS))
            in seamToggles -> sendBroadcast(
                Intent("gitlab.neosalsa.quicksettings.TOGGLE")
                    .setPackage(packageName)
                    .putExtra("id", id)
                    .putExtra("on", on),
            )
        }
    }

    private fun performAction(id: String) {
        when (id) {
            "openSettings" -> startActivity(Intent(Settings.ACTION_SETTINGS))
            "aboutDevice" -> startActivity(Intent(Settings.ACTION_DEVICE_INFO_SETTINGS))
            "resetView" -> sendBroadcast(
                Intent("gitlab.neosalsa.quicksettings.RECENTER")
                    .setPackage(packageName),
            )
            "reportProblem" -> sendBroadcast(
                Intent("gitlab.neosalsa.quicksettings.REPORT")
                    .setPackage(packageName),
            )
        }
    }
}
