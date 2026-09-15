package gitlab.neosalsa.home;

import android.app.NativeActivity;
import android.content.Intent;
import android.util.Log;

// The shell's own activity on the physical display. Exists so HOME presses
// reach native code: when we are already the front activity, the framework
// delivers onNewIntent instead of a key event, and that is our recenter
// trigger.
public class PanelActivity extends NativeActivity {
    private static final String TAG = "vrhome.act";

    @Override protected void onNewIntent(Intent i) {
        super.onNewIntent(i);
        Log.i(TAG, "onNewIntent " + i.getAction());
        nativeHome();
    }

    private static native void nativeHome();
}
