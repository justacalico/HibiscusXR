package gitlab.neosalsa.hud;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.hardware.input.InputManager;
import android.os.IBinder;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import android.view.InputChannel;
import android.view.InputEvent;
import android.view.InputEventReceiver;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;

import java.lang.reflect.Method;

/*
 * The system-overlay half of the shell: a fullscreen transparent
 * TYPE_SYSTEM_OVERLAY window holding a SurfaceView the native side renders
 * panels into. Virtual displays and the whole task/input plumbing live in
 * this process, so panels stay alive over any app - including VR games -
 * and the window can be summoned over them without going home first.
 *
 * Visibility: shown whenever the env activity owns display 0 (home space)
 * or while summoned over another app; hidden otherwise. The window is
 * focusable whenever shown so headset keys reach HudView instead of the app
 * underneath.
 *
 * The headset home button arrives as keycode 1003 (DEFINE_HOME in
 * gpio-keys.kl - real KEYCODE_HOME is consumed by system_server before
 * dispatch, so a plain keycode had to be substituted). An input monitor
 * watches for it globally: short press toggles the menu over a covered app
 * or recenters the ring in home space; long press goes home.
 */
public class HudService extends Service implements SurfaceHolder.Callback,
        HudView.KeySink, ShellBridge.CoveredListener {
    private static final String TAG = "vrhud";
    private static final int K_SUMMON = 1003;      // DEFINE_HOME
    private static final long LONG_MS = 600;

    static { System.loadLibrary("vrhud"); }

    private WindowManager wm;
    private HudView view;
    private WindowManager.LayoutParams lp;
    private ShellBridge bridge;
    private Object monitor;              // android.view.InputMonitor (hidden)
    private InputEventReceiver receiver;

    // start hidden: the first poll decides; a game booting before the
    // service should never see the overlay flash up
    private volatile boolean covered = true;
    private volatile boolean summoned;   // user pulled the HUD over it
    private long summonDownAt = -1;

    // --------------------------------------------------------- lifecycle

    @Override public void onCreate() {
        super.onCreate();
        try {
            bridge = new ShellBridge(this, this);
        } catch (Throwable t) {
            Log.e(TAG, "bridge ctor failed", t);
        }
        nativeInit(this, bridge);
        buildWindow();
        startMonitor();
        foreground();
        Log.i(TAG, "hud up");
    }

    @Override public void onDestroy() {
        if (receiver != null) receiver.dispose();
        disposeMonitor();
        nativeShutdown();
        super.onDestroy();
    }

    @Override public IBinder onBind(Intent i) { return null; }

    private void foreground() {
        NotificationManager nm =
                (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        nm.createNotificationChannel(new NotificationChannel("vrhud",
                "HUD", NotificationManager.IMPORTANCE_MIN));
        startForeground(1, new Notification.Builder(this, "vrhud")
                .setContentTitle("PN2 HUD")
                .setSmallIcon(android.R.drawable.ic_menu_compass)
                .build());
    }

    // --------------------------------------------------------- window

    private void buildWindow() {
        wm = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
        view = new HudView(this, this);
        SurfaceView sv = new SurfaceView(this);
        sv.getHolder().setFormat(PixelFormat.TRANSLUCENT);
        sv.getHolder().addCallback(this);
        view.addView(sv, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        lp = new WindowManager.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.TYPE_SYSTEM_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                        | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                        | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                        | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                        | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
                        | WindowManager.LayoutParams.FLAG_FULLSCREEN
                        | WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
                PixelFormat.TRANSLUCENT);
        lp.setTitle("vrhud");
        wm.addView(view, lp);
        view.setVisibility(View.GONE);
    }

    // shown = home space (env is the top task) or summoned over an app.
    // Going GONE tears the surface down, which parks the render loop; a
    // hidden window also must not hold focus
    private void updateWindow() {
        final boolean shown = !covered || summoned;
        int f = lp.flags;
        if (shown) f &= ~WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
        else       f |=  WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
        if (f != lp.flags) {
            lp.flags = f;
            wm.updateViewLayout(view, lp);
        }
        final int vis = shown ? View.VISIBLE : View.GONE;
        if (view.getVisibility() != vis) {
            view.setVisibility(vis);
            if (shown) view.requestFocus();
        }
    }

    // --------------------------------------------------------- callbacks

    // ShellBridge poller, on the main looper
    @Override public void onCovered(boolean c) {
        covered = c;
        if (!c) summoned = false;   // back in home space: no stale summon
        updateWindow();
    }

    // summon key from the input monitor: short press toggles the menu over a
    // covered app or recenters the ring in home space; long press goes home
    private void onSummon(int action) {
        if (action == KeyEvent.ACTION_DOWN) {
            summonDownAt = SystemClock.uptimeMillis();
        } else if (action == KeyEvent.ACTION_UP && summonDownAt >= 0) {
            final boolean long_ =
                    SystemClock.uptimeMillis() - summonDownAt > LONG_MS;
            summonDownAt = -1;
            if (long_) { goHome(); return; }
            if (covered) {
                summoned = !summoned;
                updateWindow();
                if (summoned) nativeRecenter();
            } else {
                nativeRecenter();
            }
        }
    }

    private void goHome() {
        Intent i = new Intent(Intent.ACTION_MAIN);
        i.addCategory(Intent.CATEGORY_HOME);
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(i);
    }

    // HudView.KeySink: keys that reached the focused HUD window
    @Override public boolean onKey(int code, int action, int repeat) {
        if (code == K_SUMMON) return true;   // the monitor owns this one
        if (code == KeyEvent.KEYCODE_BACK && action == KeyEvent.ACTION_UP
                && covered && summoned) {
            summoned = false;
            updateWindow();
            return true;
        }
        nativeKey(code, action, repeat);
        return true;
    }

    // SurfaceHolder.Callback: the native render thread owns the GL end
    @Override public void surfaceCreated(SurfaceHolder h) {
        nativeWindow(h.getSurface());
    }
    @Override public void surfaceChanged(SurfaceHolder h, int f, int w,
                                         int ht) {}
    @Override public void surfaceDestroyed(SurfaceHolder h) {
        nativeWindowGone();
    }

    // --------------------------------------------------------- monitor

    // InputManager.monitorGestureInput is hidden; the platform signature and
    // the hidden-api exemption cover both the call and MONITOR_INPUT. The
    // monitor sees display-0 keys regardless of which window is focused -
    // exactly what a summon key needs
    private void startMonitor() {
        try {
            InputManager im =
                    (InputManager) getSystemService(Context.INPUT_SERVICE);
            Method m = InputManager.class.getDeclaredMethod(
                    "monitorGestureInput", String.class, int.class);
            monitor = m.invoke(im, "vrhud", 0);
            Method getChan = monitor.getClass().getDeclaredMethod(
                    "getInputChannel");
            InputChannel ch = (InputChannel) getChan.invoke(monitor);
            receiver = new InputEventReceiver(ch, Looper.getMainLooper()) {
                @Override public void onInputEvent(InputEvent ev) {
                    if (ev instanceof KeyEvent) {
                        KeyEvent k = (KeyEvent) ev;
                        if (k.getKeyCode() == K_SUMMON)
                            onSummon(k.getAction());
                    }
                    finishInputEvent(ev, false);
                }
            };
            Log.i(TAG, "input monitor up");
        } catch (Throwable t) {
            Log.e(TAG, "no input monitor - summon key dead", t);
        }
    }

    private void disposeMonitor() {
        if (monitor == null) return;
        try {
            monitor.getClass().getDeclaredMethod("dispose").invoke(monitor);
        } catch (Throwable ignored) {}
        monitor = null;
    }

    // --------------------------------------------------------- natives

    private static native void nativeInit(Context ctx, ShellBridge bridge);
    private static native void nativeWindow(Surface surface);
    private static native void nativeWindowGone();
    private static native void nativeKey(int code, int action, int repeat);
    private static native void nativeRecenter();
    private static native void nativeShutdown();
}
