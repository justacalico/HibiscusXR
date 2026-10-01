package gitlab.neosalsa.hud;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.SurfaceTexture;
import android.os.SystemClock;
import android.util.Log;
import android.view.Surface;

/*
 * The floating-keyboard contract: the IME (gitlab.neosalsa.keyboard)
 * draws into a Surface we own and reports the display id it landed on so
 * the render thread can route touches. Android only lets a process put
 * presentation windows on a private display it owns, so the split is
 * fixed: surface/texture here, display + presentation there. No task
 * ever lives on it, so it stays out of the panel display reaper.
 */
class KbdLink {
    private static final String TAG = "vrhud.bridge";

    // the IME's package; the app grid filters it out too
    static final String PKG = "gitlab.neosalsa.keyboard";
    private static final String KBD_PKG = PKG;
    private static final String ACTION_KBD =
            "gitlab.neosalsa.hud.action.KBD";
    private static final String ACTION_KBD_QUERY =
            "gitlab.neosalsa.keyboard.action.QUERY";
    private static final String ACTION_KBD_SURFACE =
            "gitlab.neosalsa.keyboard.action.SURFACE";
    private static final String ACTION_KBD_HIDE =
            "gitlab.neosalsa.keyboard.action.HIDE";

    private final Context ctx;
    private final ShellBridge.CoveredListener listener;

    private SurfaceTexture kbdSt;
    private Surface kbdSurf;
    private int kbdW = -1, kbdH = -1, kbdDpi = 0;
    // the IME compares this instead of the Surface itself: each QUERY reply
    // parcels a fresh Surface object, so instance equality can't tell an
    // unchanged surface from a new one. uptimeMillis at creation time is
    // unique per surface instance across HUD restarts
    private int kbdSeq;
    private volatile boolean kbdShown;
    private volatile int kbdDisplay = -1;
    private volatile boolean kbdQuery;
    private volatile boolean kbdOnly;

    KbdLink(Context c, ShellBridge.CoveredListener l) {
        ctx = c;
        listener = l;
    }

    void register() {
        IntentFilter kf = new IntentFilter();
        kf.addAction(ACTION_KBD);
        kf.addAction(ACTION_KBD_QUERY);
        ctx.registerReceiver(kbdRecv, kf);
    }

    // render thread: create the shared surface; texId is a GL texture in
    // the render context. Idempotent - a HUD restart makes a new one and
    // the next QUERY reply carries it to the IME
    boolean createSurface(int texId, int w, int h, int dpi) {
        if (kbdSurf != null) return true;
        try {
            kbdSt = new SurfaceTexture(texId);
            kbdSt.setDefaultBufferSize(w, h);
            kbdSurf = new Surface(kbdSt);
            kbdW = w; kbdH = h; kbdDpi = dpi;
            kbdSeq = (int)(SystemClock.uptimeMillis() & 0x7fffffff);
            Log.i(TAG, "kbd surface " + w + "x" + h);
            return true;
        } catch (Throwable t) {
            Log.e(TAG, "createKbdSurface", t);
            return false;
        }
    }

    // render thread: the SurfaceTexture feeding the quad texture
    SurfaceTexture texture() { return kbdSt; }

    // render thread: the IME asked for the surface since the last take
    boolean takeQuery() {
        final boolean q = kbdQuery;
        kbdQuery = false;
        return q;
    }

    // send the surface over; the IME wraps it in a private virtual display
    // and shows the keys as a Presentation on it
    void sendSurface() {
        final Surface s = kbdSurf;
        if (s == null) return;
        ctx.sendBroadcast(new Intent(ACTION_KBD_SURFACE).setPackage(KBD_PKG)
                .putExtra("surface", s)
                .putExtra("w", kbdW).putExtra("h", kbdH)
                .putExtra("dpi", kbdDpi).putExtra("seq", kbdSeq));
    }

    // render thread: ask the IME to put the quad away - BACK on a floating
    // keyboard should drop the keyboard, not the window behind it
    void sendHide() {
        if (!kbdShown) return;
        ctx.sendBroadcast(new Intent(ACTION_KBD_HIDE).setPackage(KBD_PKG));
    }

    void setOnly(boolean v) { kbdOnly = v; }

    // render thread: {shown, displayId, kbdOnly} - only means the window is
    // up over a covered app solely for the quad, like toastOnly/sysMsgOnly
    int[] state() {
        return new int[]{kbdShown ? 1 : 0, kbdDisplay, kbdOnly ? 1 : 0};
    }

    private final BroadcastReceiver kbdRecv = new BroadcastReceiver() {
        @Override public void onReceive(Context c, Intent i) {
            final String a = i.getAction();
            if (ACTION_KBD.equals(a)) {
                kbdShown = i.getBooleanExtra("shown", false);
                kbdDisplay = i.getIntExtra("display", -1);
                if (listener != null) listener.onKbd(kbdShown);
            } else if (ACTION_KBD_QUERY.equals(a)) {
                // the surface may already exist; answer inline so the IME
                // doesn't wait on the render thread for the hand-off
                if (kbdSurf != null) sendSurface();
                else kbdQuery = true;
            }
        }
    };
}
