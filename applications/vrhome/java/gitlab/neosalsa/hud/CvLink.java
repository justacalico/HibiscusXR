package gitlab.neosalsa.hud;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
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
    private static final int TR_KEY_EVENT = 2, TR_START_THREAD = 7,
            TR_REGISTER_CB = 12, TR_CTRL_SN = 18, TR_UNBIND = 19,
            TR_INTERRUPT_PAIR = 28, TR_DEV_BLE_MAC = 39,
            TR_USB_PAIR = 43, TR_CV2_CONN = 45, TR_START_PAIR = 50,
            TR_STOP_PAIR = 51, TR_SET_UNITY = 54, TR_CHARGING = 62;
    private static final int CB_THREAD_STARTED = 4;
    private static final int BATTERY_INDEX = 8;
    private static final String ACTION_SCREEN_ON =
            "android.intent.action.SCREEN_ON";
    private static final String ACTION_START_THREAD =
            "com.picovr.picovrlib.cv.broadcast.start.thread";
    // the only pushes that arrive unprompted are start-acks and live
    // controller traffic - an idle station says nothing, so "dead" can
    // only mean a very long silence. A restart on a live worker is a
    // ~8ms sensor toggle, on a dead one it's the ~28s re-init that
    // brings the station back, so err on the long side
    private static final long THREAD_DEAD_MS = 300000;

    private final Context ctx;
    private final Handler main = new Handler(Looper.getMainLooper());
    // binder calls to the link can block for as long as the remote end
    // stalls - none of them may run on the main or render threads, so
    // every transact goes through this worker
    private final Handler work;
    private volatile IBinder svc;
    private volatile ServiceConnection conn;
    private volatile Listener listener;
    private PowerManager.WakeLock wakeLock;
    // set by the service's callback pushes: any inbound frame means the
    // SPI worker is alive, which is the only time pair/query calls land
    private volatile boolean threadUp;
    // a pair request made before the thread was up - fired when the
    // first callback lands
    private volatile boolean wantPair;
    // last service push (or start request we should give time to land);
    // the poke restart gate runs off this
    private volatile long lastBeatMs;
    // the station bond-table seed only needs to go out once per bind
    private volatile boolean seedTried;

    private CvLink(Context c) {
        ctx = c;
        HandlerThread t = new HandlerThread("vrhud.cvlink");
        t.start();
        work = new Handler(t.getLooper());
    }

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
                threadUp = false;
                seedTried = false;
                work.post(new Runnable() {
                    @Override public void run() {
                        // the stock client announces itself with a 4-part
                        // version; anything >= 2.8.0.0 flips the service
                        // into CV2 per-controller reporting
                        transactStr(TR_SET_UNITY, "3.0.0.0");
                        registerCb();
                        // the service powers and wakes the station off
                        // cvReceiver's SCREEN_ON, but the display was up
                        // long before this process existed so the real
                        // broadcast was missed - fire it again or the
                        // station never leaves reset
                        Intent on = new Intent(ACTION_SCREEN_ON);
                        on.setPackage(PKG);
                        ctx.sendBroadcast(on);
                        feedStreams();
                        // the SPI link to the RF station only exists while
                        // this worker runs, so pair/query calls are no-ops
                        // until the first callback push marks it up
                        Parcel r = transact(TR_START_THREAD, 1, 1);
                        if (r != null) r.recycle();
                        lastBeatMs = android.os.SystemClock.uptimeMillis();
                        if (listener != null) main.post(new Runnable() {
                            @Override public void run() {
                                if (listener != null) listener.onChange();
                            }
                        });
                    }
                });
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
                releaseConn();
            }
        };
        try {
            Intent i = new Intent(ACTION);
            i.setPackage(PKG);
            if (ctx.bindService(i, conn, Context.BIND_AUTO_CREATE)) {
                holdDisplay();
            } else {
                // a refused bind leaves conn set forever - drop it or no
                // later attach can ever retry
                conn = null;
            }
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

    private void transactStr(int code, String s) {
        Parcel in = Parcel.obtain();
        in.writeInterfaceToken(IFACE);
        in.writeString(s);
        Parcel out = Parcel.obtain();
        try {
            svc.transact(code, in, out, 0);
            out.readException();
            out.recycle();
        } catch (Throwable t) {
            out.recycle();
        } finally {
            in.recycle();
        }
    }

    // every push is oneway with the callback interface token up front;
    // any of them proves the SPI worker is alive
    private final android.os.Binder cb = new android.os.Binder() {
        @Override protected boolean onTransact(int code, Parcel data,
                Parcel reply, int flags) {
            try { data.enforceInterface(CB_IFACE); }
            catch (Throwable t) { return false; }
            lastBeatMs = android.os.SystemClock.uptimeMillis();
            if (!threadUp)
                main.post(new Runnable() {
                    @Override public void run() { onPush(); }
                });
            return true;
        }
    };

    private void onPush() {
        // busy pushes arrive at ~100Hz while the station works; the
        // listener only cares that the SPI worker exists, which is
        // decided by the first push - firing it per push would churn the
        // card's version and stall the render thread on snapshot calls
        if (threadUp) return;
        threadUp = true;
        if (wantPair && svc != null)
            work.post(new Runnable() {
                @Override public void run() { pairNow(); }
            });
        else if (!seedTried) {
            seedTried = true;
            work.post(new Runnable() {
                @Override public void run() { seedWhitelist(); }
            });
        }
        if (listener != null) listener.onChange();
    }

    // the station's bond table is RAM: a power-cycled station forgets
    // every wand the service ever bonded. The service keeps the last
    // bonds in its own persist files, so once the thread is up the
    // whitelist gets pushed back and a stored wand can reconnect on
    // wake instead of needing a fresh scan pair
    private void seedWhitelist() {
        String m1 = readMac("/persist/ndi/ctr1BleMac");
        String m2 = readMac("/persist/ndi/ctr2BleMac");
        if (m1 == null || m2 == null || svc == null) return;
        Parcel in = Parcel.obtain();
        in.writeInterfaceToken(IFACE);
        in.writeString(m1);
        in.writeString(m2);
        Parcel out = Parcel.obtain();
        try {
            svc.transact(TR_USB_PAIR, in, out, 0);
            out.readException();
            out.recycle();
        } catch (Throwable t) {
            out.recycle();
        } finally {
            in.recycle();
        }
    }

    // the file's first six bytes are the raw MAC, the rest is an
    // ASCII id suffix the station doesn't want
    private static String readMac(String path) {
        try {
            java.io.RandomAccessFile f =
                    new java.io.RandomAccessFile(path, "r");
            byte[] b = new byte[6];
            int n = f.read(b);
            f.close();
            if (n < 6) return null;
            return String.format("%02x:%02x:%02x:%02x:%02x:%02x",
                    b[0], b[1], b[2], b[3], b[4], b[5]);
        } catch (Throwable t) {
            return null;
        }
    }

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

    private final int[] prevState = {-1, -1};

    @Override public void poll(SettingsState.CtrlSlot[] slots) {
        if (svc == null || !threadUp) return;
        for (int i = 0; i < slots.length; ++i) {
            SettingsState.CtrlSlot c = slots[i];
            Parcel r = transact(TR_CV2_CONN, i);
            int st = 0;
            if (r != null) {
                try { st = r.readInt(); } catch (Throwable ignored) {}
                r.recycle();
            }
            c.state = st;
            // identity rides the connect edge, not the poll: the MAC/SN
            // queries spawn real station tasks and keeping them in the
            // 2s loop starves an open scan window
            if (st == 1 && prevState[i] != 1) {
                // the service numbers the two stations 1,2 - left, right
                r = transact(TR_DEV_BLE_MAC, i + 1);
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
            } else if (st == 0) {
                c.mac = "";
                c.serial = "";
            }
            prevState[i] = c.state;
            r = transact(TR_CHARGING, i);
            if (r != null) {
                try { c.chg = r.readInt() != 0; }
                catch (Throwable ignored) {}
                r.recycle();
            }
            // getControllerKeyEvent's int[] carries the battery level at
            // index 8
            r = transact(TR_KEY_EVENT, i);
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

    @Override public void setPairing(final boolean on) {
        wantPair = on;
        work.post(new Runnable() {
            @Override public void run() {
                if (svc == null) return;
                if (!on) {
                    // interrupt closes the native scan; stop clears the
                    // service's needstartpairing latch, which interrupt
                    // alone leaves set
                    Parcel r = transact(TR_INTERRUPT_PAIR);
                    if (r != null) r.recycle();
                    r = transact(TR_STOP_PAIR, 0);
                    if (r != null) r.recycle();
                    return;
                }
                if (!threadUp) {
                    Parcel r = transact(TR_START_THREAD, 1, 1);
                    if (r != null) r.recycle();
                    lastBeatMs = android.os.SystemClock.uptimeMillis();
                    return;   // onPush() fires pairNow() once up
                }
                pairNow();
            }
        });
    }

    // the service latches needstartpairing and ignores every later start
    // while it is set, so a re-tap has to stop first to get a fresh scan
    // window; the arg is the device selector the stock client always
    // passed as 0 for an open scan
    private void pairNow() {
        Parcel r = transact(TR_STOP_PAIR, 0);
        if (r != null) r.recycle();
        r = transact(TR_START_PAIR, 0);
        if (r != null) r.recycle();
    }

    @Override public void debugTransact(final int code, final int arg) {
        work.post(new Runnable() {
            @Override public void run() {
                if (svc == null) return;
                Parcel r = transact(code, arg);
                if (r == null) {
                    Log.i(TAG, "tr " + code + " -> null");
                    return;
                }
                StringBuilder sb = new StringBuilder();
                while (r.dataAvail() >= 4)
                    sb.append(r.readInt()).append(' ');
                Log.i(TAG, "tr " + code + " -> " + sb);
                r.recycle();
            }
        });
    }

    // opcode 0x11 pushes two BLE MACs into the station's bond table -
    // the whitelist is RAM state, so a station that lost power comes
    // back empty and only this (or a fresh scan bond) refills it
    @Override public void usbPair(final String mac1, final String mac2) {
        work.post(new Runnable() {
            @Override public void run() {
                if (svc == null) return;
                Parcel in = Parcel.obtain();
                in.writeInterfaceToken(IFACE);
                in.writeString(mac1);
                in.writeString(mac2);
                Parcel out = Parcel.obtain();
                try {
                    svc.transact(TR_USB_PAIR, in, out, 0);
                    out.readException();
                    out.recycle();
                } catch (Throwable t) {
                    out.recycle();
                } finally {
                    in.recycle();
                }
            }
        });
    }

    @Override public void unbindAll() {
        work.post(new Runnable() {
            @Override public void run() {
                if (svc == null) return;
                unbindAllNow();
            }
        });
    }

    private void unbindAllNow() {
        // setControllerUnbind takes the slot index; clear both
        for (int i = 0; i < 2; ++i) {
            Parcel r = transact(TR_UNBIND, i);
            if (r != null) r.recycle();
        }
    }

    // the two enables stock fires once when 6dof tracking engages: the
    // controller-side EM ADC stream (grapadc) and the slpi ssc sensor
    // receive path (ENABLE_RX) - without them the ndi_fpga stream never
    // starts even when everything else is configured. Sent once per
    // bind; they are per-service-life flags, not per-frame
    private void feedStreams() {
        Intent adc = new Intent("android.intent.pvrcon.grapadc");
        adc.setPackage(PKG);
        ctx.sendBroadcast(adc);
        Intent rx = new Intent("pui.settings.action.ENABLE_RX");
        rx.setPackage(PKG);
        rx.putExtra("enable", 1);
        ctx.sendBroadcast(rx);
    }

    // The service only streams controller data while its SPI thread
    // runs, and nothing on this build starts it (the stock shell used
    // to). The render loop pokes when the sharemem flags stop flapping -
    // thread never started, or the service crashed and came back bare.
    // Every start also restarts a live thread, so the native side only
    // calls while the channel is actually quiet.
    @Override public void poke() {
        work.post(new Runnable() {
            @Override public void run() {
                try {
                    // a dead RemoteService never gets re-bound by the
                    // settings card alone; re-attach so the service's
                    // init sequence (SCREEN_ON + thread start) replays
                    if (svc == null && conn == null) attach(listener);
                    // the sharemem stays quiet whenever no controller is
                    // linked, so "quiet" alone can't tell a dead SPI
                    // worker from an idle one - only a long push silence
                    // (and no open scan window) earns a restart
                    long now = android.os.SystemClock.uptimeMillis();
                    if (!wantPair && now - lastBeatMs > THREAD_DEAD_MS) {
                        lastBeatMs = now;
                        // restart kills the worker - drop the flag so the
                        // first push of the new thread re-fires pairNow
                        threadUp = false;
                        Intent on = new Intent(ACTION_SCREEN_ON);
                        on.setPackage(PKG);
                        ctx.sendBroadcast(on);
                        Intent i = new Intent(ACTION_START_THREAD);
                        i.setPackage(PKG);
                        i.putExtra("HeadTrackMode", 1);
                        i.putExtra("CtrlerTrackMode", 1);
                        ctx.sendBroadcast(i);
                    }
                    Log.i(TAG, "ctrl thread poked");
                } catch (Throwable t) {
                    Log.e(TAG, "ctrl poke failed", t);
                }
            }
        });
    }
}
