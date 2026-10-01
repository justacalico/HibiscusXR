package gitlab.neosalsa.hud;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.database.ContentObserver;
import android.graphics.PixelFormat;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.SystemClock;
import android.provider.Settings;
import android.util.Log;
import android.view.Display;
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
 * app; holding the key fills a progress ring in the view and recenters the
 * dash when the fill completes.
 */
public class HudService extends Service implements SurfaceHolder.Callback,
        HudView.KeySink, ShellBridge.CoveredListener {
    private static final String TAG = "vrhud";
    private static final int K_SUMMON = 1003;      // DEFINE_HOME
    private static final long LONG_MS = 600;
    private static final long TOAST_MS = 5000;
    // global key the settings app's developer toggle writes: while set the
    // window stays up over anything - covered apps included - so the
    // status line is always visible
    private static final String DEBUG_HUD = "hibiscus_debug_hud";

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
    // the hold ring has to be visible while the button is down even over an
    // app the dash isn't summoned on: the window comes up for the hold and
    // drops again if the press is released before the fill completes
    private boolean holdPreview;
    private final Runnable holdFire = new Runnable() {
        @Override public void run() { onHoldDone(); }
    };

    // notification toast: a fresh post while an app covers the display pops
    // the window for a few seconds so the card is visible mid-game. The
    // window stays NOT_FOCUSABLE so the app keeps focus; a summon during
    // the toast just turns it into the full dash
    private volatile boolean toastOn;
    private long toastEnd;
    // a live crash/ANR card keeps the window up like a toast, but it is
    // modal: it stays until the user dismisses it, not for a few seconds
    private volatile boolean sysMsgOn;
    // the floating keyboard over a covered app: the quad has to render, so
    // the window pops for the IME's whole input session
    private volatile boolean kbdOn;
    private final Runnable toastOff = new Runnable() {
        @Override public void run() {
            final long left = toastEnd - SystemClock.uptimeMillis();
            if (left > 0) { view.postDelayed(this, left); return; }
            toastOn = false;
            updateWindow();
        }
    };

    // developer-settings debug line: kept in sync with the global key so
    // the render loop can draw it over covered apps
    private volatile boolean debugHud;
    private final ContentObserver debugObs =
            new ContentObserver(new Handler(Looper.getMainLooper())) {
        @Override public void onChange(boolean self) {
            loadDebugHud();
        }
    };

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
        enableNotifAccess();
        NotifService.onPosted(new Runnable() {
            @Override public void run() { onNotifPosted(); }
        });
        SysMsgs.onChanged(new Runnable() {
            @Override public void run() { onSysMsgChanged(); }
        });
        SysMsgs.start(this);
        instance = this;
        loadDebugHud();
        getContentResolver().registerContentObserver(
                Settings.Global.getUriFor(DEBUG_HUD), false, debugObs);
        foreground();
        Log.i(TAG, "hud up");
    }

    @Override public void onDestroy() {
        instance = null;
        getContentResolver().unregisterContentObserver(debugObs);
        nativeShutdown();
        super.onDestroy();
    }

    // a locale switch lands here: bump the ui-strings version so the
    // render thread refetches the chrome labels it draws itself, and
    // re-resolve the cached launcher-app labels under the new locale
    @Override public void onConfigurationChanged(Configuration c) {
        super.onConfigurationChanged(c);
        UiStrings.invalidate();
        if (bridge != null) bridge.invalidateAppLabels();
    }

    // CVService only streams controller data while its SPI thread runs, and
    // nothing on this build starts it (stock VRShell used to). The render
    // thread calls this when the sharemem flags stop flapping - thread
    // never started, or RemoteService crashed and came back bare. Every
    // start also restarts a live thread, so the native side only calls it
    // while the channel is actually quiet.
    public void pokeCtrlThread() {
        try {
            Intent i = new Intent(
                    "com.picovr.picovrlib.cv.broadcast.start.thread");
            i.setPackage("com.picovr.picovrlib.cvcontroller");
            i.putExtra("HeadTrackMode", 1);
            i.putExtra("CtrlerTrackMode", 1);
            sendBroadcast(i);
            // stock fires this when 6dof tracking engages to enable the
            // controller-side EM ADC stream; nothing on this build does
            Intent adc = new Intent("android.intent.pvrcon.grapadc");
            adc.setPackage("com.picovr.picovrlib.cvcontroller");
            sendBroadcast(adc);
            // same story for the slpi side - CVService gates its ssc
            // sensor receive path on this action, so without it the
            // ndi_fpga stream never starts even when everything is
            // configured
            Intent rx = new Intent("pui.settings.action.ENABLE_RX");
            rx.setPackage("com.picovr.picovrlib.cvcontroller");
            rx.putExtra("enable", 1);
            sendBroadcast(rx);
            Log.i(TAG, "cvservice ctrl thread poked");
        } catch (Throwable t) {
            Log.e(TAG, "cvservice poke failed", t);
        }
    }

    @Override public IBinder onBind(Intent i) { return null; }

    private void foreground() {
        NotificationManager nm =
                (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        nm.createNotificationChannel(new NotificationChannel("vrhud",
                getString(R.string.hud_channel),
                NotificationManager.IMPORTANCE_MIN));
        startForeground(1, new Notification.Builder(this, "vrhud")
                .setContentTitle(getString(R.string.hud_label))
                .setSmallIcon(android.R.drawable.ic_menu_compass)
                .build());
    }

    // --------------------------------------------------------- window

    private void buildWindow() {
        wm = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
        view = new HudView(this, this);
        SurfaceView sv = new SurfaceView(this);
        sv.getHolder().setFormat(PixelFormat.TRANSLUCENT);
        // pin the buffer to the panel's physical mode: a surface created
        // during an app's rotation flap otherwise keeps the portrait dims
        // and both stereo eyes end up squashed into one side of the screen
        final Display.Mode mode = wm.getDefaultDisplay().getMode();
        sv.getHolder().setFixedSize(mode.getPhysicalWidth(),
                                    mode.getPhysicalHeight());
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

    private void loadDebugHud() {
        debugHud = Settings.Global.getInt(getContentResolver(),
                DEBUG_HUD, 0) == 1;
        updateWindow();
    }

    // shown = home space (env is the top task) or summoned over an app,
    // or pinned up for the debug status line. Going GONE tears the
    // surface down, which parks the render loop. The window is always
    // NOT_FOCUSABLE/NOT_TOUCHABLE: the key filter owns all HUD input, so
    // nothing underneath loses focus when the menu pops
    private void updateWindow() {
        final boolean shown = !covered || summoned || holdPreview || toastOn
                || sysMsgOn || kbdOn || debugHud;
        final int vis = shown ? View.VISIBLE : View.GONE;
        if (view.getVisibility() != vis) {
            Log.i(TAG, "window " + (shown ? "shown" : "hidden")
                    + " covered=" + covered + " summoned=" + summoned);
            view.setVisibility(vis);
        }
        if (bridge != null) {
            bridge.setToastOnly(covered && toastOn && !summoned);
            // a system message over a covered app draws alone - the dash
            // chrome stays down unless the user summons it
            bridge.setSysMsgOnly(covered && sysMsgOn && !summoned);
            // same for the floating keyboard: an IME request from a covered
            // app pops just the quad
            bridge.setKbdOnly(covered && kbdOn && !summoned);
            bridge.setDebugHud(debugHud);
            // window is up over a covered app only for the status line:
            // the render loop draws nothing else
            bridge.setDebugOnly(covered && !summoned && debugHud);
        }
    }

    // --------------------------------------------------------- callbacks

    // ShellBridge poller, on the main looper
    @Override public void onCovered(boolean c) {
        covered = c;
        if (!c) {   // back in home space: no stale summon or toast
            summoned = false;
            toastOn = false;
        }
        updateWindow();
    }

    // NotifService on the listener's binder thread: a fresh post while an
    // app owns the display pops the cards for a few seconds. At home or
    // while summoned the dash is already up, so a post just lands in the
    // stack
    static void onNotifPosted() {
        final HudService s = instance;
        if (s == null) return;
        s.view.post(new Runnable() {
            @Override public void run() {
                if (!s.covered || s.summoned) return;
                s.toastOn = true;
                s.toastEnd = SystemClock.uptimeMillis() + TOAST_MS;
                s.updateWindow();
                s.view.removeCallbacks(s.toastOff);
                s.view.postDelayed(s.toastOff, TOAST_MS);
            }
        });
    }

    // SysMsgs fires this on the main looper from its poll and on the render
    // thread from a card click: either way the window bookkeeping stays on
    // the view's own thread
    static void onSysMsgChanged() {
        final HudService s = instance;
        if (s == null) return;
        s.view.post(new Runnable() {
            @Override public void run() {
                s.sysMsgOn = SysMsgs.hasMsgs();
                s.updateWindow();
            }
        });
    }

    // the dock/launch path asks to drop the menu (immersive app going
    // foreground): called on the render thread, bounce to the looper
    @Override public void onDismissMenu() {
        view.post(new Runnable() {
            @Override public void run() {
                if (!summoned && !holdPreview && !toastOn) return;
                summoned = false;
                holdPreview = false;
                toastOn = false;
                updateWindow();
            }
        });
    }

    // the keyboard's state broadcast lands in ShellBridge's receiver on the
    // main looper already; the bounce is for symmetry with the other flags
    @Override public void onKbd(final boolean shown) {
        view.post(new Runnable() {
            @Override public void run() {
                if (kbdOn == shown) return;
                kbdOn = shown;
                updateWindow();
            }
        });
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
    // instead of reaching whatever window is focused. A system-message card
    // over a covered app owns keys too - its buttons are gaze + confirm
    static boolean menuKeysOwned() {
        HudService s = instance;
        return s != null && (!s.covered || s.summoned || s.sysMsgOn
                || s.kbdOn);
    }

    static void forwardKey(final int code, final int action,
                           final int repeat) {
        final HudService s = instance;
        if (s != null) s.view.post(new Runnable() {
            @Override public void run() { s.onKey(code, action, repeat); }
        });
    }

    // summon key from the key filter: a short press toggles the menu over a
    // covered app. Holding the key fills the progress ring; when the fill
    // completes the dash recenters in front of the head, whatever app is
    // up. Releasing early cancels and counts as a short press
    private void onSummon(int action) {
        if (action == KeyEvent.ACTION_DOWN) {
            summonDownAt = SystemClock.uptimeMillis();
            nativeHoldStart();
            if (covered && !summoned) {
                holdPreview = true;
                updateWindow();
            }
            view.postDelayed(holdFire, LONG_MS);
        } else if (action == KeyEvent.ACTION_UP && summonDownAt >= 0) {
            summonDownAt = -1;
            view.removeCallbacks(holdFire);
            nativeHoldEnd();
            if (holdPreview) { holdPreview = false; updateWindow(); }
            if (covered) {
                summoned = !summoned;
                updateWindow();
                if (summoned) nativeRecenter();
            }
        }
    }

    // the button stayed down through the whole fill: re-anchor the dash on
    // the head and make sure it's up so the recenter is visible
    private void onHoldDone() {
        summonDownAt = -1;
        nativeHoldEnd();
        nativeRecenter();
        if (covered && !summoned) summoned = true;
        if (holdPreview) holdPreview = false;
        updateWindow();
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

    // debug.vrhome.hold drives the real onSummon path: 1 = key down,
    // 0 = key up - exercises the hold ring and the long-press recenter
    public void debugSummonKey(final int action) {
        view.post(new Runnable() {
            @Override public void run() { onSummon(action); }
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

    // A surface born while the display was mid-rotation keeps the wrong
    // orientation: both eye halves then draw side by side on one side of
    // the panel and the dash looks doubled. Bounce the view so the surface
    // is recreated at the settled rotation; give up after a few tries in
    // case the rotation is real.
    private int surfBad;
    private final Runnable surfBounce = new Runnable() {
        @Override public void run() {
            if (view.getVisibility() != View.VISIBLE) return;
            view.setVisibility(View.GONE);
            view.post(new Runnable() {
                @Override public void run() {
                    view.setVisibility(View.VISIBLE);
                }
            });
        }
    };

    @Override public void surfaceChanged(SurfaceHolder h, int f, int w,
                                         int ht) {
        Display d = view.getDisplay();
        final int rot = d != null ? d.getRotation() : Surface.ROTATION_0;
        final boolean land = rot == Surface.ROTATION_0
                || rot == Surface.ROTATION_180;
        if ((w >= ht) == land) { surfBad = 0; return; }
        if (++surfBad > 4) return;
        Log.i(TAG, "surface " + w + "x" + ht + " vs rotation " + rot
                + " - recreating");
        view.removeCallbacks(surfBounce);
        view.postDelayed(surfBounce, 350);
    }

    @Override public void surfaceDestroyed(SurfaceHolder h) {
        view.removeCallbacks(surfBounce);
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

    // same self-enable trick as the key filter: the notification listener
    // binds once the secure setting names it, no settings trip needed
    private void enableNotifAccess() {
        try {
            final String svc = getPackageName() + "/"
                    + NotifService.class.getName();
            android.content.ContentResolver cr = getContentResolver();
            String cur = Settings.Secure.getString(cr,
                    "enabled_notification_listeners");
            if (cur == null || !cur.contains(svc)) {
                Settings.Secure.putString(cr,
                        "enabled_notification_listeners",
                        cur == null || cur.isEmpty() ? svc : cur + ":" + svc);
            }
            Log.i(TAG, "notif access enabled");
        } catch (Throwable t) {
            Log.e(TAG, "cannot enable notif listener", t);
        }
    }

    // --------------------------------------------------------- natives

    private static native void nativeInit(Context ctx, ShellBridge bridge);
    private static native void nativeWindow(Surface surface);
    private static native void nativeWindowGone();
    private static native void nativeKey(int code, int action, int repeat);
    private static native void nativeRecenter();
    private static native void nativeHoldStart();
    private static native void nativeHoldEnd();
    private static native void nativeShutdown();
}
