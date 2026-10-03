package gitlab.neosalsa.hud;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;

import java.lang.reflect.Method;
import java.util.List;

/*
 * Keeps the env activity pinned as the only possible home app. The pin is
 * a persistent preferred activity - persistent prefs are the one
 * preference class the framework never clears when another launcher
 * installs, so the home-app chooser never gets a state in which it can
 * come up. Settings > Default apps goes through the role manager and
 * rewrites the preference, so the poll re-resolves HOME every few seconds
 * and re-pins the moment anything else owns the slot; a switch attempt
 * reverts within one tick.
 *
 * Hidden API access is expected like everywhere else in this package:
 * gitlab.neosalsa.hud is platform signed and on
 * hidden_api_blacklist_exemptions.
 */
public class HomeLock {
    private static final String TAG = "vrhud.homelock";
    private static final long TICK_MS = 5000;
    private static final ComponentName HOME = new ComponentName(
            "gitlab.neosalsa.home", "gitlab.neosalsa.home.PanelActivity");

    private final PackageManager pm;
    private final Handler main = new Handler(Looper.getMainLooper());
    private final Intent homeIntent = new Intent(Intent.ACTION_MAIN)
            .addCategory(Intent.CATEGORY_HOME);
    private final IntentFilter homeFilter = new IntentFilter(
            Intent.ACTION_MAIN);
    private Object ipm;
    private Method mSetHome;      // addPersistentPreferredActivity preferred,
    private Method mSetHomeAlt;   // setHomeActivity as the fallback
    // set once this process has written the persistent record itself:
    // resolving to the env alone is no proof - the role-holder write is a
    // regular preference the next launcher install can clear
    private boolean pinned;
    private boolean warned;

    public HomeLock(Context c) {
        pm = c.getPackageManager();
        homeFilter.addCategory(Intent.CATEGORY_HOME);
        homeFilter.addCategory(Intent.CATEGORY_DEFAULT);
        try {
            Class<?> sm = Class.forName("android.os.ServiceManager");
            IBinder b = (IBinder) sm.getMethod("getService", String.class)
                    .invoke(null, "package");
            Class<?> stub =
                    Class.forName("android.content.pm.IPackageManager$Stub");
            ipm = stub.getMethod("asInterface", IBinder.class)
                    .invoke(null, b);
            try {
                mSetHome = ipm.getClass().getMethod(
                        "addPersistentPreferredActivity",
                        IntentFilter.class, ComponentName.class, int.class);
            } catch (NoSuchMethodException e) {
                mSetHomeAlt = ipm.getClass().getMethod("setHomeActivity",
                        ComponentName.class, int.class);
            }
        } catch (Throwable t) {
            Log.e(TAG, "home pin api unavailable", t);
        }
        // a package transaction can flip home state either way: a fresh
        // launcher grabbing the slot, or the env app (un)installing
        IntentFilter pf = new IntentFilter();
        pf.addAction(Intent.ACTION_PACKAGE_ADDED);
        pf.addAction(Intent.ACTION_PACKAGE_REMOVED);
        pf.addAction(Intent.ACTION_PACKAGE_CHANGED);
        pf.addDataScheme("package");
        c.registerReceiver(pkgWatch, pf);
        enforce();
        main.postDelayed(tick, TICK_MS);
    }

    private final BroadcastReceiver pkgWatch = new BroadcastReceiver() {
        @Override public void onReceive(Context c, Intent i) { enforce(); }
    };

    private final Runnable tick = new Runnable() {
        @Override public void run() {
            enforce();
            main.postDelayed(this, TICK_MS);
        }
    };

    private void enforce() {
        try {
            if ((mSetHome == null && mSetHomeAlt == null)
                    || !envIsHome()) return;
            ResolveInfo ri = pm.resolveActivity(homeIntent,
                    PackageManager.MATCH_DEFAULT_ONLY);
            if (ri != null && isEnv(ri) && pinned) return;
            // someone else owns the slot: drop its preference first so the
            // re-pin is the only record on the filter
            if (ri != null && !isEnv(ri)) {
                pm.clearPackagePreferredActivities(
                        ri.activityInfo.packageName);
            }
            if (mSetHome != null) {
                mSetHome.invoke(ipm, homeFilter, HOME, 0);
            } else {
                mSetHomeAlt.invoke(ipm, HOME, 0);
            }
            pinned = true;
            Log.i(TAG, "home pinned to " + HOME.flattenToShortString());
        } catch (Throwable t) {
            if (!warned) {
                warned = true;
                Log.e(TAG, "home pin failed", t);
            }
        }
    }

    private boolean isEnv(ResolveInfo ri) {
        return ri.activityInfo != null
                && HOME.getPackageName().equals(ri.activityInfo.packageName)
                && HOME.getClassName().equals(ri.activityInfo.name);
    }

    // never pin HOME to a component the package manager cannot resolve:
    // on a boot where the env apk is missing that would leave the home key
    // dead
    private boolean envIsHome() {
        List<ResolveInfo> homes = pm.queryIntentActivities(homeIntent,
                PackageManager.MATCH_DEFAULT_ONLY);
        for (ResolveInfo h : homes) {
            if (isEnv(h)) return true;
        }
        return false;
    }
}
