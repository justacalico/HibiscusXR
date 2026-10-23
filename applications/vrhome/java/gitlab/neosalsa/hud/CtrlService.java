package gitlab.neosalsa.hud;

import android.content.Context;

/*
 * The controller-link backend behind the settings card's Controllers
 * section and the render loop's keep-alive poke. Shared code only ever
 * sees this interface: detect() resolves whatever link service the
 * device's image ships and hands back that backend, or a no-op when the
 * image has none - the card then just shows two idle slots and the poke
 * silently does nothing.
 */
interface CtrlService {
    interface Listener { void onChange(); }

    static CtrlService detect(Context ctx) {
        CtrlService s = CvLink.probe(ctx);
        return s != null ? s : new NullCtrlService();
    }

    // bind to the link service; l fires on the main thread whenever
    // link state a snapshot shows may have changed
    void attach(Listener l);
    void detach();
    boolean bound();
    void poll(SettingsState.CtrlSlot[] slots);
    void setPairing(boolean on);
    void unbindAll();
    void poke();
    default void debugTransact(int code, int arg) {}
}
