package gitlab.neosalsa.hud;

import android.hardware.input.InputManager;
import android.os.SystemClock;
import android.util.Log;
import android.view.InputEvent;
import android.view.MotionEvent;

import java.lang.reflect.Method;

/*
 * Touch injection into the panel displays. Hidden API access is expected:
 * the app is platform signed and in hidden_api_blacklist_exemptions.
 */
class Injector {
    private static final String TAG = "vrhud.bridge";

    private final InputManager input;
    private final Method mSetDisplayId;
    private final Method mInject;

    // a drag is one gesture: MOVEs and the final UP keep the DOWN's downTime
    private long mDownTime = 0;

    Injector(InputManager i) throws Exception {
        input = i;
        mSetDisplayId = InputEvent.class.getDeclaredMethod("setDisplayId", int.class);
        mInject = InputManager.class.getDeclaredMethod("injectInputEvent",
                InputEvent.class, int.class);
    }

    void touch(int displayId, float x, float y, int action) {
        try {
            long now = SystemClock.uptimeMillis();
            if (action == MotionEvent.ACTION_DOWN || mDownTime == 0)
                mDownTime = now;
            MotionEvent ev = MotionEvent.obtain(mDownTime, now, action, x, y, 0);
            ev.setSource(android.view.InputDevice.SOURCE_TOUCHSCREEN);
            mSetDisplayId.invoke(ev, displayId);
            boolean ok = (Boolean) mInject.invoke(input, ev, 0);
            if (action != MotionEvent.ACTION_MOVE)
                Log.i(TAG, "inject " + action + " @" + (int)x + "," + (int)y +
                        " disp " + displayId + " -> " + ok);
            ev.recycle();
            if (action == MotionEvent.ACTION_UP ||
                    action == MotionEvent.ACTION_CANCEL)
                mDownTime = 0;
        } catch (Throwable t) {
            Log.e(TAG, "injectTouch", t);
        }
    }

    void tap(int displayId, float x, float y) {
        touch(displayId, x, y, MotionEvent.ACTION_DOWN);
        touch(displayId, x, y, MotionEvent.ACTION_UP);
    }
}
