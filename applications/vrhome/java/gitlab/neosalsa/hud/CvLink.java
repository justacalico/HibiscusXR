package gitlab.neosalsa.hud;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.Parcel;
import android.os.PowerManager;
import android.util.Log;

/*
 * Controller link over the image's CV controller service: two tracked
 * slots polled for link state, BLE mac, serial, charge and battery over
 * raw binder transactions (the service ships no client library, so the
 * codes are spelled out); link scan and unbind are fire-and-forget, and
 * poke() is the broadcast trio that wakes the service's SPI/sensor
 * threads when the shared-memory feed goes quiet.
 *
 * This file is the device layer for the link: every package name,
 * action and transaction code in here belongs to that one service, and
 * probe() only instantiates it when resolveService says the service
 * exists on the image - the rest of the shell never names it.
 */
final class CvLink implements CtrlService {
    private static final String TAG = "vrhud.cvlink";
    private static final String PKG =
            "com.picovr.picovrlib.cvcontroller";
    private static final String ACTION =
            "com.picovr.picovrlib.cvcontrollerlib.CVControllerAIDLService";
    private static final String IFACE =
            "com.picovr.picovrlib.cvcontrollerlib.CVControllerAIDLService";
    private static final String CB_IFACE =
            "com.picovr.picovrlib.cvcontrollerlib.ICVAIDLServiceCallback";
    private static final int TR_START_THREAD = 7, TR_REGISTER_CB = 12,
            TR_CTRL_SN = 18, TR_UNBIND = 19, TR_DEV_BLE_MAC = 39,
            TR_CV2_CONN = 45, TR_START_PAIR = 50, TR_STOP_PAIR = 51,
            TR_CHARGING = 62, TR_BATTERY = 63;
    private static final int BATTERY_INDEX = 8;

    private final Context ctx;
    private IBinder svc;
    private ServiceConnection conn;
    private Listener listener;
    private PowerManager.WakeLock wakeLock;

    private CvLink(Context c) { ctx = c; }

    static CvLink probe(Context ctx) {
        Intent i = new Intent(ACTION);
        i.setPackage(PKG);
        return ctx.getPackageManager().resolveService(i, 0) != null
                ? new CvLink(ctx) : null;
    }

    @Override public void attach(Listener l) {
        listener = l;
        if (conn != null) return;
        conn = new ServiceConnection() {
            @Override public void onServiceConnected(
                    ComponentName n, IBinder b) {
                svc = b;
                registerCb();
                if (listener != null) listener.onChange();
            }
            @Override public void onServiceDisconnected(
                    ComponentName n) {
                svc = null;
                if (listener != null) listener.onChange();
            }
            // a dead binding never reconnects on its own: drop the conn so
            // the next attach() binds fresh instead of returning early on a
            // zombie connection
            @Override public void onBindingDied(ComponentName n) {
                releaseConn();
                if (listener != null) listener.onChange();
            }
            @Override public void onNullBinding(ComponentName n) {
                svc = null;
            }
        };
        try {
            Intent i = new Intent(ACTION);
            i.setPackage(PKG);
            if (ctx.bindService(i, conn, Context.BIND_AUTO_CREATE))
                holdDisplay();
        } catch (Throwable t) {
            conn = null;
            Log.w(TAG, "ctrl bind failed", t);
        }
    }

    @Override public void detach() {
        releaseConn();
    }

    private void releaseConn() {
        svc = null;
        if (conn != null) {
            try { ctx.unbindService(conn); } catch (Throwable ignored) {}
            conn = null;
        }
        if (wakeLock != null && wakeLock.isHeld()) wakeLock.release();
    }

    // the CV service powers its station on SCREEN_ON; nothing keeps a
    // head-worn build from sleeping, so while the link is wanted we hold
    // the display up - same end state stock's session lock produces
    private void holdDisplay() {
        if (wakeLock == null) {
            PowerManager pm =
                    (PowerManager) ctx.getSystemService(Context.POWER_SERVICE);
            wakeLock = pm.newWakeLock(
                    PowerManager.SCREEN_BRIGHT_WAKE_LOCK
                            | PowerManager.ACQUIRE_CAUSES_WAKEUP,
                    "vrhud:ctrl");
            wakeLock.setReferenceCounted(false);
        }
        if (!wakeLock.isHeld()) wakeLock.acquire();
    }

    @Override public boolean bound() { return svc != null; }

    // the interface token must lead the parcel: callers never write into
    // the request themselves, they pass int args here so the token always
    // lands at offset 0 where the service's enforceInterface reads it
    private Parcel transact(int code, int... args) {
        Parcel in = Parcel.obtain();
        in.writeInterfaceToken(IFACE);
        for (int a : args) in.writeInt(a);
        Parcel out = Parcel.obtain();
        try {
            svc.transact(code, in, out, 0);
            out.readException();
            return out;
        } catch (Throwable t) {
            out.recycle();
            return null;
        } finally {
            in.recycle();
        }
    }

    private final android.os.Binder cb = new android.os.Binder() {
        @Override protected boolean onTransact(int code, Parcel data,
                Parcel reply, int flags) {
            try { data.enforceInterface(CB_IFACE); }
            catch (Throwable ignored) {}
            return true;
        }
    };

    private void registerCb() {
        Parcel in = Parcel.obtain();
        try {
            in.writeInterfaceToken(IFACE);
            in.writeStrongBinder(cb);
            svc.transact(TR_REGISTER_CB, in, null,
                    IBinder.FLAG_ONEWAY);
        } catch (Throwable ignored) {}
        in.recycle();
    }

    @Override public void poll(SettingsState.CtrlSlot[] slots) {
        if (svc == null) return;
        for (int i = 0; i < slots.length; ++i) {
            SettingsState.CtrlSlot c = slots[i];
            Parcel r = transact(TR_CV2_CONN, i);
            int st = 0;
            if (r != null) {
                try { st = r.readInt(); } catch (Throwable ignored) {}
                r.recycle();
            }
            c.state = st;
            r = transact(TR_DEV_BLE_MAC, i);
            if (r != null) {
                try { c.mac = nullToEmpty(r.readString()); }
                catch (Throwable ignored) {}
                r.recycle();
            }
            r = transact(TR_CTRL_SN, i);
            if (r != null) {
                try { c.serial = nullToEmpty(r.readString()); }
                catch (Throwable ignored) {}
                r.recycle();
            }
            r = transact(TR_CHARGING, i);
            if (r != null) {
                try { c.chg = r.readInt() != 0; }
                catch (Throwable ignored) {}
                r.recycle();
            }
            r = transact(TR_BATTERY, i);
            if (r != null) {
                try {
                    int[] b = r.createIntArray();
                    if (b != null && b.length > BATTERY_INDEX)
                        c.batt = b[BATTERY_INDEX];
                    else if (b != null && b.length > 0)
                        c.batt = b[0];
                } catch (Throwable ignored) {}
                r.recycle();
            }
        }
    }

    private static String nullToEmpty(String s) {
        return s == null ? "" : s;
    }

    @Override public void setPairing(boolean on) {
        if (svc == null) return;
        Parcel r = transact(on ? TR_START_PAIR : TR_STOP_PAIR, on ? 1 : 0);
        if (r != null) r.recycle();
        if (on) {
            Parcel rr = transact(TR_START_THREAD, 1, 1);
            if (rr != null) rr.recycle();
        }
    }

    @Override public void debugTransact(int code, int arg) {
        if (svc == null) return;
        Parcel r = transact(code, arg);
        if (r == null) { Log.i(TAG, "tr " + code + " -> null"); return; }
        StringBuilder sb = new StringBuilder();
        while (r.dataAvail() >= 4) sb.append(r.readInt()).append(' ');
        Log.i(TAG, "tr " + code + " -> " + sb);
        r.recycle();
    }

    @Override public void unbindAll() {
        if (svc == null) return;
        Parcel r = transact(TR_UNBIND);
        if (r != null) r.recycle();
    }

    // The service only streams controller data while its SPI thread
    // runs, and nothing on this build starts it (the stock shell used
    // to). The render loop pokes when the sharemem flags stop flapping -
    // thread never started, or the service crashed and came back bare.
    // Every start also restarts a live thread, so the native side only
    // calls while the channel is actually quiet.
    @Override public void poke() {
        try {
            Intent i = new Intent(
                    "com.picovr.picovrlib.cv.broadcast.start.thread");
            i.setPackage(PKG);
            i.putExtra("HeadTrackMode", 1);
            i.putExtra("CtrlerTrackMode", 1);
            ctx.sendBroadcast(i);
            // stock fires this when 6dof tracking engages to enable the
            // controller-side EM ADC stream; nothing on this build does
            Intent adc = new Intent("android.intent.pvrcon.grapadc");
            adc.setPackage(PKG);
            ctx.sendBroadcast(adc);
            // same story for the slpi side - the service gates its ssc
            // sensor receive path on this action, so without it the
            // ndi_fpga stream never starts even when everything is
            // configured
            Intent rx = new Intent("pui.settings.action.ENABLE_RX");
            rx.setPackage(PKG);
            rx.putExtra("enable", 1);
            ctx.sendBroadcast(rx);
            Log.i(TAG, "ctrl thread poked");
        } catch (Throwable t) {
            Log.e(TAG, "ctrl poke failed", t);
        }
    }
}
