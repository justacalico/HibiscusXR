package gitlab.neosalsa.home;

import android.content.ComponentName;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;

// Tells the native side when a non-env task owns display 0 so the render
// loop can stop presenting. Same ActivityTaskManager reflection the HUD's
// ShellBridge uses, minus the virtual-display bookkeeping - the env only
// needs the top task's package.
public final class CoverWatch {
    private static final String TAG = "vrhome.cover";
    private static final String ENV = "gitlab.neosalsa.home";

    static { System.loadLibrary("vrhome"); }

    private static Object atm;
    private static Method mGetTasks;
    private static Field fDisplayId, fTopActivity, fBaseActivity;
    private static final Handler main = new Handler(Looper.getMainLooper());
    private static boolean started;

    private CoverWatch() {}

    public static synchronized void start() {
        if (started) return;
        started = true;
        try {
            Class<?> atmCls = Class.forName("android.app.ActivityTaskManager");
            atm = atmCls.getDeclaredMethod("getService").invoke(null);
            mGetTasks = atm.getClass().getMethod("getTasks", int.class);
            Class<?> rti = Class.forName(
                    "android.app.ActivityManager$RunningTaskInfo");
            fDisplayId = rti.getField("displayId");
            fTopActivity = rti.getField("topActivity");
            fBaseActivity = rti.getField("baseActivity");
        } catch (Throwable t) {
            Log.e(TAG, "ATM reflection failed", t);
            return;
        }
        main.postDelayed(poll, 800);
        Log.i(TAG, "cover watch up");
    }

    private static final Runnable poll = new Runnable() {
        @Override public void run() {
            try { pollOnce(); } catch (Throwable t) { Log.e(TAG, "poll", t); }
            main.postDelayed(this, 800);
        }
    };

    @SuppressWarnings("unchecked")
    private static void pollOnce() throws Exception {
        // the task list is MRU-ordered: the first display-0 entry owns the
        // physical panel
        for (Object t : (List<Object>) mGetTasks.invoke(atm, 80)) {
            if (fDisplayId.getInt(t) != 0) continue;
            ComponentName top = (ComponentName) fTopActivity.get(t);
            String pkg = top != null ? top.getPackageName() : null;
            if (pkg == null) {
                ComponentName base = (ComponentName) fBaseActivity.get(t);
                pkg = base != null ? base.getPackageName() : null;
            }
            nativeSetCovered(pkg != null && !pkg.equals(ENV));
            return;
        }
        nativeSetCovered(false);
    }

    private static native void nativeSetCovered(boolean covered);
}
