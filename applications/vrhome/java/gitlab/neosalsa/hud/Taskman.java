package gitlab.neosalsa.hud;

import android.app.ActivityOptions;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.Set;

/*
 * The task side of the shell: the IActivityTaskManager proxy, the
 * RunningTaskInfo field handles and every operation that touches a task
 * or launches onto a display - launch, adopt, focus, kill. Hidden API
 * access is expected: the app is platform signed and in
 * hidden_api_blacklist_exemptions.
 */
class Taskman {
    private static final String TAG = "vrhud.bridge";

    private final Context ctx;
    private final PackageManager pm;
    private final Handler main;
    private final ShellBridge.CoveredListener listener;
    private final PanelDisplays displays;
    // shared with the poller: task ids mid adoption and the
    // display-teardown queue startWithRetry writes into when a launch
    // gives up. Both sit under the adopt queue's monitor
    private final Object adoptLock;
    private final Set<Integer> adopting;
    private final Queue<Integer> pendingReleases;

    // IActivityTaskManager proxy + the methods we use on it
    private Object atm;
    private Method mGetTasks, mRemoveTask, mSetFocusedTask, mMoveStack;
    private Method mSetLaunchDisplayId;

    private Field fTaskId, fStackId, fDisplayId, fTopActivity, fBaseActivity,
                  fBaseIntent, fNumActivities;

    private final Map<String, Boolean> vrCache = new HashMap<>();

    Taskman(Context c, Handler m, ShellBridge.CoveredListener l,
            PanelDisplays d, Object adoptLock, Set<Integer> adopting,
            Queue<Integer> pendingReleases) throws Exception {
        ctx = c;
        pm = c.getPackageManager();
        main = m;
        listener = l;
        displays = d;
        this.adoptLock = adoptLock;
        this.adopting = adopting;
        this.pendingReleases = pendingReleases;

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

        mSetLaunchDisplayId = ActivityOptions.class.getDeclaredMethod(
                "setLaunchDisplayId", int.class);
    }

    @SuppressWarnings("unchecked")
    List<Object> tasks() {
        try {
            return (List<Object>) mGetTasks.invoke(atm, 80);
        } catch (Throwable t) {
            return java.util.Collections.emptyList();
        }
    }

    String pkgOf(Object t) throws Exception {
        ComponentName top = (ComponentName) fTopActivity.get(t);
        if (top != null) return top.getPackageName();
        ComponentName base = (ComponentName) fBaseActivity.get(t);
        return base != null ? base.getPackageName() : null;
    }

    int taskId(Object t) throws Exception { return fTaskId.getInt(t); }

    int displayId(Object t) throws Exception { return fDisplayId.getInt(t); }

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
                synchronized (adoptLock) {
                    pendingReleases.add(displayId);
                }
            }
        }
    }

    void launchPackageOn(String pkg, int displayId) {
        try {
            Intent i = pm.getLaunchIntentForPackage(pkg);
            if (i == null) { Log.e(TAG, "no launch intent " + pkg); return; }
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            displays.markLaunching(displayId);
            startWithRetry(i, displayId, 3);
        } catch (Throwable t) {
            Log.e(TAG, "launchPackageOn " + pkg, t);
        }
    }

    // real Pico VR apps declare pvr.app.type=vr (or com.picovr.type=vr);
    // they drive the compositor directly and must own the physical display,
    // a virtual window can't host them
    boolean isVrApp(String pkg) {
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

    // isVrApp does binder calls; the poll hits every display-0 task each
    // 400ms, so cache the answer per package
    boolean vrApp(String pkg) {
        Boolean v = vrCache.get(pkg);
        if (v == null) {
            v = isVrApp(pkg);
            vrCache.put(pkg, v);
        }
        return v;
    }

    // the launcher rebuilt its app list: cached answers may name packages
    // that came or went, drop them
    void clearVrCache() { vrCache.clear(); }

    // VR apps launch plain on display 0: no panel, no display override.
    // One immersive app at a time - the Monado runtime lives in-process, so
    // two live XR apps would fight over the panel, the IMU and the Vulkan
    // device. A second launch kills the running one first; relaunching the
    // running one just refocuses it
    void launchVrApp(String pkg) {
        try {
            for (Object t : tasks()) {
                String p = pkgOf(t);
                if (p == null || p.equals(pkg) || !vrApp(p)) continue;
                Log.i(TAG, "closing vr app " + p + " for " + pkg);
                mRemoveTask.invoke(atm, fTaskId.getInt(t));
            }
            for (Object t : tasks()) {
                if (pkg.equals(pkgOf(t))) {
                    mSetFocusedTask.invoke(atm, fTaskId.getInt(t));
                    Log.i(TAG, "vr app " + pkg + " already running, focused");
                    if (listener != null) listener.onDismissMenu();
                    return;
                }
            }
            Intent i = pm.getLaunchIntentForPackage(pkg);
            if (i == null) { Log.e(TAG, "no launch intent " + pkg); return; }
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            ctx.startActivity(i);
            Log.i(TAG, "launched vr app " + pkg + " on display 0");
            if (listener != null) listener.onDismissMenu();
        } catch (Throwable t) {
            Log.e(TAG, "launchVrApp " + pkg, t);
        }
    }

    // a task that spawned on the physical display gets moved into a panel.
    // Prefer moveStackToDisplay (atomic, keeps the running activity); the
    // relaunch+remove path is only a fallback - it races when the system
    // retargets the still-living original task for the new intent
    void adoptTaskOn(int taskId, int displayId) {
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
            displays.markLaunching(displayId);
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
            synchronized (adoptLock) {
                adopting.remove(taskId);
            }
        }
    }

    void focusTask(int taskId) {
        try { mSetFocusedTask.invoke(atm, taskId); } catch (Throwable ignored) {}
    }

    void removeTask(int taskId) {
        try { mRemoveTask.invoke(atm, taskId); } catch (Throwable t) {
            Log.e(TAG, "removeTask " + taskId, t);
        }
    }

    // render thread: kill every task living on a panel's display. A panel's
    // cached taskId lies once adoption's relaunch path swaps the real task
    // underneath it - and launchPackageOn panels never learn one at all -
    // so the display binding is what a close can trust
    void removeTasksOnDisplay(int displayId) {
        try {
            for (Object t : tasks()) {
                if (fDisplayId.getInt(t) != displayId) continue;
                int id = fTaskId.getInt(t);
                mRemoveTask.invoke(atm, id);
                Log.i(TAG, "removed task " + id + " on display "
                        + displayId);
            }
        } catch (Throwable t) {
            Log.e(TAG, "removeTasksOnDisplay " + displayId, t);
        }
    }
}
