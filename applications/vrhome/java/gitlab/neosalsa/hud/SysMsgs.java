package gitlab.neosalsa.hud;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.DropBoxManager;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

/*
 * Crash and ANR feed for the dash. The image sets hide_error_dialogs so a
 * mono "app keeps stopping" dialog never lands unwarped in both eyes - but
 * the same events still hit the DropBox, so this polls it and turns each
 * entry into a card the native side renders. A card carries real actions:
 * Close just drops it, Restart rides the normal launch path so a dead VR
 * app relaunches fullscreen and a dead 2D app comes back as a panel.
 *
 * Threading mirrors NotifService: the poll runs on the main looper, the
 * render thread reads through snapshot()/version(), and clicks come back
 * on the render thread via click()/dismiss().
 */
public class SysMsgs {
    private static final String TAG = "vrhud.sysmsg";
    // dropbox tags worth surfacing: app + server crashes, ANRs, WTFs and
    // native tombstones
    private static final String[] TAGS = {
        "system_app_crash", "system_app_anr", "system_app_wtf",
        "data_app_crash", "data_app_anr", "data_app_wtf",
        "system_server_crash", "system_server_anr",
        "system_server_native_crash", "system_tombstone",
    };
    private static final long POLL_MS = 2000;
    // the first poll looks back this far so a crash during boot still
    // surfaces once the service comes up
    private static final long SEED_BACK_MS = 5 * 60 * 1000;
    private static final int MAX_CARDS = 6;
    private static final int BODY_MAX = 64 * 1024;

    // one card's worth of data for the native side; buttons[0] is always
    // Close, an optional buttons[1] is Restart
    public static class Msg {
        public long id;
        public String pkg;
        public String title;
        public String text;
        public String[] buttons;
    }

    private static final List<Msg> cur = new ArrayList<>();
    private static final Handler main = new Handler(Looper.getMainLooper());
    private static volatile int ver = 0;
    // HudService hooks this so a fresh card pops the window over a covered
    // app; a plain Runnable keeps the feed off the window
    private static volatile Runnable listener;
    private static Context ctx;
    private static DropBoxManager db;
    private static PackageManager pm;
    // high-water mark of consumed entry times, persisted so a service
    // restart doesn't replay the same crashes
    private static long lastMs;
    private static long nextId = 1;
    private static boolean dead;

    public static void start(Context c) {
        ctx = c.getApplicationContext();
        db = (DropBoxManager) ctx.getSystemService(Context.DROPBOX_SERVICE);
        pm = ctx.getPackageManager();
        lastMs = ctx.getSharedPreferences("sysmsgs", 0).getLong("lastMs",
                System.currentTimeMillis() - SEED_BACK_MS);
        main.postDelayed(poll, 1500);
    }

    public static void onChanged(Runnable r) { listener = r; }

    public static int version() { return ver; }

    public static boolean hasMsgs() {
        synchronized (cur) { return !cur.isEmpty(); }
    }

    public static Msg[] snapshot() {
        synchronized (cur) { return cur.toArray(new Msg[0]); }
    }

    // render thread via the bridge: btn 0 closes the card, btn 1 relaunches
    // the app through the same queue the library panel uses - either way the
    // card is consumed
    public static void click(long id, int btn) {
        Msg m;
        synchronized (cur) {
            m = find(id);
            if (m == null) return;
            cur.remove(m);
        }
        ++ver;
        notifyChange();
        if (btn >= 1 && m.pkg != null && !m.pkg.isEmpty()) {
            Log.i(TAG, "restart " + m.pkg);
            ShellBridge.openPackage(m.pkg);
        }
    }

    public static void dismiss(long id) { click(id, 0); }

    private static Msg find(long id) {
        for (Msg m : cur)
            if (m.id == id) return m;
        return null;
    }

    private static void notifyChange() {
        Runnable r = listener;
        if (r != null) r.run();
    }

    // ------------------------------------------------------------ polling

    private static final Runnable poll = new Runnable() {
        @Override public void run() {
            scan();
            if (!dead) main.postDelayed(this, POLL_MS);
        }
    };

    private static void scan() {
        if (db == null || dead) return;
        try {
            for (String tag : TAGS) {
                for (;;) {
                    DropBoxManager.Entry e = db.getNextEntry(tag, lastMs);
                    if (e == null) break;
                    try {
                        ingest(tag, e);
                    } finally {
                        e.close();
                    }
                }
            }
            ctx.getSharedPreferences("sysmsgs", 0).edit()
                    .putLong("lastMs", lastMs).apply();
        } catch (Throwable t) {
            // READ_LOGS missing on a debug-signed build: stop polling
            // rather than spamming the log every two seconds
            dead = true;
            Log.e(TAG, "dropbox scan stopped", t);
        }
    }

    private static void ingest(String tag, DropBoxManager.Entry e) {
        final long t = e.getTimeMillis();
        if (t <= lastMs) return;
        lastMs = t;
        final String body = e.getText(BODY_MAX);
        final boolean anr = tag.contains("anr");
        Msg m = new Msg();
        m.id = nextId++;
        m.pkg = parsePkg(body);
        m.title = appLabel(m.pkg)
                + (anr ? " isn't responding" : " keeps stopping");
        m.text = faultLine(body);
        final boolean restart = !m.pkg.isEmpty()
                && pm.getLaunchIntentForPackage(m.pkg) != null;
        m.buttons = restart ? new String[]{"Close app", "Restart"}
                            : new String[]{"Close app"};
        synchronized (cur) {
            cur.add(m);
            while (cur.size() > MAX_CARDS) cur.remove(0);
        }
        ++ver;
        notifyChange();
        Log.i(TAG, "sys msg: " + m.title);
    }

    // crash and ANR entries carry a "Package: <pkg> v<ver> (<name>)" line;
    // tombstones and server records may only have a process name, so fall
    // back through Process: then Cmdline:
    private static String parsePkg(String body) {
        if (body == null) return "";
        String proc = "", cmd = "";
        for (String ln : body.split("\n")) {
            if (ln.startsWith("Package:")) {
                final String rest = ln.substring(8).trim();
                final int sp = rest.indexOf(' ');
                return sp > 0 ? rest.substring(0, sp) : rest;
            }
            if (proc.isEmpty() && ln.startsWith("Process:"))
                proc = ln.substring(8).trim();
            if (cmd.isEmpty() && ln.startsWith("Cmdline:"))
                cmd = ln.substring(8).trim();
        }
        String p = !proc.isEmpty() ? proc : cmd;
        // "com.foo:remote" is a process inside com.foo
        final int c = p.indexOf(':');
        return c > 0 ? p.substring(0, c) : p;
    }

    private static String appLabel(String pkg) {
        if (!pkg.isEmpty()) {
            try {
                return pm.getApplicationLabel(
                        pm.getApplicationInfo(pkg, 0)).toString();
            } catch (Throwable ignored) {}
        }
        return pkg.isEmpty() ? "System" : pkg;
    }

    // the card body: the first exception-ish line in the record, else the
    // first line that isn't a header field
    private static String faultLine(String body) {
        if (body == null) return "";
        final String[] lines = body.split("\n");
        for (String ln : lines) {
            final String s = ln.trim();
            if (s.contains("Exception") || s.contains("Error:"))
                return cut(s);
        }
        for (String ln : lines) {
            final String s = ln.trim();
            if (s.isEmpty() || s.startsWith("***") || s.startsWith("-----")
                    || s.startsWith("Process:") || s.startsWith("PID:")
                    || s.startsWith("UID:") || s.startsWith("Flags:")
                    || s.startsWith("Package:") || s.startsWith("Foreground:")
                    || s.startsWith("Build") || s.startsWith("Loop:")
                    || s.startsWith("Subject:") || s.startsWith("Cmdline:")
                    || s.startsWith("pid:") || s.startsWith("tid:")
                    || s.startsWith("signal") || s.startsWith("ABI")
                    || s.startsWith("Timestamp") || s.startsWith("Drop-Box"))
                continue;
            return cut(s);
        }
        return "";
    }

    private static String cut(String s) {
        return s.length() > 160 ? s.substring(0, 160) : s;
    }
}
