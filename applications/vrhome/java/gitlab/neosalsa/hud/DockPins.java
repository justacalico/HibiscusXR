package gitlab.neosalsa.hud;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/*
 * The dock's pin list, persisted in prefs. First boot seeds the settings
 * app so the strip is never empty - everything else pins/unpins from the
 * dock itself. Saved pins for apps that went away (the standalone library
 * app, an uninstall) drop off on load instead of drawing dead icons.
 */
class DockPins {
    // the settings app seeds the dock's pin list on first boot
    private static final String SETTINGS_PKG = "gitlab.neosalsa.settings";

    private final Context ctx;
    private final PackageManager pm;
    private final List<String> pins = new ArrayList<>();
    private volatile boolean dirty = true;

    DockPins(Context c, PackageManager p) {
        ctx = c;
        pm = p;
        load();
    }

    private void load() {
        SharedPreferences sp = ctx.getSharedPreferences("dock", 0);
        String saved = sp.getString("pins", null);
        synchronized (pins) {
            if (saved == null) {
                pins.add(SETTINGS_PKG);
                save();
            } else if (!saved.isEmpty()) {
                for (String p : saved.split(",")) {
                    if (pm.getLaunchIntentForPackage(p) != null)
                        pins.add(p);
                }
            }
        }
    }

    private void save() {
        StringBuilder b = new StringBuilder();
        synchronized (pins) {
            for (String p : pins) {
                if (b.length() > 0) b.append(',');
                b.append(p);
            }
        }
        ctx.getSharedPreferences("dock", 0).edit()
                .putString("pins", b.toString()).apply();
    }

    // render thread: the pin list when it changed since the last take,
    // else null. Called every frame, so keep it cheap when clean
    String[] take() {
        if (!dirty) return null;
        dirty = false;
        synchronized (pins) {
            return pins.toArray(new String[0]);
        }
    }

    // render thread: the dock long-press pushed a new pin list
    void set(String[] pkgs) {
        synchronized (pins) {
            pins.clear();
            pins.addAll(Arrays.asList(pkgs));
        }
        save();
    }
}
