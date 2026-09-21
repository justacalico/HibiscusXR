package gitlab.neosalsa.hud;

import android.accessibilityservice.AccessibilityService;
import android.util.Log;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityEvent;

/* Global key filter. Gesture monitors only ever see motion events on this
 * build, so the remapped summon key goes through the accessibility key
 * filter instead - it runs before normal dispatch and works no matter which
 * app owns the physical display. Returning true eats the key so a VR game
 * underneath never sees code 1003 at all.
 */
public class SummonKeyService extends AccessibilityService {
    private static final int K_SUMMON = 1003;      // DEFINE_HOME
    private static final int K_CONFIRM = 1001;     // DEFINE_CONFIRM

    @Override public boolean onKeyEvent(KeyEvent ev) {
        final int code = ev.getKeyCode();
        if (code == K_SUMMON) {
            Log.i("vrhud", "summon key act " + ev.getAction()
                    + " rep " + ev.getRepeatCount());
            if (ev.getAction() == KeyEvent.ACTION_DOWN
                    && ev.getRepeatCount() > 0)
                return true;   // repeats would re-stamp the long-press timer
            HudService.onSummonKey(ev.getAction());
            return true;
        }
        // while the HUD is shown, menu keys belong to it - the overlay
        // window never takes focus, so they arrive through here instead
        if (HudService.menuKeysOwned()
                && (code == K_CONFIRM || code == KeyEvent.KEYCODE_BACK)) {
            HudService.forwardKey(code, ev.getAction(),
                    ev.getRepeatCount());
            return true;
        }
        return false;
    }

    @Override public void onAccessibilityEvent(AccessibilityEvent ev) {}
    @Override public void onInterrupt() {}
}
