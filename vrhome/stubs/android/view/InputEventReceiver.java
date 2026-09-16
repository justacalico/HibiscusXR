package android.view;

import android.os.Looper;

// compile-time stub for the hidden framework class; only the signatures we
// call need to match the real ones
public abstract class InputEventReceiver {
    public InputEventReceiver(InputChannel inputChannel, Looper looper) {}
    public void onInputEvent(InputEvent event) {}
    public void finishInputEvent(InputEvent event, boolean handled) {}
    public void dispose() {}
}
