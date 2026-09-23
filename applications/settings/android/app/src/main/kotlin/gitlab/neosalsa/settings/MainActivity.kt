package gitlab.neosalsa.settings

import android.app.UiModeManager
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.media.AudioManager
import android.net.wifi.WifiManager
import android.os.Build
import android.provider.Settings
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodChannel
import java.io.File

// android.media.AudioManager.VOLUME_CHANGED_ACTION is @hide
private const val VOLUME_CHANGED = "android.media.VOLUME_CHANGED_ACTION"
// Settings.System.SHOW_TOUCHES is @hide too; on Android 10 the key still
// sits in the system table, so the raw name is the way in.
private const val SHOW_TOUCHES = "show_touches"
private const val TAG = "SettingsMain"

class MainActivity : FlutterActivity() {
    private var eventSink: EventChannel.EventSink? = null
    private var controllers: ControllerClient? = null

    // adb-triggerable scan toggle, same path as tapping the card.
    // "device" extra drives a raw startPairingMode probe instead.
    private val scanReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            android.util.Log.i(TAG, "scan broadcast received")
            val dev = intent.getIntExtra("device", -1)
            if (dev >= 0) {
                controllers?.scanRaw(dev)
                return
            }
            val m1 = intent.getStringExtra("mac1")
            val m2 = intent.getStringExtra("mac2")
            if (m1 != null || m2 != null) {
                controllers?.usbPair(m1.orEmpty(), m2.orEmpty())
                return
            }
            val slot = intent.getIntExtra("enterpair", -1)
            if (slot >= 0) {
                controllers?.enterPair(slot)
                return
            }
            if (intent.hasExtra("whitelist")) {
                controllers?.whiteList()
                return
            }
            if (intent.hasExtra("blescan")) {
                bleScanProbe()
                return
            }
            performAction("controllerPair")
        }
    }

    // BLE probe: lists every advertiser for ~20s to check whether a
    // pairing-mode controller is broadcasting at all.
    @Suppress("DEPRECATION")
    private fun bleScanProbe() {
        val adapter = BluetoothAdapter.getDefaultAdapter()
        if (adapter == null || !adapter.isEnabled) {
            android.util.Log.w(TAG, "blescan: bt off")
            return
        }
        val cb = BluetoothAdapter.LeScanCallback { dev, rssi, rec ->
            val name = try { dev.name } catch (e: SecurityException) { null }
            android.util.Log.i(
                TAG,
                "blescan dev=${dev.address} rssi=$rssi name=$name rec=${rec?.size ?: 0}B",
            )
        }
        if (!adapter.startLeScan(cb)) {
            android.util.Log.w(TAG, "blescan: startLeScan refused")
            return
        }
        android.util.Log.i(TAG, "blescan started")
        android.os.Handler(mainLooper).postDelayed({
            adapter.stopLeScan(cb)
            android.util.Log.i(TAG, "blescan stopped")
        }, 40_000)
    }

    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val update = mutableMapOf<String, Any?>()
            when (intent.action) {
                WifiManager.WIFI_STATE_CHANGED_ACTION,
                WifiManager.NETWORK_STATE_CHANGED_ACTION -> {
                    update["toggles"] = mapOf("wifiToggle" to wifiOn())
                    update["texts"] = mapOf("wifiSsid" to (wifiSsid() ?: ""))
                }
                BluetoothAdapter.ACTION_STATE_CHANGED ->
                    update["toggles"] =
                        mapOf("bluetoothToggle" to bluetoothOn())
                VOLUME_CHANGED ->
                    update["sliders"] = mapOf("volume" to volume())
            }
            if (update.isNotEmpty()) eventSink?.success(update)
        }
    }

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        controllers = ControllerClient(applicationContext).also { c ->
            c.onChange = { eventSink?.success(controllerSnapshot()) }
            c.bind()
        }
        registerReceiver(
            scanReceiver,
            IntentFilter("gitlab.neosalsa.settings.CONTROLLER_SCAN"),
        )
        val messenger = flutterEngine.dartExecutor.binaryMessenger
        MethodChannel(messenger, "gitlab.neosalsa.settings/system")
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "load" -> result.success(snapshot())
                    "setSlider" -> {
                        setSlider(
                            call.argument<String>("id") ?: "",
                            call.argument<Double>("value") ?: 0.5,
                        )
                        result.success(null)
                    }
                    "requestToggle" -> {
                        requestToggle(
                            call.argument<String>("id") ?: "",
                            call.argument<Boolean>("on") ?: false,
                        )
                        result.success(null)
                    }
                    "performAction" -> {
                        performAction(call.argument<String>("id") ?: "")
                        result.success(null)
                    }
                    else -> result.notImplemented()
                }
            }
        EventChannel(messenger, "gitlab.neosalsa.settings/events")
            .setStreamHandler(object : EventChannel.StreamHandler {
                override fun onListen(arguments: Any?, sink: EventChannel.EventSink) {
                    eventSink = sink
                    registerReceiver(
                        receiver,
                        IntentFilter().apply {
                            addAction(WifiManager.WIFI_STATE_CHANGED_ACTION)
                            addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION)
                            addAction(BluetoothAdapter.ACTION_STATE_CHANGED)
                            addAction(VOLUME_CHANGED)
                        },
                    )
                }

                override fun onCancel(arguments: Any?) {
                    eventSink = null
                    unregisterReceiver(receiver)
                }
            })
    }

    override fun onDestroy() {
        unregisterReceiver(scanReceiver)
        controllers?.unbind()
        controllers = null
        super.onDestroy()
    }

    private fun snapshot(): Map<String, Any?> = controllerSnapshot() + mapOf(
        "toggles" to mapOf(
            "wifiToggle" to wifiOn(),
            "bluetoothToggle" to bluetoothOn(),
            "micMute" to !audio().isMicrophoneMute,
            "nightMode" to nightModeOn(),
            "adbToggle" to
                (Settings.Global.getInt(
                    contentResolver, Settings.Global.ADB_ENABLED, 0,
                ) == 1),
            "stayAwake" to
                (Settings.Global.getInt(
                    contentResolver,
                    Settings.Global.STAY_ON_WHILE_PLUGGED_IN, 0,
                ) != 0),
            "showTouches" to
                (Settings.System.getInt(
                    contentResolver, SHOW_TOUCHES, 0,
                ) == 1),
            "controllerPair" to (controllers?.pairingActive ?: false),
        ),
        "sliders" to mapOf(
            "volume" to volume(),
            "brightness" to brightness(),
        ),
        "texts" to mapOf(
            "wifiSsid" to (wifiSsid() ?: ""),
            "modelName" to Build.MODEL,
            "androidVersion" to Build.VERSION.RELEASE,
            "hibiscusVersion" to hibiscusVersion(),
        ),
    )

    // Controller state rides the same snapshot shape as the rest of the
    // app: a per-slot map plus the pairing flag and main-hand choice.
    private fun controllerSnapshot(): Map<String, Any?> {
        val c = controllers
        val pairing = c?.pairingActive ?: false
        fun slot(i: Int) = mapOf(
            // A slot that has not linked shows "pairing" while the
            // station scan is open.
            "state" to (c?.states?.get(i) ?: 0).let {
                if (it != 1 && pairing) 2 else it
            },
            "battery" to (c?.batteries?.get(i) ?: -1),
            "charging" to (c?.charging?.get(i) ?: false),
            "mac" to (c?.macs?.get(i) ?: ""),
            "serial" to (c?.serials?.get(i) ?: ""),
        )
        return mapOf(
            "controllers" to mapOf(
                "controllerLeft" to slot(ControllerClient.CONTROLLER_LEFT),
                "controllerRight" to slot(ControllerClient.CONTROLLER_RIGHT),
            ),
            "toggles" to mapOf("controllerPair" to (c?.pairingActive ?: false)),
        )
    }

    // Stamped into the image by system/dist/scripts/write-version.sh.
    // A plain file, not a prop: apps cannot read custom ro.* props on
    // Android 10 without a declared property context.
    private fun hibiscusVersion(): String = try {
        File("/system/etc/hibiscus-release").readText().trim()
    } catch (_: Exception) {
        ""
    }

    private fun wifiManager() =
        applicationContext.getSystemService(WifiManager::class.java)

    private fun audio() = getSystemService(AudioManager::class.java)

    private fun wifiOn() = wifiManager()?.isWifiEnabled == true

    private fun bluetoothOn(): Boolean =
        getSystemService(BluetoothManager::class.java)
            ?.adapter?.isEnabled == true

    private fun wifiSsid(): String? {
        val ssid = wifiManager()?.connectionInfo?.ssid ?: return null
        val clean = ssid.removeSurrounding("\"")
        return clean.takeIf { it.isNotEmpty() && it != "<unknown ssid>" }
    }

    private fun volume(): Double {
        val am = audio()
        val max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC)
        return if (max > 0) {
            am.getStreamVolume(AudioManager.STREAM_MUSIC)
                .toDouble() / max
        } else {
            0.0
        }
    }

    private fun brightness(): Double {
        val v = Settings.System.getInt(
            contentResolver, Settings.System.SCREEN_BRIGHTNESS, 128,
        )
        return (v / 255.0).coerceIn(0.0, 1.0)
    }

    private fun nightModeOn(): Boolean =
        getSystemService(UiModeManager::class.java)?.nightMode ==
            UiModeManager.MODE_NIGHT_YES

    private fun setSlider(id: String, v: Double) {
        when (id) {
            "volume" -> {
                val am = audio()
                val max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC)
                am.setStreamVolume(
                    AudioManager.STREAM_MUSIC,
                    (v * max).toInt().coerceIn(0, max),
                    0,
                )
            }
            "brightness" -> {
                if (Settings.System.canWrite(this)) {
                    Settings.System.putInt(
                        contentResolver,
                        Settings.System.SCREEN_BRIGHTNESS,
                        (v * 255).toInt().coerceIn(0, 255),
                    )
                } else {
                    val attrs = window.attributes
                    attrs.screenBrightness = v.toFloat().coerceIn(0f, 1f)
                    window.attributes = attrs
                }
            }
        }
    }

    @Suppress("DEPRECATION")
    private fun requestToggle(id: String, on: Boolean) {
        when (id) {
            "wifiToggle" -> {
                try {
                    wifiManager()?.isWifiEnabled = on
                } catch (_: SecurityException) {
                    startActivity(
                        Intent(Settings.Panel.ACTION_INTERNET_CONNECTIVITY),
                    )
                }
            }
            "bluetoothToggle" -> {
                val adapter =
                    getSystemService(BluetoothManager::class.java)?.adapter
                try {
                    if (on) adapter?.enable() else adapter?.disable()
                } catch (_: SecurityException) {
                    startActivity(Intent(Settings.ACTION_BLUETOOTH_SETTINGS))
                }
            }
            "micMute" -> audio().isMicrophoneMute = !on
            "nightMode" -> {
                try {
                    getSystemService(UiModeManager::class.java)?.nightMode =
                        if (on) {
                            UiModeManager.MODE_NIGHT_YES
                        } else {
                            UiModeManager.MODE_NIGHT_NO
                        }
                } catch (_: SecurityException) {}
            }
            "adbToggle" ->
                putGlobalInt(Settings.Global.ADB_ENABLED, if (on) 1 else 0)
            "stayAwake" ->
                putGlobalInt(
                    Settings.Global.STAY_ON_WHILE_PLUGGED_IN,
                    if (on) 3 else 0,
                )
            "showTouches" -> {
                if (Settings.System.canWrite(this)) {
                    Settings.System.putInt(
                        contentResolver, SHOW_TOUCHES,
                        if (on) 1 else 0,
                    )
                }
            }
        }
    }

    private fun putGlobalInt(key: String, v: Int) {
        try {
            Settings.Global.putInt(contentResolver, key, v)
        } catch (_: SecurityException) {}
    }

    private fun performAction(id: String) {
        when (id) {
            "wifiSettings" ->
                startActivity(Intent(Settings.ACTION_WIFI_SETTINGS))
            "bluetoothSettings" ->
                startActivity(Intent(Settings.ACTION_BLUETOOTH_SETTINGS))
            "languagePicker" ->
                startActivity(Intent(Settings.ACTION_LOCALE_SETTINGS))
            "timeZone" ->
                startActivity(Intent(Settings.ACTION_DATE_SETTINGS))
            "keyboardPicker" ->
                startActivity(Intent(Settings.ACTION_INPUT_METHOD_SETTINGS))
            "controllerPair" -> {
                val c = controllers
                if (c == null) {
                    android.util.Log.w(TAG, "scan tapped with no client")
                } else {
                    android.util.Log.i(
                        TAG,
                        "scan tapped: bound=${c.bound} " +
                            "pairState=${c.pairState} scanning=${c.scanning}",
                    )
                    if (c.pairingActive) c.interruptPairMode()
                    else c.enterPairMode()
                }
            }
            "controllerUnbind" -> controllers?.unbindAll()
        }
    }
}
