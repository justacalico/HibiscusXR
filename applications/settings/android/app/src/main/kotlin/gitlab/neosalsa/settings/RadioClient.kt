package gitlab.neosalsa.settings

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.wifi.ScanResult
import android.net.wifi.WifiConfiguration
import android.net.wifi.WifiManager
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.view.inputmethod.InputMethodManager

private const val TAG = "SettingsRadios"

// Everything the wifi + bluetooth + keyboard sections need from the
// platform. The app is platform signed and privileged, so the
// deprecated management APIs (addNetwork/enableNetwork/removeNetwork/
// startScan, adapter discovery) still answer - that is what lets the
// sections stay in-app instead of bouncing to com.android.settings,
// which renders as a mono window and breaks the panel.
class RadioClient(private val appCtx: Context) {
    var onChange: ((Map<String, Any?>) -> Unit)? = null

    private val main = Handler(Looper.getMainLooper())
    private val discovered = LinkedHashMap<String, String>()
    private val aclLinked = HashSet<String>()

    @Volatile
    var wifiScanning = false
        private set

    @Volatile
    var btDiscovering = false
        private set

    private val wifiRx = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            when (intent.action) {
                WifiManager.SCAN_RESULTS_AVAILABLE_ACTION -> {
                    wifiScanning = false
                    pushWifi()
                }
                WifiManager.NETWORK_STATE_CHANGED_ACTION -> pushWifi()
            }
        }
    }

    private val btRx = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            when (intent.action) {
                BluetoothAdapter.ACTION_DISCOVERY_STARTED -> {
                    discovered.clear()
                    btDiscovering = true
                    pushBt()
                }
                BluetoothAdapter.ACTION_DISCOVERY_FINISHED -> {
                    btDiscovering = false
                    pushBt()
                }
                BluetoothDevice.ACTION_FOUND -> {
                    @Suppress("DEPRECATION")
                    val dev = intent.getParcelableExtra<BluetoothDevice>(
                        BluetoothDevice.EXTRA_DEVICE,
                    ) ?: return
                    val name = intent.getStringExtra(BluetoothDevice.EXTRA_NAME)
                        ?: safeName(dev)
                        ?: dev.address
                    discovered[dev.address] = name
                    pushBt()
                }
                BluetoothDevice.ACTION_BOND_STATE_CHANGED -> {
                    @Suppress("DEPRECATION")
                    val dev = intent.getParcelableExtra<BluetoothDevice>(
                        BluetoothDevice.EXTRA_DEVICE,
                    ) ?: return
                    val state = intent.getIntExtra(
                        BluetoothDevice.EXTRA_BOND_STATE,
                        BluetoothDevice.BOND_NONE,
                    )
                    if (state == BluetoothDevice.BOND_BONDED) {
                        discovered.remove(dev.address)
                    }
                    pushBt()
                }
                BluetoothDevice.ACTION_ACL_CONNECTED -> {
                    @Suppress("DEPRECATION")
                    intent.getParcelableExtra<BluetoothDevice>(
                        BluetoothDevice.EXTRA_DEVICE,
                    )?.let { aclLinked.add(it.address) }
                    pushBt()
                }
                BluetoothDevice.ACTION_ACL_DISCONNECTED -> {
                    @Suppress("DEPRECATION")
                    intent.getParcelableExtra<BluetoothDevice>(
                        BluetoothDevice.EXTRA_DEVICE,
                    )?.let { aclLinked.remove(it.address) }
                    pushBt()
                }
            }
        }
    }

    fun register() {
        appCtx.registerReceiver(
            wifiRx,
            IntentFilter().apply {
                addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION)
                addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION)
            },
        )
        appCtx.registerReceiver(
            btRx,
            IntentFilter().apply {
                addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED)
                addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED)
                addAction(BluetoothDevice.ACTION_FOUND)
                addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED)
                addAction(BluetoothDevice.ACTION_ACL_CONNECTED)
                addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED)
            },
        )
        btDiscovering = adapter()?.isDiscovering == true
    }

    fun unregister() {
        try { appCtx.unregisterReceiver(wifiRx) } catch (_: Throwable) {}
        try { appCtx.unregisterReceiver(btRx) } catch (_: Throwable) {}
    }

    // Snapshot fragment merged into "load" and pushed as the event
    // payload after every radio change.
    fun snapshot(): Map<String, Any?> = mapOf(
        "wifi" to wifiList(),
        "wifiScan" to wifiScanning,
        "bt" to btList(),
        "btScan" to btDiscovering,
        "imes" to imeList(),
    )

    private fun pushWifi() {
        onChange?.invoke(mapOf("wifi" to wifiList(), "wifiScan" to wifiScanning))
    }

    private fun pushBt() {
        onChange?.invoke(mapOf("bt" to btList(), "btScan" to btDiscovering))
    }

    private fun wifi() = appCtx.getSystemService(WifiManager::class.java)

    private fun adapter(): BluetoothAdapter? =
        appCtx.getSystemService(BluetoothManager::class.java)?.adapter

    // ---------------------------------------------------------- wifi

    private fun savedBySsid(): Map<String, WifiConfiguration> = try {
        @Suppress("DEPRECATION")
        wifi()?.configuredNetworks
            ?.associateBy { it.SSID.removeSurrounding("\"") }
            ?: emptyMap()
    } catch (_: Throwable) {
        emptyMap()
    }

    @SuppressLint("MissingPermission")
    private fun wifiList(): List<Map<String, Any?>> {
        val wm = wifi() ?: return emptyList()
        val saved = savedBySsid()
        val connSsid = try {
            wm.connectionInfo?.ssid?.removeSurrounding("\"")
        } catch (_: Throwable) {
            null
        }
        val seen = HashSet<String>()
        val results = try {
            wm.scanResults ?: emptyList<ScanResult>()
        } catch (_: Throwable) {
            emptyList()
        }
        // one row per ssid, strongest signal wins
        return results
            .sortedByDescending { it.level }
            .mapNotNull { r ->
                val ssid = r.SSID ?: return@mapNotNull null
                if (ssid.isEmpty() || !seen.add(ssid)) return@mapNotNull null
                mapOf(
                    "ssid" to ssid,
                    "caps" to (r.capabilities ?: ""),
                    "level" to WifiManager.calculateSignalLevel(r.level, 5),
                    "connected" to (ssid == connSsid),
                    "saved" to (saved[ssid]?.networkId ?: -1),
                )
            }
    }

    @SuppressLint("MissingPermission")
    fun scanWifi() {
        val wm = wifi() ?: return
        if (!wm.isWifiEnabled) return
        try {
            @Suppress("DEPRECATION")
            if (wm.startScan()) {
                wifiScanning = true
                // scan-results broadcasts are not guaranteed; drop the
                // flag anyway so the row can't spin forever
                main.postDelayed({
                    if (wifiScanning) {
                        wifiScanning = false
                        pushWifi()
                    }
                }, 8_000)
            }
        } catch (_: Throwable) {}
        pushWifi()
    }

    // Saved configurations join straight away; a new one is written
    // from the security choice + the password the dialog collected.
    @Suppress("DEPRECATION")
    @SuppressLint("MissingPermission")
    fun connectWifi(ssid: String, security: String, password: String) {
        val wm = wifi() ?: return
        val existing = savedBySsid()[ssid]
        val netId = if (existing != null) {
            existing.networkId
        } else {
            val conf = WifiConfiguration().apply {
                SSID = "\"$ssid\""
                when (security) {
                    "wep" -> {
                        allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE)
                        allowedAuthAlgorithms.set(
                            WifiConfiguration.AuthAlgorithm.OPEN,
                        )
                        allowedAuthAlgorithms.set(
                            WifiConfiguration.AuthAlgorithm.SHARED,
                        )
                        wepKeys[0] = "\"$password\""
                        wepTxKeyIndex = 0
                    }
                    "open" -> {
                        allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE)
                    }
                    else -> {
                        allowedKeyManagement.set(
                            WifiConfiguration.KeyMgmt.WPA_PSK,
                        )
                        if (password.isNotEmpty()) {
                            preSharedKey = "\"$password\""
                        }
                    }
                }
            }
            val id = wm.addNetwork(conf)
            if (id < 0) {
                android.util.Log.w(TAG, "addNetwork refused for $ssid")
                return
            }
            id
        }
        wm.disconnect()
        wm.enableNetwork(netId, true)
        wm.reconnect()
        pushWifi()
    }

    @Suppress("DEPRECATION")
    @SuppressLint("MissingPermission")
    fun forgetWifi(netId: Int) {
        val wm = wifi() ?: return
        if (netId < 0) return
        wm.removeNetwork(netId)
        pushWifi()
    }

    // ---------------------------------------------------------- bt

    private fun safeName(dev: BluetoothDevice): String? = try {
        dev.name
    } catch (_: Throwable) {
        null
    }

    @SuppressLint("MissingPermission")
    private fun btList(): List<Map<String, Any?>> {
        val out = mutableListOf<Map<String, Any?>>()
        try {
            adapter()?.bondedDevices?.forEach { d ->
                out += mapOf(
                    "name" to (safeName(d) ?: d.address),
                    "address" to d.address,
                    "bonded" to true,
                    "connected" to aclLinked.contains(d.address),
                )
            }
        } catch (_: Throwable) {}
        discovered.forEach { (addr, name) ->
            out += mapOf(
                "name" to name,
                "address" to addr,
                "bonded" to false,
                "connected" to false,
            )
        }
        return out
    }

    @SuppressLint("MissingPermission")
    fun scanBt() {
        val a = adapter() ?: return
        if (!a.isEnabled) return
        try {
            if (a.isDiscovering) a.cancelDiscovery()
            discovered.clear()
            // ACTION_DISCOVERY_STARTED does the state push
            if (!a.startDiscovery()) {
                btDiscovering = false
                pushBt()
            }
        } catch (_: Throwable) {
            btDiscovering = false
            pushBt()
        }
    }

    @SuppressLint("MissingPermission")
    fun pairBt(address: String) {
        try {
            adapter()?.getRemoteDevice(address)?.createBond()
        } catch (_: Throwable) {}
    }

    // removeBond is @hide; gitlab.neosalsa.settings is on the hidden
    // api blacklist exemption list (pn2-home.rc) so the call resolves.
    fun unpairBt(address: String) {
        try {
            val dev = adapter()?.getRemoteDevice(address) ?: return
            dev.javaClass.getMethod("removeBond").invoke(dev)
        } catch (_: Throwable) {}
    }

    // ---------------------------------------------------------- ime

    private fun imeList(): List<Map<String, Any?>> {
        val imm = appCtx.getSystemService(InputMethodManager::class.java)
            ?: return emptyList()
        val active = Settings.Secure.getString(
            appCtx.contentResolver,
            Settings.Secure.DEFAULT_INPUT_METHOD,
        )
        return imm.inputMethodList.map { info ->
            mapOf(
                "id" to info.id,
                "label" to info.loadLabel(appCtx.packageManager).toString(),
                "active" to (info.id == active),
            )
        }
    }

    fun setIme(id: String) {
        try {
            Settings.Secure.putString(
                appCtx.contentResolver,
                Settings.Secure.DEFAULT_INPUT_METHOD,
                id,
            )
        } catch (_: SecurityException) {}
        onChange?.invoke(mapOf("imes" to imeList()))
    }
}
