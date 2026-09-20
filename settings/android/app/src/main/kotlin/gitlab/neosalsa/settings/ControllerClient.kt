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

        private const val POLL_MS = 2000L
        private const val BATTERY_INDEX = 8
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
    var mainController = -1

    var onChange: (() -> Unit)? = null

    private val handler = Handler(Looper.getMainLooper())
    private val poll = object : Runnable {
        override fun run() {
            poll()
            handler.postDelayed(this, POLL_MS)
        }
    }

    private val connection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName, binder: IBinder) {
            service = CVControllerAIDLService.Stub.asInterface(binder)
            bound = true
            try {
                service?.setUnityVersion(CLIENT_VERSION)
                service?.registerCallback(callback)
                // These answer through the callback, not the return value.
                for (i in 0..1) {
                    service?.getDeviceBleMac(i)
                    service?.getControllerSn(i)
                }
            } catch (e: RemoteException) {
                Log.w(TAG, "service init failed", e)
            }
            poll()
            handler.postDelayed(poll, POLL_MS)
            onChange?.invoke()
        }

        override fun onServiceDisconnected(name: ComponentName) {
            service = null
            bound = false
            handler.removeCallbacks(poll)
            onChange?.invoke()
        }
    }

    private val callback = object : ICVAIDLServiceCallback.Stub() {
        override fun feedbackConnectStatus(controller: Int, status: Int) {
            if (controller in 0..1) states[controller] = status
            handler.post { onChange?.invoke() }
        }

        override fun feedbackControllerUnbind(status: Int) {
            handler.post { onChange?.invoke() }
        }

        override fun feedbackControllerDeviceBleMac(device: Int, mac: String) {
            if (device in 0..1) macs[device] = mac
            handler.post { onChange?.invoke() }
        }

        override fun feedbackControllerControllerSn(device: Int, sn: String) {
            if (device in 0..1) serials[device] = sn
            handler.post { onChange?.invoke() }
        }

        override fun feedbackDeviceInfo(i1: String?, i2: String?, f: Int) {}
        override fun feedbackMainControllerSerialNumChanged(i: Int) {}
        override fun feedbackControllerThreadStarted() {}
        override fun feedbackControllerDeviceVersion(d: Int, v: String?) {}
        override fun feedbackControllerStatus(s: Int) {}
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
            context.bindService(
                intent, connection, Context.BIND_AUTO_CREATE,
            )
        } catch (e: Exception) {
            Log.w(TAG, "bind failed", e)
        }
    }

    fun unbind() {
        handler.removeCallbacks(poll)
        if (!bound) return
        try {
            service?.unregisterCallback(callback)
            context.unbindService(connection)
        } catch (e: Exception) {
            Log.w(TAG, "unbind failed", e)
        }
        service = null
        bound = false
    }

    fun enterPairMode() {
        try {
            // Pair whichever controller is waiting; the service takes
            // the slot index and handles both slots on 2.
            service?.enterPairMode(2)
        } catch (e: RemoteException) {
            Log.w(TAG, "enterPairMode failed", e)
        }
        poll()
    }

    fun interruptPairMode() {
        try {
            service?.interruptPairMode()
        } catch (e: RemoteException) {
            Log.w(TAG, "interruptPairMode failed", e)
        }
        poll()
    }

    fun unbindAll() {
        try {
            for (i in 0..1) service?.setControllerUnbind(i)
        } catch (e: RemoteException) {
            Log.w(TAG, "setControllerUnbind failed", e)
        }
        poll()
    }

    fun setMainController(index: Int) {
        try {
            service?.setMainControllerSerialNum(index)
        } catch (e: RemoteException) {
            Log.w(TAG, "setMainControllerSerialNum failed", e)
        }
        poll()
    }

    fun poll() {
        val svc = service ?: return
        try {
            pairState = svc.stationPairState
            mainController = svc.mainControllerSerialNum
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
        } catch (e: RemoteException) {
            Log.w(TAG, "poll failed", e)
        }
    }
}
