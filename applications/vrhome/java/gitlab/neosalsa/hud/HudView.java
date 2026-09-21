package gitlab.neosalsa.hud;

import android.content.Context;
import android.view.KeyEvent;
import android.widget.FrameLayout;

// Root view of the overlay window. While the HUD is shown the window is
// focusable, so headset keys land here; the service decides what each one
// does (summon/dismiss policy is java-side, panel input goes native).
public class HudView extends FrameLayout {
    public interface KeySink {
        boolean onKey(int code, int action, int repeat);
    }
    private final KeySink sink;

    public HudView(Context ctx, KeySink sink) {
        super(ctx);
        this.sink = sink;
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    @Override public boolean dispatchKeyEvent(KeyEvent ev) {
        if (sink.onKey(ev.getKeyCode(), ev.getAction(), ev.getRepeatCount()))
            return true;
        return super.dispatchKeyEvent(ev);
    }
}
