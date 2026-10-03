package gitlab.neosalsa.hud;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.graphics.drawable.Drawable;
import android.hardware.display.DisplayManager;
import android.hardware.input.InputManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.BatteryManager;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/*
 * JNI facade for the panel shell, living in the HUD service: the render
 * thread calls in through the methods cached in bridge.cpp, so every
 * signature here is a hard contract. The work itself is split by
 * responsibility: DockPins (pin persistence), PanelDisplays (virtual
 * displays), Taskman (tasks + launches), Injector (touch injection) and
 * KbdLink (the floating keyboard surface). Hidden API access is expected:
 * the app is platform signed and gitlab.neosalsa.hud is in
 * hidden_api_blacklist_exemptions.
 *
 * Threading: createPanel, launch/adopt/release, the takePending getters and
 * the inject methods are all called from the render thread. The poller and
 * the open-package receiver run on the main looper.
 */
public class ShellBridge {
    private static final String TAG = "vrhud.bridge";
    // packages that may legitimately top display 0 without being "covered":
    // the env activity is the only one, everything else is a covered app.
    // Our own tasks never sit on display 0 but exclude this package anyway
    // so a stray can never be ourselves
    private static final String ENV_PKG = "gitlab.neosalsa.home";
    private static final String SELF = "gitlab.neosalsa.hud";
    // the standalone library app is deprecated: the app grid inside the
    // dash replaced it. Kept out of the grid itself so nothing launches a
    // leftover install
    private static final String LIB_PKG = "gitlab.neosalsa.library";

    // cross-process launch contract: the library app (or anything else on
    // a panel) asks the shell for a fresh window with a package-scoped
    // ordered broadcast; RESULT_OK tells the sender the launch was claimed
    public static final String ACTION_OPEN_PACKAGE =
            "gitlab.neosalsa.hud.action.OPEN_PACKAGE";
    public static final String EXTRA_PACKAGE = "package";

    public interface CoveredListener {
        void onCovered(boolean covered);
        // the menu should drop so the user lands inside the app - an
        // immersive launch or a dock tap on a live XR item
        void onDismissMenu();
        // the floating keyboard went up or down; over a covered app the
        // window has to come up for the quad to be visible at all
        void onKbd(boolean shown);
    }

    private final Context ctx;
    private final PackageManager pm;
    private final Handler main = new Handler(Looper.getMainLooper());
    private final CoveredListener listener;

    private final DockPins pins;
    private final PanelDisplays displays;
    private final Taskman taskman;
    private final Injector injector;
    private final KbdLink kbd;

    public static class Pending {
        public int taskId;
        public String pkg;
    }
    private final ArrayDeque<Pending> pendingAdopts = new ArrayDeque<>();
    private final ArrayDeque<Integer> pendingReleases = new ArrayDeque<>();
    private final Set<Integer> adopting = new HashSet<>();
    // dock state: immersive tasks on the physical display rebuilt every
    // poll (vrVer bumps on change so the render thread only pulls the
    // array when it moved)
    private final List<Pending> vrRunning = new ArrayList<>();
    private volatile int vrVer = 0;
    // set by the poller: a non-env task owns the physical display
    private volatile boolean covered = false;
    // the listener only hears CHANGES, so the first poll must fire
    // unconditionally - otherwise a service that starts while the env is
    // already front keeps its fail-hidden default and never shows
    private volatile boolean firstPoll = true;

    static {
        System.loadLibrary("vrhud");
    }

    public ShellBridge(Context c, CoveredListener l) throws Exception {
        ctx = c;
        listener = l;
        pm = c.getPackageManager();
        DisplayManager dm =
                (DisplayManager) c.getSystemService(Context.DISPLAY_SERVICE);
        InputManager input =
                (InputManager) c.getSystemService(Context.INPUT_SERVICE);

        pins = new DockPins(c, pm);
        displays = new PanelDisplays(dm);
        taskman = new Taskman(c, main, l, displays, pendingAdopts,
                adopting, pendingReleases);
        injector = new Injector(input);
        kbd = new KbdLink(c, l);

        ctx.registerReceiver(openReq, new IntentFilter(ACTION_OPEN_PACKAGE));
        IntentFilter pf = new IntentFilter();
        pf.addAction(Intent.ACTION_PACKAGE_ADDED);
        pf.addAction(Intent.ACTION_PACKAGE_REMOVED);
        pf.addAction(Intent.ACTION_PACKAGE_CHANGED);
        pf.addDataScheme("package");
        ctx.registerReceiver(pkgWatch, pf);
        kbd.register();
        main.postDelayed(poll, 800);
        Log.i(TAG, "bridge up");
    }

    // ---------------------------------------------------------- dock

    // render thread: the pin list when it changed since the last take,
    // else null. Called every frame, so keep it cheap when clean
    public String[] takePins() { return pins.take(); }

    // render thread: the dock long-press pushed a new pin list
    public void setPins(String[] pkgs) { pins.set(pkgs); }

    // render thread: app icon as a 96px ARGB bitmap for the dock texture,
    // or null - the dock falls back to a letter tile
    public Bitmap appIcon(String pkg) {
        try {
            Drawable d = pm.getApplicationIcon(pkg);
            Bitmap b = Bitmap.createBitmap(96, 96, Bitmap.Config.ARGB_8888);
            Canvas c = new Canvas(b);
            // every icon rides the same squircle tile: full-bleed bitmaps get
            // clipped to it, transparent glyphs sit on the plate
            Paint pt = new Paint(Paint.ANTI_ALIAS_FLAG);
            RectF box = new RectF(0.0f, 0.0f, 96.0f, 96.0f);
            float r = 96.0f * 0.22f;
            pt.setColor(0xFF262B33);
            c.drawRoundRect(box, r, r, pt);
            d.setBounds(0, 0, 96, 96);
            d.draw(c);
            pt.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DST_IN));
            c.drawRoundRect(box, r, r, pt);
            return b;
        } catch (Throwable t) {
            return null;
        }
    }

    // render thread: status bits for the dock cluster - wifi link state,
    // battery percent, charging flag. Local reads, cheap enough per frame
    public int[] sysStatus() {
        int wifi = 0, pct = 0, chg = 0;
        try {
            ConnectivityManager cm = (ConnectivityManager)
                ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo wi = cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
            if (wi != null && wi.isConnected()) wifi = 1;
        } catch (Throwable ignored) {}
        try {
            Intent batt = ctx.registerReceiver(null,
                new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
            if (batt != null) {
                int lv = batt.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
                int sc = batt.getIntExtra(BatteryManager.EXTRA_SCALE, 100);
                if (lv >= 0 && sc > 0) pct = lv * 100 / sc;
                int st = batt.getIntExtra(BatteryManager.EXTRA_STATUS, -1);
                if (st == BatteryManager.BATTERY_STATUS_CHARGING ||
                        st == BatteryManager.BATTERY_STATUS_FULL) chg = 1;
            }
        } catch (Throwable ignored) {}
        return new int[]{wifi, pct, chg};
    }

    // render thread: bumped when the immersive task set changes
    public int vrVersion() { return vrVer; }

    // render thread: immersive tasks on display 0 right now (at most one in
    // practice; the launch rule keeps it that way)
    public Pending[] runningVr() {
        synchronized (vrRunning) {
            return vrRunning.toArray(new Pending[0]);
        }
    }

    // render thread: drop the summoned menu so the user lands in the app
    public void dismissMenu() {
        if (listener != null) listener.onDismissMenu();
    }

    // ---------------------------------------------------------- notifs

    // set by the window logic: a toast window is up over a covered app,
    // so the render loop draws only the card stack - no panels, no dock
    private volatile boolean toastOnly;

    public void setToastOnly(boolean v) { toastOnly = v; }

    // render thread: card-stack-only render mode
    public boolean toastOnly() { return toastOnly; }

    // developer-settings debug line: debugHud mirrors the toggle itself;
    // debugOnly means the window is up over a covered app just for that
    // line, so the render loop skips the whole scene
    private volatile boolean debugHud;
    private volatile boolean debugOnly;

    public void setDebugHud(boolean v) { debugHud = v; }

    public void setDebugOnly(boolean v) { debugOnly = v; }

    // render thread: the settings toggle is on
    public boolean debugHud() { return debugHud; }

    // render thread: status-line-only render mode
    public boolean debugOnly() { return debugOnly; }

    // quick-settings "center new apps" toggle: while set the render loop
    // opens each window on the middle slot instead of filling left first
    private volatile boolean centerLaunch;

    public void setCenterLaunch(boolean v) { centerLaunch = v; }

    // render thread: the quick-settings toggle is on
    public boolean centerLaunch() { return centerLaunch; }

    // render thread: bumped on every post/removal by the listener
    public int notifVersion() { return NotifService.version(); }

    // ---------------------------------------------------------- app grid

    // one launchable app for the in-dash library overlay
    public static class LauncherApp {
        public String pkg;
        public String label;
    }
    private final List<LauncherApp> apps = new ArrayList<>();
    private volatile int appsVer = 0;
    private volatile boolean appsDirty = true;

    // render thread: bumped whenever the launchable package set changes
    public int appsVersion() { return appsVer; }

    // render thread: the launchable apps, sorted by label; the shell's own
    // pieces and the deprecated library app stay out
    public LauncherApp[] launcherApps() {
        synchronized (apps) {
            if (appsDirty) {
                appsDirty = false;
                apps.clear();
                Intent it = new Intent(Intent.ACTION_MAIN);
                it.addCategory(Intent.CATEGORY_LAUNCHER);
                List<ResolveInfo> rs = pm.queryIntentActivities(it, 0);
                for (ResolveInfo r : rs) {
                    if (r.activityInfo == null) continue;
                    String p = r.activityInfo.packageName;
                    if (p == null || p.equals(SELF) || p.equals(ENV_PKG) ||
                            p.equals(LIB_PKG) || p.equals(KbdLink.PKG))
                        continue;
                    LauncherApp a = new LauncherApp();
                    a.pkg = p;
                    try {
                        a.label = r.loadLabel(pm).toString();
                    } catch (Throwable t) {
                        a.label = p;
                    }
                    apps.add(a);
                }
                Collections.sort(apps,
                        (a, b) -> a.label.compareToIgnoreCase(b.label));
                taskman.clearVrCache();
            }
            return apps.toArray(new LauncherApp[0]);
        }
    }

    // main looper via onConfigurationChanged: app labels resolved under
    // the old locale re-resolve on the next launcherApps() pull
    public void invalidateAppLabels() {
        appsDirty = true;
        appsVer++;
    }

    private final BroadcastReceiver pkgWatch = new BroadcastReceiver() {
        @Override public void onReceive(Context c, Intent in) {
            appsDirty = true;
            appsVer++;
        }
    };

    // render thread: the live notification set, newest first
    public NotifService.Info[] notifs() { return NotifService.snapshot(); }

    // render thread: dismiss a card's notification back through the
    // listener; a non-clearable post is refused by the system itself
    public void dismissNotif(String key) { NotifService.cancel(key); }

    // ---------------------------------------------------------- sys msgs

    // set by the window logic: a system-message card is up over a covered
    // app, so the render loop draws only the dialog - no panels, no dock
    private volatile boolean sysMsgOnly;

    public void setSysMsgOnly(boolean v) { sysMsgOnly = v; }

    // render thread: dialog-only render mode
    public boolean sysMsgOnly() { return sysMsgOnly; }

    // render thread: bumped on every dropbox entry and card dismissal
    public int sysMsgVersion() { return SysMsgs.version(); }

    // render thread: the live system-message cards, oldest first
    public SysMsgs.Msg[] sysMsgs() { return SysMsgs.snapshot(); }

    // render thread: a card button - 0 closes, 1 restarts the app
    public void sysMsgClick(long id, int btn) { SysMsgs.click(id, btn); }

    // render thread: drop the card without acting on it
    public void sysMsgDismiss(long id) { SysMsgs.dismiss(id); }

    // -------------------------------------------------------- ui strings

    // render thread: bumps on a locale switch
    public int uiStringsVersion() { return UiStrings.version(); }

    // render thread: localized labels for chrome the GL side draws
    public String[] uiStrings() { return UiStrings.snapshot(ctx); }

    // ---------------------------------------------------------- displays

    // Called on the render thread: texId must come from its GL context.
    public int createPanel(int texId, int w, int h, int dpi) {
        return displays.create(texId, w, h, dpi);
    }

    public SurfaceTexture panelTexture(int displayId) {
        return displays.texture(displayId);
    }

    // display name for a panel's window bar
    public String appLabel(String pkg) {
        try {
            return pm.getApplicationLabel(
                    pm.getApplicationInfo(pkg, 0)).toString();
        } catch (Throwable t) {
            return pkg;
        }
    }

    public void releasePanel(int displayId) { displays.release(displayId); }

    // ---------------------------------------------------------- keyboard

    // render thread: create the shared surface; texId is a GL texture in
    // the render context. Idempotent - a HUD restart makes a new one and
    // the next QUERY reply carries it to the IME
    public boolean createKbdSurface(int texId, int w, int h, int dpi) {
        return kbd.createSurface(texId, w, h, dpi);
    }

    // render thread: the SurfaceTexture feeding the quad texture
    public SurfaceTexture kbdTexture() { return kbd.texture(); }

    // render thread: the IME asked for the surface since the last take
    public boolean takeKbdQuery() { return kbd.takeQuery(); }

    // send the surface over; the IME wraps it in a private virtual display
    // and shows the keys as a Presentation on it
    public void sendKbdSurface() { kbd.sendSurface(); }

    // render thread: ask the IME to put the quad away - BACK on a floating
    // keyboard should drop the keyboard, not the window behind it
    public void sendKbdHide() { kbd.sendHide(); }

    public void setKbdOnly(boolean v) { kbd.setOnly(v); }

    // render thread: {shown, displayId, kbdOnly}
    public int[] kbdState() { return kbd.state(); }

    // ---------------------------------------------------------- launches

    public void launchPackageOn(String pkg, int displayId) {
        taskman.launchPackageOn(pkg, displayId);
    }

    public boolean isVrApp(String pkg) { return taskman.isVrApp(pkg); }

    // VR apps launch plain on display 0: no panel, no display override
    public void launchVrApp(String pkg) { taskman.launchVrApp(pkg); }

    // render thread: true while something other than the env owns display 0
    public boolean isCovered() { return covered; }

    // a task that spawned on the physical display gets moved into a panel
    public void adoptTaskOn(int taskId, int displayId) {
        taskman.adoptTaskOn(taskId, displayId);
    }

    public void focusTask(int taskId) { taskman.focusTask(taskId); }

    public void removeTask(int taskId) { taskman.removeTask(taskId); }

    // render thread: kill every task living on a panel's display
    public void removeTasksOnDisplay(int displayId) {
        taskman.removeTasksOnDisplay(displayId);
    }

    // ---------------------------------------------------------- input

    public void injectTouch(int displayId, float x, float y, int action) {
        injector.touch(displayId, x, y, action);
    }

    public void injectTap(int displayId, float x, float y) {
        injector.tap(displayId, x, y);
    }

    // ---------------------------------------------------------- polling

    private final Runnable poll = new Runnable() {
        @Override public void run() {
            try { pollOnce(); } catch (Throwable t) { Log.e(TAG, "poll", t); }
            main.postDelayed(this, 400);
        }
    };

    private boolean ownPkg(String pkg) {
        return pkg.equals(ENV_PKG) || pkg.equals(SELF);
    }

    private void pollOnce() throws Exception {
        Set<Integer> liveDisplays = new HashSet<>();
        Set<Integer> liveTasks = new HashSet<>();
        final List<Object> tl = taskman.tasks();
        synchronized (pendingAdopts) {
            // the task list is MRU-ordered: the first display-0 entry is the
            // top one - anything but the env means an app owns the HMD. An
            // empty list means getTasks failed, not "nothing on display 0":
            // keep the last covered state instead of wiping a summon
            boolean top = !tl.isEmpty();
            for (Object t : tl) {
                int disp = taskman.displayId(t);
                if (disp != 0) continue;
                if (top) {
                    String p = taskman.pkgOf(t);
                    final boolean cov = p != null && !ownPkg(p);
                    if (cov != covered || firstPoll) {
                        covered = cov;
                        firstPoll = false;
                        if (listener != null) listener.onCovered(cov);
                    }
                    top = false;
                }
            }
            if (top) {
                if (covered || firstPoll) {
                    covered = false;
                    firstPoll = false;
                    if (listener != null) listener.onCovered(false);
                }
            }

            List<Pending> vr = new ArrayList<>();
            for (Object t : tl) {
                int taskId = taskman.taskId(t);
                int disp = taskman.displayId(t);
                liveTasks.add(taskId);
                String pkg = taskman.pkgOf(t);
                if (pkg == null) continue;
                if (taskman.vrApp(pkg)) {
                    // immersive tasks on the physical display feed the
                    // dock's running section - panels never host them
                    if (disp == 0) {
                        Pending p = new Pending();
                        p.taskId = taskId;
                        p.pkg = pkg;
                        vr.add(p);
                    }
                    continue;
                }
                if (disp == 0 && !ownPkg(pkg)
                        && !adopting.contains(taskId)) {
                    Pending p = new Pending();
                    p.taskId = taskId;
                    p.pkg = pkg;
                    pendingAdopts.add(p);
                    adopting.add(taskId);
                    Log.i(TAG, "stray task " + taskId + " " + pkg);
                } else if (disp != 0) {
                    liveDisplays.add(disp);
                }
            }
            adopting.retainAll(liveTasks);

            // publish the immersive set for the dock; bump the version only
            // on a real change so the render thread isn't rebuilding every
            // poll
            synchronized (vrRunning) {
                if (!sameVr(vr, vrRunning)) {
                    vrRunning.clear();
                    vrRunning.addAll(vr);
                    ++vrVer;
                }
            }

            displays.reap(liveDisplays, pendingReleases);
        }
    }

    private static boolean sameVr(List<Pending> a, List<Pending> b) {
        if (a.size() != b.size()) return false;
        for (int i = 0; i < a.size(); ++i)
            if (a.get(i).taskId != b.get(i).taskId) return false;
        return true;
    }

    // render thread: next stray task waiting for a panel, or null
    public Pending takePendingAdopt() {
        synchronized (pendingAdopts) {
            return pendingAdopts.poll();
        }
    }

    // render thread: next display id whose panel should be torn down, or -1
    public int takePendingRelease() {
        synchronized (pendingAdopts) {
            Integer id = pendingReleases.poll();
            return id != null ? id : -1;
        }
    }

    // ---------------------------------------------------------- entry points

    // package-scoped ordered broadcast from a panel app (the library): a
    // claimed launch answers RESULT_OK so the sender skips its own
    // startActivity, then queues the same render-thread path the old
    // in-apk grid used
    private final BroadcastReceiver openReq = new BroadcastReceiver() {
        @Override public void onReceive(Context c, Intent i) {
            String pkg = i.getStringExtra(EXTRA_PACKAGE);
            if (pkg == null || pkg.isEmpty()) return;
            setResultCode(Activity.RESULT_OK);
            openPackage(pkg);
        }
    };

    public static void openPackage(String pkg) {
        nativeQueueLaunch(pkg);
    }

    private static native void nativeQueueLaunch(String pkg);
}
