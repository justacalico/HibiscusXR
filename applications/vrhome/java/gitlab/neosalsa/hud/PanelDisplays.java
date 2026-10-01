package gitlab.neosalsa.hud;

import android.graphics.SurfaceTexture;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.os.SystemClock;
import android.util.Log;
import android.view.Surface;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/*
 * The panel virtual displays: SurfaceTexture -> Surface -> VirtualDisplay
 * triples the render thread creates and the poller reaps once no task
 * lives on them anymore.
 */
class PanelDisplays {
    private static final String TAG = "vrhud.bridge";

    // VIRTUAL_DISPLAY_FLAG_PUBLIC | VIRTUAL_DISPLAY_FLAG_SUPPORTS_TOUCH
    private static final int VD_FLAGS = 1 | 64;

    static class Vd {
        SurfaceTexture st;
        Surface surf;
        VirtualDisplay vd;
        long createdMs;
    }

    private final DisplayManager dm;
    private final Map<Integer, Vd> vds = new HashMap<>();
    // displays the render thread just launched something onto; don't reap
    // them while the task is still landing
    private final Set<Integer> launching = new java.util.HashSet<>();

    PanelDisplays(DisplayManager d) {
        dm = d;
    }

    void markLaunching(int displayId) { launching.add(displayId); }

    // Called on the render thread: texId must come from its GL context.
    int create(int texId, int w, int h, int dpi) {
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
            setDisplayIme(id, true);
            Log.i(TAG, "panel display id=" + id + " " + w + "x" + h);
            return id;
        } catch (Throwable t) {
            Log.e(TAG, "createPanel", t);
            return -1;
        }
    }

    // Mark a panel display as IME-capable so a focused window on it gets
    // the soft keyboard docked inside the panel instead of the keyboard
    // falling back to the physical display - mono in both eyes, which is
    // what "the keyboard breaks in VR" looked like. The call is
    // IWindowManager.setShouldShowIme, hidden and gated to
    // INTERNAL_SYSTEM_WINDOW callers plus a display owned by uid 1000;
    // both hold only because this apk runs under android.uid.system.
    private void setDisplayIme(int displayId, boolean on) {
        try {
            Class<?> wmg = Class.forName("android.view.WindowManagerGlobal");
            Object wms = wmg.getMethod("getWindowManagerService").invoke(null);
            wms.getClass()
                    .getMethod("setShouldShowIme", int.class, boolean.class)
                    .invoke(wms, displayId, on);
        } catch (Throwable t) {
            Log.w(TAG, "setShouldShowIme " + displayId + "=" + on +
                    " failed", t);
        }
    }

    SurfaceTexture texture(int displayId) {
        Vd v = vds.get(displayId);
        return v != null ? v.st : null;
    }

    void release(int displayId) {
        Vd v = vds.remove(displayId);
        launching.remove(displayId);
        if (v == null) return;
        // the flag is persisted per display uniqueId; clear it so the
        // display_settings.xml entry doesn't outlive the panel
        setDisplayIme(displayId, false);
        try { v.vd.release(); } catch (Throwable ignored) {}
        try { v.surf.release(); } catch (Throwable ignored) {}
        try { v.st.release(); } catch (Throwable ignored) {}
        Log.i(TAG, "released display " + displayId);
    }

    // queue a teardown for every display no live task sits on; a display
    // still inside its launch grace keeps breathing
    void reap(Set<Integer> liveDisplays, java.util.Queue<Integer> out) {
        long now = SystemClock.uptimeMillis();
        for (Map.Entry<Integer, Vd> e : vds.entrySet()) {
            int id = e.getKey();
            if (liveDisplays.contains(id)) { launching.remove(id); continue; }
            if (launching.contains(id) && now - e.getValue().createdMs < 6000)
                continue;   // task still landing on it
            launching.remove(id);
            out.add(id);
        }
    }
}
