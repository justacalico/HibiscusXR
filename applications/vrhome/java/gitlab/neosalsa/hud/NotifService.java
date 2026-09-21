package gitlab.neosalsa.hud;

import android.app.Notification;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

/*
 * Notification feed for the shell. Enabled in secure settings by
 * HudService (platform signature carries WRITE_SECURE_SETTINGS), then the
 * system binds it and pushes posts/removals. The render thread never
 * talks to it directly: snapshot() hands out a stable array behind a
 * version counter, cancel() routes a dismiss back to the status bar.
 *
 * Our own foreground-service notification is filtered out - a permanent
 * "PN2 HUD" card on top of the stack would just be noise.
 */
public class NotifService extends NotificationListenerService {
    private static final String TAG = "vrhud.notif";
    private static final String SELF = "gitlab.neosalsa.hud";

    // one card's worth of data for the native side; public fields mirror
    // the Pending pattern ShellBridge already uses
    public static class Info {
        public String key;
        public String pkg;
        public String title;
        public String text;
        public long postMs;
        public boolean clearable;
    }

    private static final List<Info> cur = new ArrayList<>();
    private static volatile int ver = 0;
    private static volatile NotifService live;
    // shade entries that predate the listener are stale context, not
    // something to flash in the dash: only posts newer than the bind get
    // surfaced
    private static volatile long boundAt = Long.MAX_VALUE;
    // HudService hooks this so a fresh post can pop the toast window over
    // a covered app; a plain Runnable keeps NotifService off the window
    private static volatile Runnable poster;

    public static int version() { return ver; }

    public static Info[] snapshot() {
        synchronized (cur) {
            return cur.toArray(new Info[0]);
        }
    }

    public static void onPosted(Runnable r) { poster = r; }

    public static void cancel(String key) {
        NotifService s = live;
        if (s == null || key == null) return;
        try {
            s.cancelNotification(key);
        } catch (Throwable t) {
            Log.e(TAG, "cancel " + key, t);
        }
    }

    @Override public void onListenerConnected() {
        live = this;
        boundAt = System.currentTimeMillis();
        rebuild();
        Log.i(TAG, "notif listener bound");
    }

    @Override public void onListenerDisconnected() {
        live = null;
    }

    @Override public void onNotificationPosted(StatusBarNotification sbn) {
        rebuild();
        Runnable r = poster;
        if (r != null) r.run();
    }

    @Override public void onNotificationRemoved(StatusBarNotification sbn) {
        rebuild();
    }

    private void rebuild() {
        List<Info> out = new ArrayList<>();
        try {
            StatusBarNotification[] act = getActiveNotifications();
            if (act != null) {
                for (StatusBarNotification sbn : act) {
                    if (sbn == null || SELF.equals(sbn.getPackageName())
                            || sbn.getPostTime() <= boundAt)
                        continue;
                    Info i = new Info();
                    i.key = sbn.getKey();
                    i.pkg = sbn.getPackageName();
                    i.postMs = sbn.getPostTime();
                    i.clearable = sbn.isClearable();
                    Notification n = sbn.getNotification();
                    if (n != null && n.extras != null) {
                        CharSequence t = n.extras.getCharSequence(
                                Notification.EXTRA_TITLE);
                        CharSequence x = n.extras.getCharSequence(
                                Notification.EXTRA_TEXT);
                        i.title = t != null ? t.toString() : "";
                        i.text = x != null ? x.toString() : "";
                    }
                    out.add(i);
                }
            }
        } catch (Throwable t) {
            Log.e(TAG, "rebuild", t);
        }
        // newest first so the stack's top card is the freshest post
        Info[] arr = out.toArray(new Info[0]);
        Arrays.sort(arr, new Comparator<Info>() {
            @Override public int compare(Info a, Info b) {
                return Long.compare(b.postMs, a.postMs);
            }
        });
        synchronized (cur) {
            cur.clear();
            cur.addAll(Arrays.asList(arr));
        }
        ++ver;
    }
}
