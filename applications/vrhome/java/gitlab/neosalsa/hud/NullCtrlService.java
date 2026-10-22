package gitlab.neosalsa.hud;

/* No link service on this image: the settings card reads empty slots,
 * pairing writes and the keep-alive poke all do nothing. */
final class NullCtrlService implements CtrlService {
    @Override public void attach(Listener l) {}
    @Override public void detach() {}
    @Override public boolean bound() { return false; }
    @Override public void poll(SettingsState.CtrlSlot[] slots) {}
    @Override public void setPairing(boolean on) {}
    @Override public void unbindAll() {}
    @Override public void poke() {}
}
