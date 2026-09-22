package gitlab.neosalsa.settings

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.RemoteException
import android.util.Log
import com.picovr.picovrlib.cvcontrollerlib.CVControllerAIDLService
import com.picovr.picovrlib.cvcontrollerlib.ICVAIDLServiceCallback

// Thin binding to the stock controller daemon (CVService.apk,
// package com.picovr.picovrlib.cvcontroller). It owns the RF link to
// both hand controllers; pairing, link state and battery all go
// through this binder.
class ControllerClient(private val context: Context) {
    companion object {
        private const val TAG = "ControllerClient"
        private const val ACTION =
            "com.picovr.picovrlib.cvcontrollerlib.CVControllerAIDLService"
        private const val PKG = "com.picovr.picovrlib.cvcontroller"

        // Service reads the controller index, 0 = left, 1 = right.
        const val CONTROLLER_LEFT = 0
        const val CONTROLLER_RIGHT = 1

        // Reported as soon as we bind; any version >= 2.8.0.0 flips the
        // service into per-controller (CV2) reporting.
        private const val CLIENT_VERSION = "3.0.0.0"

        // Sensor mode args for the SPI worker thread. Stock PUI starts
        // the thread with both sensors on (1,1); with head=0 the station
        // only ran partial tracking and pairing never found controllers.
        private const val HEAD_SENSOR = 1
        private const val HAND_SENSOR = 1

        private const val POLL_MS = 2000L
        private const val BATTERY_INDEX = 8

        // The station keeps scanning after the one-shot pair command;
        // getStationPairState only reports the command result, so the
        // open window is tracked here and expires on its own.
        private const val SCAN_TIMEOUT_MS = 60_000L
    }

    var service: CVControllerAIDLService? = null
        private set
    var bound = false
        private set

    // Pushed state, all written on the main thread by poll()/callbacks.
    val states = intArrayOf(0, 0)
    val batteries = intArrayOf(-1, -1)
    val charging = booleanArrayOf(false, false)
    val macs = arrayOf("", "")
    val serials = arrayOf("", "")
    var pairState = 0
    var scanning = false
        private set
    // SPI worker is only safe to query once the service reports it
    // started; calls before that lazily create SpiSensor on a binder
    // thread and crash the service (no app classloader there).
    var threadReady = false
        private set

    val pairingActive: Boolean
        get() = scanning || pairState > 0

    var onChange: (() -> Unit)? = null

    private val handler = Handler(Looper.getMainLooper())
    private val poll = object : Runnable {
        override fun run() {
            poll()
            handler.postDelayed(this, POLL_MS)
        }
    }

    private val scanTimeout = Runnable {
        Log.i(TAG, "scan window expired")
        scanning = false
        poll()
        onChange?.invoke()
    }

    private val connection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName, binder: IBinder) {
            service = CVControllerAIDLService.Stub.asInterface(binder)
            bound = true
            threadReady = false
            Log.i(TAG, "bound, iface=${service != null}")
            try {
                service?.setUnityVersion(CLIENT_VERSION)
                Log.i(TAG, "setUnityVersion($CLIENT_VERSION) sent")
                service?.registerCallback(callback)
                // The SPI link to the RF station only exists while this
                // worker thread runs; pairing/state calls no-op without it.
                service?.startCVControllerThread(HEAD_SENSOR, HAND_SENSOR)
                Log.i(TAG, "startCVControllerThread sent")
            } catch (e: RemoteException) {
                Log.w(TAG, "service init failed", e)
            }
            onChange?.invoke()
        }

        override fun onServiceDisconnected(name: ComponentName) {
            Log.i(TAG, "service disconnected")
            service = null
            bound = false
            threadReady = false
            handler.removeCallbacks(poll)
            onChange?.invoke()
        }
    }

    // Any feedback callback means the SPI worker is alive, so this also
    // covers binding after the thread-started broadcast already fired.
    private fun markThreadReady() {
        if (threadReady) return
        threadReady = true
        handler.post {
            try {
                for (i in 0..1) {
                    service?.getDeviceBleMac(i + 1)
                    service?.getControllerSn(i)
                }
            } catch (e: RemoteException) {
                Log.w(TAG, "id query failed", e)
            }
            poll()
            handler.removeCallbacks(poll)
            handler.postDelayed(poll, POLL_MS)
            onChange?.invoke()
        }
    }

    private val callback = object : ICVAIDLServiceCallback.Stub() {
        override fun feedbackConnectStatus(controller: Int, status: Int) {
            Log.i(TAG, "connect cb: controller=$controller status=$status")
            markThreadReady()
            if (controller in 0..1) states[controller] = status
            handler.post {
                if (status == 1 && controller in 0..1) {
                    // Fresh link: identity and battery arrive async.
                    try {
                        service?.getDeviceBleMac(controller + 1)
                        service?.getControllerSn(controller)
                    } catch (e: RemoteException) {
                        Log.w(TAG, "id query failed", e)
                    }
                }
                poll()
                onChange?.invoke()
            }
        }

        override fun feedbackControllerUnbind(status: Int) {
            Log.i(TAG, "unbind cb: status=$status")
            handler.post { onChange?.invoke() }
        }

        override fun feedbackControllerDeviceBleMac(device: Int, mac: String) {
            // Same numbering as getDeviceBleMac: 1 = left, 2 = right.
            Log.i(TAG, "mac cb: device=$device mac=$mac")
            markThreadReady()
            if (device in 1..2) macs[device - 1] = mac
            handler.post { onChange?.invoke() }
        }

        override fun feedbackControllerControllerSn(device: Int, sn: String) {
            Log.i(TAG, "sn cb: device=$device sn=$sn")
            markThreadReady()
            if (device in 0..1) serials[device] = sn
            handler.post { onChange?.invoke() }
        }

        override fun feedbackDeviceInfo(i1: String?, i2: String?, f: Int) {
            Log.i(TAG, "device info cb: $i1 $i2 flag=$f")
            markThreadReady()
        }
        override fun feedbackMainControllerSerialNumChanged(i: Int) {
            Log.i(TAG, "main controller cb: $i")
            markThreadReady()
        }
        override fun feedbackControllerThreadStarted() {
            Log.i(TAG, "controller thread started")
            markThreadReady()
        }
        override fun feedbackControllerDeviceVersion(d: Int, v: String?) {}
        override fun feedbackControllerStatus(s: Int) {
            Log.i(TAG, "controller status cb: $s")
            markThreadReady()
            // 2 is the station confirming it entered pair scan.
            if (s == 2) {
                scanning = true
                handler.removeCallbacks(scanTimeout)
                handler.postDelayed(scanTimeout, SCAN_TIMEOUT_MS)
                handler.post { onChange?.invoke() }
            }
        }
        override fun feedbackControllerBusyStatus(s: Int) {}
        override fun feedbackControllerOTAStatusCode(d: Int, c: Int) {}
        override fun feedbackControllerDeviceVersionSN(d: Int, v: String?) {}
        override fun feedbackControllerUniqueIdentifier(id: String?) {}
        override fun feedbackControllerCombinedKeyUnbind(s: Int) {}
        override fun feedbackStationOTAProgress(p: Int) {}
        override fun feedbackStationOTAErroCode(c: Int) {}
        override fun feedbackControllerOTAProgress(d: Int, p: Int, s: Int) {}
        override fun feedbackControllerOTAErroCode(d: Int, c: Int, s: Int) {}
        override fun feedbackOTAComplete(s: Int) {}
        override fun feedbackControllerBlePacketLossRate(d: Int, r: Float) {}
        override fun feedbackHandNessSerialNumChanged(s: Int) {}
        override fun feedbackDeviceChannel(d: Int, c: Int) {}
        override fun feedbackControllerNDIVersion(d: Int, v: String?) {}
    }

    fun bind() {
        val intent = Intent(ACTION).setPackage(PKG)
        try {
            val ok = context.bindService(
                intent, connection, Context.BIND_AUTO_CREATE,
            )
            Log.i(TAG, "bindService -> $ok")
        } catch (e: Exception) {
            Log.w(TAG, "bind failed", e)
        }
    }

    fun unbind() {
        handler.removeCallbacks(poll)
        handler.removeCallbacks(scanTimeout)
        scanning = false
        if (!bound) return
        try {
            service?.stopCVControllerThread(HEAD_SENSOR, HAND_SENSOR)
            service?.unregisterCallback(callback)
            context.unbindService(connection)
        } catch (e: Exception) {
            Log.w(TAG, "unbind failed", e)
        }
        service = null
        bound = false
        threadReady = false
    }

    fun enterPairMode() {
        val svc = service
        if (svc == null || !threadReady) {
            Log.w(TAG, "enterPairMode: bound=$bound ready=$threadReady")
            return
        }
        try {
            // Opcode 0x14 "scanning mode" opens the station discovery
            // window. Do not also call enterPairMode here: its 0x0d
            // paired-MAC command switches the station out of scanning
            // mode, which is why mixing both never found a controller.
            svc.startPairingMode(0)
            Log.i(TAG, "startPairingMode(0) sent")
            scanning = true
            handler.postDelayed(scanTimeout, SCAN_TIMEOUT_MS)
        } catch (e: RemoteException) {
            Log.w(TAG, "enterPairMode failed", e)
        }
        poll()
    }

    fun interruptPairMode() {
        if (service == null || !threadReady) {
            Log.w(TAG, "interruptPairMode: bound=$bound ready=$threadReady")
            scanning = false
            handler.removeCallbacks(scanTimeout)
            onChange?.invoke()
            return
        }
        try {
            service?.interruptPairMode()
            Log.i(TAG, "interruptPairMode sent")
            // One stop clears the service's needstartpairing latch and
            // closes the station scan window.
            service?.stopPairingMode(0)
            Log.i(TAG, "stopPairingMode(0) sent")
        } catch (e: RemoteException) {
            Log.w(TAG, "interruptPairMode failed", e)
        }
        scanning = false
        handler.removeCallbacks(scanTimeout)
        poll()
    }

    // Raw probe for the station scan command; the service latches
    // needstartpairing so stop first to clear it, then start fresh.
    fun scanRaw(device: Int) {
        val svc = service ?: return
        if (!threadReady) {
            Log.w(TAG, "scanRaw: thread not ready")
            return
        }
        try {
            svc.stopPairingMode(device)
            svc.startPairingMode(device)
            Log.i(TAG, "scanRaw device=$device sent")
        } catch (e: RemoteException) {
            Log.w(TAG, "scanRaw failed", e)
        }
    }

    // Push known BLE MACs into the station bond table (opcode 0x11).
    fun usbPair(mac1: String, mac2: String) {
        val svc = service ?: return
        if (!threadReady) {
            Log.w(TAG, "usbPair: thread not ready")
            return
        }
        try {
            svc.enterUSBPairMode(mac1, mac2)
            Log.i(TAG, "enterUSBPairMode mac1=$mac1 mac2=$mac2 sent")
        } catch (e: RemoteException) {
            Log.w(TAG, "usbPair failed", e)
        }
    }

    // Dumps the station bond table; the MAC list lands in the
    // service's own log (getStationWhiteList buf_rx).
    fun whiteList() {
        val svc = service ?: return
        if (!threadReady) {
            Log.w(TAG, "whiteList: thread not ready")
            return
        }
        try {
            svc.getStationWhiteListNumber()
            Log.i(TAG, "getStationWhiteListNumber sent")
        } catch (e: RemoteException) {
            Log.w(TAG, "whiteList failed", e)
        }
    }

    // Paired-scan probe (opcode 0x0d), reconnects a stored bond for a slot.
    fun enterPair(slot: Int) {
        val svc = service ?: return
        if (!threadReady) {
            Log.w(TAG, "enterPair: thread not ready")
            return
        }
        try {
            svc.enterPairMode(slot)
            Log.i(TAG, "enterPairMode($slot) sent")
        } catch (e: RemoteException) {
            Log.w(TAG, "enterPair failed", e)
        }
    }

    fun unbindAll() {
        if (!threadReady) return
        try {
            for (i in 0..1) service?.setControllerUnbind(i)
            Log.i(TAG, "setControllerUnbind sent for both slots")
        } catch (e: RemoteException) {
            Log.w(TAG, "setControllerUnbind failed", e)
        }
        poll()
    }

    fun poll() {
        val svc = service ?: return
        if (!threadReady) return
        try {
            pairState = svc.stationPairState
            for (i in 0..1) {
                states[i] = svc.getCV2ControllerConnectionState(i)
                val keys = svc.getControllerKeyEvent(i)
                batteries[i] =
                    if (keys != null && keys.size > BATTERY_INDEX) {
                        keys[BATTERY_INDEX]
                    } else {
                        -1
                    }
                charging[i] = svc.isChargeing(i)
            }
            if (scanning && states[0] == 1 && states[1] == 1) {
                Log.i(TAG, "both slots linked, closing scan window")
                scanning = false
                handler.removeCallbacks(scanTimeout)
            }
            Log.i(
                TAG,
                "poll pair=$pairState scan=$scanning " +
                    "L{st=${states[0]} bat=${batteries[0]} " +
                    "chg=${charging[0]} mac=${macs[0]} sn=${serials[0]}} " +
                    "R{st=${states[1]} bat=${batteries[1]} " +
                    "chg=${charging[1]} mac=${macs[1]} sn=${serials[1]}}",
            )
        } catch (e: RemoteException) {
            Log.w(TAG, "poll failed", e)
        }
    }
}
