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

// android.media.AudioManager.VOLUME_CHANGED_ACTION is @hide
private const val VOLUME_CHANGED = "android.media.VOLUME_CHANGED_ACTION"
private const val TAG = "SettingsMain"

// Project-owned keys. qvrd / the shell read these through the same seam
// the quick panel broadcasts on.
private const val KEY_TRACKING_ENABLED = "pn2_tracking_enabled"
private const val KEY_TRACKING_FREQ = "pn2_tracking_frequency"
private const val KEY_SEETHROUGH = "pn2_seethrough"
private const val KEY_BOUNDARY = "pn2_boundary"

class MainActivity : FlutterActivity() {
    private var eventSink: EventChannel.EventSink? = null
    private var controllers: ControllerClient? = null

    // adb-triggerable scan toggle, same path as tapping the card.
    private val scanReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            android.util.Log.i(TAG, "scan broadcast received")
            performAction("controllerPair")
        }
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
                    "selectChoice" -> {
                        selectChoice(
                            call.argument<String>("id") ?: "",
                            call.argument<String>("value") ?: "",
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
            "trackingToggle" to globalOn(KEY_TRACKING_ENABLED, true),
            "boundary" to globalOn(KEY_BOUNDARY, true),
            "seethrough" to globalOn(KEY_SEETHROUGH, false),
            "nightMode" to nightModeOn(),
            "controllerPair" to (controllers?.pairingActive ?: false),
        ),
        "sliders" to mapOf(
            "volume" to volume(),
            "brightness" to brightness(),
        ),
        "choices" to mapOf(
            "trackingFrequency" to
                globalStr(KEY_TRACKING_FREQ, "auto"),
            "controllerMain" to
                if (controllers?.mainController ==
                    ControllerClient.CONTROLLER_LEFT
                ) {
                    "left"
                } else {
                    "right"
                },
        ),
        "texts" to mapOf(
            "wifiSsid" to (wifiSsid() ?: ""),
            "modelName" to Build.MODEL,
            "androidVersion" to Build.VERSION.RELEASE,
            "buildNumber" to Build.DISPLAY,
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
            "choices" to mapOf(
                "controllerMain" to
                    if (c?.mainController == ControllerClient.CONTROLLER_LEFT) {
                        "left"
                    } else {
                        "right"
                    },
            ),
        )
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

    private fun globalOn(key: String, def: Boolean): Boolean =
        Settings.Global.getInt(
            contentResolver, key, if (def) 1 else 0,
        ) == 1

    private fun globalStr(key: String, def: String): String =
        Settings.Global.getString(contentResolver, key) ?: def

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
            "trackingToggle" -> putGlobal(KEY_TRACKING_ENABLED, on)
            "boundary" -> {
                putGlobal(KEY_BOUNDARY, on)
                seam("boundary", on)
            }
            "seethrough" -> {
                putGlobal(KEY_SEETHROUGH, on)
                seam("seethrough", on)
            }
        }
    }

    private fun selectChoice(id: String, value: String) {
        if (id == "trackingFrequency") {
            Settings.Global.putString(
                contentResolver, KEY_TRACKING_FREQ, value,
            )
        }
        if (id == "controllerMain") {
            controllers?.setMain(
                if (value == "left") {
                    ControllerClient.CONTROLLER_LEFT
                } else {
                    ControllerClient.CONTROLLER_RIGHT
                },
            )
        }
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
            "backupNow" ->
                startActivity(Intent(Settings.ACTION_PRIVACY_SETTINGS))
            "devOptions" -> startActivity(
                Intent(Settings.ACTION_APPLICATION_DEVELOPMENT_SETTINGS),
            )
            "aboutOpen" ->
                startActivity(Intent(Settings.ACTION_DEVICE_INFO_SETTINGS))
            "resetView" -> sendBroadcast(
                Intent("gitlab.neosalsa.settings.RECENTER")
                    .setPackage(packageName),
            )
            "updateCheck" -> sendBroadcast(
                Intent("gitlab.neosalsa.settings.CHECK_UPDATE")
                    .setPackage(packageName),
            )
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

    private fun putGlobal(key: String, on: Boolean) {
        try {
            Settings.Global.putInt(
                contentResolver, key, if (on) 1 else 0,
            )
        } catch (_: SecurityException) {}
    }

    // Panel-owned toggles the OS layer reacts to; same seam the quick
    // panel uses.
    private fun seam(id: String, on: Boolean) = sendBroadcast(
        Intent("gitlab.neosalsa.settings.TOGGLE")
            .setPackage(packageName)
            .putExtra("id", id)
            .putExtra("on", on),
    )
}
