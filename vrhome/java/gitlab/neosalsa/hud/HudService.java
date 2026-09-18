package gitlab.neosalsa.hud;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.IBinder;
import android.os.SystemClock;
import android.provider.Settings;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;

/*
 * The system-overlay half of the shell: a fullscreen transparent
 * TYPE_SYSTEM_OVERLAY window holding a SurfaceView the native side renders
 * panels into. Virtual displays and the whole task/input plumbing live in
 * this process, so panels stay alive over any app - including VR games -
 * and the window can be summoned over them without going home first.
 *
 * Visibility: shown whenever the env activity owns display 0 (home space)
 * or while summoned over another app; hidden otherwise. The window never
 * takes focus - all menu keys arrive through the key filter instead, so a
 * covered game keeps focus and never sees the menu's input.
 *
 * The headset home button arrives as keycode 1003 (DEFINE_HOME in
 * gpio-keys.kl - real KEYCODE_HOME is consumed by system_server before
 * dispatch, so a plain keycode had to be substituted). SummonKeyService,
 * an accessibility service with flagRequestFilterKeyEvents, sees every key
 * before dispatch no matter which app is focused - gesture monitors never
 * receive keys on this build. Short press toggles the menu over a covered
 * app or recenters the ring in home space; long press goes home.
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
    private static volatile HudService instance;

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
        enableKeyFilter();
        instance = this;
        foreground();
        Log.i(TAG, "hud up");
    }

    @Override public void onDestroy() {
        instance = null;
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
    // Going GONE tears the surface down, which parks the render loop. The
    // window is always NOT_FOCUSABLE/NOT_TOUCHABLE: the key filter owns all
    // HUD input, so nothing underneath loses focus when the menu pops
    private void updateWindow() {
        final boolean shown = !covered || summoned;
        final int vis = shown ? View.VISIBLE : View.GONE;
        if (view.getVisibility() != vis) view.setVisibility(vis);
    }

    // --------------------------------------------------------- callbacks

    // ShellBridge poller, on the main looper
    @Override public void onCovered(boolean c) {
        covered = c;
        if (!c) summoned = false;   // back in home space: no stale summon
        updateWindow();
    }

    // SummonKeyService calls these on its own binder thread: bounce to the
    // main looper so window/flag state stays single-threaded
    static void onSummonKey(final int action) {
        final HudService s = instance;
        if (s != null) s.view.post(new Runnable() {
            @Override public void run() { s.onSummon(action); }
        });
    }

    // true while the menu is shown: at home (env front) or summoned over a
    // covered app. Menu keys get consumed by the filter and forwarded here
    // instead of reaching whatever window is focused
    static boolean menuKeysOwned() {
        HudService s = instance;
        return s != null && (!s.covered || s.summoned);
    }

    static void forwardKey(final int code, final int action,
                           final int repeat) {
        final HudService s = instance;
        if (s != null) s.view.post(new Runnable() {
            @Override public void run() { s.onKey(code, action, repeat); }
        });
    }

    // summon key from the key filter: short press toggles the menu over a
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

    // render-thread debug hook: adb input keyevent never reaches the
    // accessibility key filter, so debug.vrhome.summon toggles the same
    // path a short press would
    public void debugSummon() {
        view.post(new Runnable() {
            @Override public void run() {
                if (!covered) return;
                summoned = !summoned;
                updateWindow();
                if (summoned) nativeRecenter();
            }
        });
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

    // ------------------------------------------------------- key filter

    // The summon key is a plain keycode to the system, and Android 10 gives
    // apps no global key monitor (gesture monitors are touch-only). The
    // accessibility key filter does see every key, and the platform
    // signature holds WRITE_SECURE_SETTINGS so the service can switch itself
    // on - no settings trip needed after a flash.
    private void enableKeyFilter() {
        try {
            final String svc = getPackageName() + "/"
                    + SummonKeyService.class.getName();
            android.content.ContentResolver cr = getContentResolver();
            String cur = Settings.Secure.getString(cr,
                    Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES);
            if (cur == null || !cur.contains(svc)) {
                Settings.Secure.putString(cr,
                        Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES,
                        cur == null || cur.isEmpty() ? svc : cur + ":" + svc);
            }
            Settings.Secure.putInt(cr, Settings.Secure.ACCESSIBILITY_ENABLED,
                    1);
            Log.i(TAG, "key filter enabled");
        } catch (Throwable t) {
            Log.e(TAG, "cannot enable key filter - summon key dead", t);
        }
    }

    // --------------------------------------------------------- natives

    private static native void nativeInit(Context ctx, ShellBridge bridge);
    private static native void nativeWindow(Surface surface);
    private static native void nativeWindowGone();
    private static native void nativeKey(int code, int action, int repeat);
    private static native void nativeRecenter();
    private static native void nativeShutdown();
}
