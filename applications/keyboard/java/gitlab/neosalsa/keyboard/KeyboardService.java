package gitlab.neosalsa.keyboard;

import android.app.Presentation;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.inputmethodservice.InputMethodService;
import android.inputmethodservice.Keyboard;
import android.inputmethodservice.KeyboardView;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.FrameLayout;
import android.view.Surface;

/*
 * The floating keyboard. Android 10 lets an IME window live only on the
 * target's own display or the default one, so a docked keyboard can never be
 * a separate window in front of the app. Instead the HUD owns a texture and
 * hands us its Surface over broadcast; we wrap it in our own private virtual
 * display (presentations are allowed on a private display owned by the
 * caller) and put the keys there as a Presentation. The HUD draws the
 * texture as a quad floating under the panel that owns the text field and
 * routes injected touches to the display id we report back. The docked IME
 * window itself stays a hairline on the app's display - it only exists
 * because InputMethodService needs a window object.
 *
 * If the HUD surface never arrives (shell not up yet, or presentation
 * refused) the docked hairline turns back into the real keyboard, docked at
 * the bottom of the app panel - the same place it sat before, still usable.
 */
public class KeyboardService extends InputMethodService
        implements KeyboardView.OnKeyboardActionListener {

    private static final String TAG = "pn2kbd";
    private static final String HUD_PKG = "gitlab.neosalsa.hud";
    // we -> HUD: the quad is visible, touches should land on this display
    private static final String ACTION_STATE = "gitlab.neosalsa.hud.action.KBD";
    // we -> HUD: please send the keyboard surface
    private static final String ACTION_QUERY = "gitlab.neosalsa.keyboard.action.QUERY";
    // HUD -> us: here is the surface (+ w/h/dpi extras)
    private static final String ACTION_SURFACE =
            "gitlab.neosalsa.keyboard.action.SURFACE";
    // HUD -> us: the shell's BACK wants the quad down
    private static final String ACTION_HIDE =
            "gitlab.neosalsa.keyboard.action.HIDE";

    private FrameLayout dockBox;      // inputView container on the app display
    private KeyboardView kv;          // the one view, reparented dock <-> quad
    private Keyboard letters, symbols;

    private Surface surf;
    private int surfSeq = -1;         // HUD's surface generation
    private int kbdW, kbdH, kbdDpi;
    private VirtualDisplay vd;
    private KbdPanel panel;
    private boolean floating;         // quad is live
    private boolean imeVisible;       // IME window is up on the app side

    private final BroadcastReceiver surfaceRecv = new BroadcastReceiver() {
        @Override public void onReceive(Context c, Intent i) {
            if (ACTION_HIDE.equals(i.getAction())) {
                requestHideSelf(0);
                return;
            }
            Surface s = i.getParcelableExtra("surface");
            if (s == null) return;
            final int seq = i.getIntExtra("seq", -1);
            if (seq != surfSeq) {
                surf = s;
                surfSeq = seq;
                kbdW = i.getIntExtra("w", 0);
                kbdH = i.getIntExtra("h", 0);
                kbdDpi = i.getIntExtra("dpi", 240);
                tearDownPanel();
            }
            if (imeVisible) showPanel();
        }
    };

    @Override public void onCreate() {
        super.onCreate();
        IntentFilter f = new IntentFilter();
        f.addAction(ACTION_SURFACE);
        f.addAction(ACTION_HIDE);
        registerReceiver(surfaceRecv, f);
        sendBroadcast(new Intent(ACTION_QUERY).setPackage(HUD_PKG));
    }

    @Override public void onDestroy() {
        unregisterReceiver(surfaceRecv);
        tearDownPanel();
        // a dead IME leaves a frozen quad up otherwise
        reportState(false);
        super.onDestroy();
    }

    @Override public View onCreateInputView() {
        letters = new Keyboard(this, R.xml.kbd_qwerty);
        symbols = new Keyboard(this, R.xml.kbd_symbols);
        kv = (KeyboardView) getLayoutInflater().inflate(R.layout.kbd, null);
        kv.setKeyboard(letters);
        kv.setOnKeyboardActionListener(this);
        kv.setPreviewEnabled(false);
        dockBox = new FrameLayout(this);
        dockBox.addView(kv);
        return dockBox;
    }

    @Override public boolean onEvaluateFullscreenMode() {
        // the quad floats over whatever the app is doing - never go
        // landscape-fullscreen extract mode
        return false;
    }

    @Override public void onStartInputView(EditorInfo info, boolean restarting) {
        super.onStartInputView(info, restarting);
        imeVisible = true;
        // re-query on every show: a HUD restart hands out a new surface and
        // the old VD would silently stop producing frames
        sendBroadcast(new Intent(ACTION_QUERY).setPackage(HUD_PKG));
        if (surf != null) showPanel();
    }

    @Override public void onFinishInputView(boolean finishingInput) {
        super.onFinishInputView(finishingInput);
        imeVisible = false;
        hidePanel();
    }

    @Override public void onWindowHidden() {
        super.onWindowHidden();
        imeVisible = false;
        hidePanel();
    }

    // ---------------------------------------------------------- quad side

    private void showPanel() {
        if (floating || kbdW <= 0 || kbdH <= 0) return;
        try {
            if (vd == null) {
                DisplayManager dm =
                        (DisplayManager) getSystemService(Context.DISPLAY_SERVICE);
                vd = dm.createVirtualDisplay("pn2kbd", kbdW, kbdH, kbdDpi,
                        surf, DisplayManager.VIRTUAL_DISPLAY_FLAG_PRESENTATION);
            }
            if (panel == null)
                panel = new KbdPanel(this, vd.getDisplay());
            if (!panel.isShowing()) panel.show();
            moveKeys(panel.box);
            floating = true;
            dockBox.setVisibility(View.GONE);
            reportState(true);
            Log.i(TAG, "floating on display " + vd.getDisplay().getDisplayId());
        } catch (Throwable t) {
            Log.w(TAG, "panel path failed, staying docked", t);
            floating = false;
            attachDocked();
            reportState(false);
        }
    }

    private void hidePanel() {
        if (!floating) { reportState(false); return; }
        floating = false;
        try { if (panel != null) panel.dismiss(); } catch (Throwable ignored) {}
        attachDocked();
        reportState(false);
    }

    private void tearDownPanel() {
        floating = false;
        try { if (panel != null) panel.dismiss(); } catch (Throwable ignored) {}
        panel = null;
        try { if (vd != null) vd.release(); } catch (Throwable ignored) {}
        vd = null;
        attachDocked();
    }

    private void attachDocked() {
        if (dockBox == null || kv == null) return;
        moveKeys(dockBox);
        dockBox.setVisibility(View.VISIBLE);
    }

    private void moveKeys(ViewGroup into) {
        if (kv == null) return;
        if (kv.getParent() == into) return;
        if (kv.getParent() != null)
            ((ViewGroup) kv.getParent()).removeView(kv);
        into.addView(kv);
    }

    private void reportState(boolean shown) {
        Intent i = new Intent(ACTION_STATE).setPackage(HUD_PKG)
                .putExtra("shown", shown)
                .putExtra("display", vd != null && shown
                        ? vd.getDisplay().getDisplayId() : -1);
        sendBroadcast(i);
    }

    // ---------------------------------------------------------- keys

    @Override public void onKey(int code, int[] codes) {
        InputConnection ic = getCurrentInputConnection();
        switch (code) {
            case Keyboard.KEYCODE_DELETE:
                if (ic != null) ic.deleteSurroundingText(1, 0);
                break;
            case Keyboard.KEYCODE_SHIFT:
                setCaps(!isCaps());
                break;
            case Keyboard.KEYCODE_MODE_CHANGE:
                kv.setKeyboard(kv.getKeyboard() == letters ? symbols : letters);
                break;
            case Keyboard.KEYCODE_DONE:
                sendDownUpKeyEvents(KeyEvent.KEYCODE_ENTER);
                break;
            case Keyboard.KEYCODE_CANCEL:
                requestHideSelf(0);
                break;
            default:
                char ch = (char) code;
                if (isCaps() && Character.isLetter(ch))
                    ch = Character.toUpperCase(ch);
                if (ic != null) ic.commitText(String.valueOf(ch), 1);
                break;
        }
    }

    private boolean isCaps() { return kv.getKeyboard() != null
            && kv.getKeyboard().isShifted(); }

    private void setCaps(boolean on) { letters.setShifted(on); kv.invalidateAllKeys(); }

    @Override public void onPress(int code) {}
    @Override public void onRelease(int code) {}
    @Override public void onText(CharSequence text) {
        InputConnection ic = getCurrentInputConnection();
        if (ic != null) ic.commitText(text, 1);
    }
    @Override public void swipeDown() {}
    @Override public void swipeUp() {}
    @Override public void swipeLeft() {}
    @Override public void swipeRight() {}

    // the presentation window fills our VD; NOT_FOCUSABLE keeps the panel
    // app's window as the focused one on its own display so the IME stays
    // bound to it
    static class KbdPanel extends Presentation {
        final FrameLayout box;
        KbdPanel(Context ctx, Display d) {
            super(ctx, d, R.style.KbdTheme);
            box = new FrameLayout(getContext());
        }
        @Override protected void onCreate(Bundle b) {
            super.onCreate(b);
            setContentView(box);
            Window w = getWindow();
            w.addFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
                    WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL);
        }
        @Override protected void onStart() {
            super.onStart();
            Window w = getWindow();
            w.setLayout(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT);
        }
    }
}
