package gitlab.neosalsa.hud;

import android.app.Activity;
import android.app.ActivityOptions;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.SurfaceTexture;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.hardware.input.InputManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import android.view.InputEvent;
import android.view.MotionEvent;
import android.view.Surface;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayDeque;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/*
 * System-side plumbing for the panel shell, living in the HUD service now:
 * virtual displays, tasks and input injection all belong to the process
 * that renders them, and the HUD's overlay window is that process. Hidden
 * API access is expected: the app is platform signed and
 * gitlab.neosalsa.hud is in hidden_api_blacklist_exemptions.
 *
 * Threading: createPanel, launch/adopt/release, the takePending getters and
 * the inject methods are all called from the render thread. The poller and
 * the open-package receiver run on the main looper.
 */
public class ShellBridge {
    private static final String TAG = "vrhud.bridge";
    // packages that may legitimately top display 0 without being "covered":
    // the env activity is the only one, everything else is a covered app.
    // Our own tasks never sit on display 0 (the library lives on a panel)
    // but exclude this package anyway so a stray can never be ourselves
    private static final String ENV_PKG = "gitlab.neosalsa.home";
    private static final String SELF = "gitlab.neosalsa.hud";
    private static final String LIB_PKG = "gitlab.neosalsa.library";

    // cross-process launch contract: the library app (or anything else on
    // a panel) asks the shell for a fresh window with a package-scoped
    // ordered broadcast; RESULT_OK tells the sender the launch was claimed
    public static final String ACTION_OPEN_PACKAGE =
            "gitlab.neosalsa.hud.action.OPEN_PACKAGE";
    public static final String EXTRA_PACKAGE = "package";

    // VIRTUAL_DISPLAY_FLAG_PUBLIC | VIRTUAL_DISPLAY_FLAG_SUPPORTS_TOUCH
    private static final int VD_FLAGS = 1 | 64;

    public interface CoveredListener {
        void onCovered(boolean covered);
    }

    private final Context ctx;
    private final DisplayManager dm;
    private final PackageManager pm;
    private final Handler main = new Handler(Looper.getMainLooper());
    private final CoveredListener listener;

    // IActivityTaskManager proxy + the methods we use on it
    private Object atm;
    private Method mGetTasks, mRemoveTask, mSetFocusedTask, mMoveStack;
    private Method mSetDisplayId, mInject, mSetLaunchDisplayId;
    private InputManager input;

    private Field fTaskId, fStackId, fDisplayId, fTopActivity, fBaseActivity,
                  fBaseIntent, fNumActivities;

    static class Vd {
        SurfaceTexture st;
        Surface surf;
        VirtualDisplay vd;
        long createdMs;
    }
    private final Map<Integer, Vd> vds = new HashMap<>();

    public static class Pending {
        public int taskId;
        public String pkg;
    }
    private final ArrayDeque<Pending> pendingAdopts = new ArrayDeque<>();
    private final ArrayDeque<Integer> pendingReleases = new ArrayDeque<>();
    private final Set<Integer> adopting = new HashSet<>();
    // displays the render thread just launched something onto; don't reap
    // them while the task is still landing
    private final Set<Integer> launching = new HashSet<>();
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
        dm = (DisplayManager) c.getSystemService(Context.DISPLAY_SERVICE);
        pm = c.getPackageManager();
        input = (InputManager) c.getSystemService(Context.INPUT_SERVICE);

        Class<?> atmCls = Class.forName("android.app.ActivityTaskManager");
        atm = atmCls.getDeclaredMethod("getService").invoke(null);
        Class<?> proxy = atm.getClass();
        mGetTasks = proxy.getMethod("getTasks", int.class);
        mRemoveTask = proxy.getMethod("removeTask", int.class);
        mSetFocusedTask = proxy.getMethod("setFocusedTask", int.class);
        try {
            mMoveStack = proxy.getMethod("moveStackToDisplay",
                    int.class, int.class);
        } catch (NoSuchMethodException e) {
            mMoveStack = null;
        }

        Class<?> rti = Class.forName("android.app.ActivityManager$RunningTaskInfo");
        fTaskId = rti.getField("taskId");
        fStackId = rti.getField("stackId");
        fDisplayId = rti.getField("displayId");
        fTopActivity = rti.getField("topActivity");
        fBaseActivity = rti.getField("baseActivity");
        fBaseIntent = rti.getField("baseIntent");
        fNumActivities = rti.getField("numActivities");

        mSetDisplayId = InputEvent.class.getDeclaredMethod("setDisplayId", int.class);
        mInject = InputManager.class.getDeclaredMethod("injectInputEvent",
                InputEvent.class, int.class);
        mSetLaunchDisplayId = ActivityOptions.class.getDeclaredMethod(
                "setLaunchDisplayId", int.class);

        ctx.registerReceiver(openReq, new IntentFilter(ACTION_OPEN_PACKAGE));
        main.postDelayed(poll, 800);
        Log.i(TAG, "bridge up");
    }

    // ---------------------------------------------------------- displays

    // Called on the render thread: texId must come from its GL context.
    public int createPanel(int texId, int w, int h, int dpi) {
        try {
            SurfaceTexture st = new SurfaceTexture(texId);
            st.setDefaultBufferSize(w, h);
            Surface surf = new Surface(st);
            VirtualDisplay vd = dm.createVirtualDisplay("pn2panel",
                    w, h, dpi, surf, VD_FLAGS);
            if (vd == null) { surf.release(); st.release(); return -1; }
            int id = vd.getDisplay().getDisplayId();
            Vd v = new Vd();
            v.st = st; v.surf = surf; v.vd = vd;
            v.createdMs = SystemClock.uptimeMillis();
            vds.put(id, v);
            Log.i(TAG, "panel display id=" + id + " " + w + "x" + h);
            return id;
        } catch (Throwable t) {
            Log.e(TAG, "createPanel", t);
            return -1;
        }
    }

    public SurfaceTexture panelTexture(int displayId) {
        Vd v = vds.get(displayId);
        return v != null ? v.st : null;
    }

    // display name for a panel's window bar; the library gets a fixed label
    // so the pill matches whatever the app itself is called
    public String appLabel(String pkg) {
        if (LIB_PKG.equals(pkg)) return "Library";
        try {
            return pm.getApplicationLabel(
                    pm.getApplicationInfo(pkg, 0)).toString();
        } catch (Throwable t) {
            return pkg;
        }
    }

    public void releasePanel(int displayId) {
        Vd v = vds.remove(displayId);
        launching.remove(displayId);
        if (v == null) return;
        try { v.vd.release(); } catch (Throwable ignored) {}
        try { v.surf.release(); } catch (Throwable ignored) {}
        try { v.st.release(); } catch (Throwable ignored) {}
        Log.i(TAG, "released display " + displayId);
    }

    // ---------------------------------------------------------- launches

    private Bundle displayOpts(int displayId) throws Exception {
        ActivityOptions o = ActivityOptions.makeBasic();
        mSetLaunchDisplayId.invoke(o, displayId);
        return o.toBundle();
    }

    // ATMS on this build can NPE in ActivityRecord.computeBounds when an
    // activity lands on a not-yet-laid-out VD; mark the display busy up front
    // and retry briefly on the main thread
    private void startWithRetry(final Intent i, final int displayId,
                                final int tries) {
        try {
            ctx.startActivity(i, displayOpts(displayId));
            Log.i(TAG, "launched " + i.getComponent() + " on " + displayId);
        } catch (Throwable t) {
            if (tries > 0) {
                main.postDelayed(new Runnable() {
                    @Override public void run() {
                        startWithRetry(i, displayId, tries - 1);
                    }
                }, 350);
            } else {
                Log.e(TAG, "startActivity failed " + i.getComponent(), t);
                synchronized (pendingAdopts) {
                    pendingReleases.add(displayId);
                }
            }
        }
    }

    public void launchPackageOn(String pkg, int displayId) {
        try {
            Intent i = pm.getLaunchIntentForPackage(pkg);
            if (i == null) { Log.e(TAG, "no launch intent " + pkg); return; }
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            launching.add(displayId);
            startWithRetry(i, displayId, 3);
        } catch (Throwable t) {
            Log.e(TAG, "launchPackageOn " + pkg, t);
        }
    }

    // real Pico VR apps declare pvr.app.type=vr (or com.picovr.type=vr);
    // they drive the compositor directly and must own the physical display,
    // a virtual window can't host them
    public boolean isVrApp(String pkg) {
        try {
            ApplicationInfo ai = pm.getApplicationInfo(pkg,
                    PackageManager.GET_META_DATA);
            if (ai.metaData != null) {
                for (String key : new String[]{"pvr.app.type", "com.picovr.type"}) {
                    Object v = ai.metaData.get(key);
                    if (v != null && "vr".equalsIgnoreCase(String.valueOf(v)))
                        return true;
                }
            }
            // OpenXR-style apps mark their activity with an immersive
            // category instead of Pico's metadata; same rule applies.
            // The query must be implicit: getLaunchIntentForPackage sets a
            // component, and an explicit intent resolves by component with
            // the category ignored - that classified every app as VR and
            // sent all of them to display 0. Package-scoped + category, no
            // action so any activity in the package declaring it counts.
            for (String cat : new String[]{
                    "org.khronos.openxr.intent.category.IMMERSIVE_HMD",
                    "com.oculus.intent.category.VR"}) {
                Intent i = new Intent();
                i.addCategory(cat);
                i.setPackage(pkg);
                if (!pm.queryIntentActivities(i, 0).isEmpty())
                    return true;
            }
        } catch (Throwable ignored) {}
        return false;
    }

    // VR apps launch plain on display 0: no panel, no display override
    public void launchVrApp(String pkg) {
        try {
            Intent i = pm.getLaunchIntentForPackage(pkg);
            if (i == null) { Log.e(TAG, "no launch intent " + pkg); return; }
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            ctx.startActivity(i);
            Log.i(TAG, "launched vr app " + pkg + " on display 0");
        } catch (Throwable t) {
            Log.e(TAG, "launchVrApp " + pkg, t);
        }
    }

    // render thread: true while something other than the env owns display 0
    public boolean isCovered() { return covered; }

    // a task that spawned on the physical display gets moved into a panel.
    // Prefer moveStackToDisplay (atomic, keeps the running activity); the
    // relaunch+remove path is only a fallback - it races when the system
    // retargets the still-living original task for the new intent
    public void adoptTaskOn(int taskId, int displayId) {
        try {
            int stackId = -1;
            Intent i = null;
            String pkg = null;
            for (Object t : tasks()) {
                if (fTaskId.getInt(t) != taskId) continue;
                stackId = fStackId.getInt(t);
                Intent base = (Intent) fBaseIntent.get(t);
                if (base != null && base.getComponent() != null) {
                    i = new Intent(base);
                } else {
                    ComponentName b = (ComponentName) fBaseActivity.get(t);
                    if (b != null) {
                        pkg = b.getPackageName();
                        i = pm.getLaunchIntentForPackage(pkg);
                    }
                }
                break;
            }
            launching.add(displayId);
            if (stackId >= 0 && mMoveStack != null) {
                try {
                    mMoveStack.invoke(atm, stackId, displayId);
                    Log.i(TAG, "moved stack " + stackId + " (task " + taskId +
                            ") onto " + displayId);
                    return;
                } catch (Throwable moveErr) {
                    Log.w(TAG, "moveStackToDisplay failed, relaunching",
                            moveErr);
                }
            }
            if (i != null) {
                i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startWithRetry(i, displayId, 3);
                mRemoveTask.invoke(atm, taskId);
                Log.i(TAG, "adopted task " + taskId + " onto " + displayId);
            } else {
                // nothing to relaunch: kill it rather than leave a mono app
                // welded to the physical display
                mRemoveTask.invoke(atm, taskId);
                Log.w(TAG, "task " + taskId + " had no intent, removed");
            }
        } catch (Throwable t) {
            Log.e(TAG, "adoptTaskOn " + taskId, t);
        } finally {
            synchronized (pendingAdopts) {
                adopting.remove(taskId);
            }
        }
    }

    public void focusTask(int taskId) {
        try { mSetFocusedTask.invoke(atm, taskId); } catch (Throwable ignored) {}
    }

    public void removeTask(int taskId) {
        try { mRemoveTask.invoke(atm, taskId); } catch (Throwable t) {
            Log.e(TAG, "removeTask " + taskId, t);
        }
    }

    // ---------------------------------------------------------- input

    // a drag is one gesture: MOVEs and the final UP keep the DOWN's downTime
    private long mDownTime = 0;

    public void injectTouch(int displayId, float x, float y, int action) {
        try {
            long now = SystemClock.uptimeMillis();
            if (action == MotionEvent.ACTION_DOWN || mDownTime == 0)
                mDownTime = now;
            MotionEvent ev = MotionEvent.obtain(mDownTime, now, action, x, y, 0);
            ev.setSource(android.view.InputDevice.SOURCE_TOUCHSCREEN);
            mSetDisplayId.invoke(ev, displayId);
            boolean ok = (Boolean) mInject.invoke(input, ev, 0);
            if (action != MotionEvent.ACTION_MOVE)
                Log.i(TAG, "inject " + action + " @" + (int)x + "," + (int)y +
                        " disp " + displayId + " -> " + ok);
            ev.recycle();
            if (action == MotionEvent.ACTION_UP ||
                    action == MotionEvent.ACTION_CANCEL)
                mDownTime = 0;
        } catch (Throwable t) {
            Log.e(TAG, "injectTouch", t);
        }
    }

    public void injectTap(int displayId, float x, float y) {
        injectTouch(displayId, x, y, MotionEvent.ACTION_DOWN);
        injectTouch(displayId, x, y, MotionEvent.ACTION_UP);
    }

    // ---------------------------------------------------------- polling

    @SuppressWarnings("unchecked")
    private List<Object> tasks() {
        try {
            return (List<Object>) mGetTasks.invoke(atm, 80);
        } catch (Throwable t) {
            return java.util.Collections.emptyList();
        }
    }

    private String pkgOf(Object t) throws Exception {
        ComponentName top = (ComponentName) fTopActivity.get(t);
        if (top != null) return top.getPackageName();
        ComponentName base = (ComponentName) fBaseActivity.get(t);
        return base != null ? base.getPackageName() : null;
    }

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
        final List<Object> tl = tasks();
        synchronized (pendingAdopts) {
            // the task list is MRU-ordered: the first display-0 entry is the
            // top one - anything but the env means an app owns the HMD. An
            // empty list means getTasks failed, not "nothing on display 0":
            // keep the last covered state instead of wiping a summon
            boolean top = !tl.isEmpty();
            for (Object t : tl) {
                int disp = fDisplayId.getInt(t);
                if (disp != 0) continue;
                if (top) {
                    String p = pkgOf(t);
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

            for (Object t : tasks()) {
                int taskId = fTaskId.getInt(t);
                int disp = fDisplayId.getInt(t);
                liveTasks.add(taskId);
                String pkg = pkgOf(t);
                if (disp == 0 && pkg != null && !ownPkg(pkg)
                        && !isVrApp(pkg)      // VR keeps display 0
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

            long now = SystemClock.uptimeMillis();
            for (Map.Entry<Integer, Vd> e : vds.entrySet()) {
                int id = e.getKey();
                if (liveDisplays.contains(id)) { launching.remove(id); continue; }
                if (launching.contains(id) && now - e.getValue().createdMs < 6000)
                    continue;   // task still landing on it
                launching.remove(id);
                pendingReleases.add(id);
            }
        }
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
