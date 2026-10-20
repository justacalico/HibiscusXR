package gitlab.neosalsa.hud;

import android.content.Context;
import android.content.SharedPreferences;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/*
 * The library's app groups, persisted in prefs like the dock's pins. The
 * wire format is one encoded group per line - "name<TAB>pkg,pkg" - which
 * the render thread decodes on its side; groups save whole on every set.
 */
class LibGroups {
    private final Context ctx;
    private final List<String> lines = new ArrayList<>();
    private volatile boolean dirty = true;

    LibGroups(Context c) {
        ctx = c;
        load();
    }

    private void load() {
        SharedPreferences sp = ctx.getSharedPreferences("library", 0);
        String saved = sp.getString("groups", null);
        synchronized (lines) {
            if (saved == null) {
                save();
            } else {
                for (String l : saved.split("\n"))
                    if (!l.isEmpty()) lines.add(l);
            }
        }
    }

    private void save() {
        StringBuilder b = new StringBuilder();
        synchronized (lines) {
            for (String l : lines) {
                if (b.length() > 0) b.append('\n');
                b.append(l);
            }
        }
        ctx.getSharedPreferences("library", 0).edit()
                .putString("groups", b.toString()).apply();
    }

    // render thread: the encoded list when it changed since the last take,
    // else null. Called every frame, so keep it cheap when clean
    String[] take() {
        if (!dirty) return null;
        dirty = false;
        synchronized (lines) {
            return lines.toArray(new String[0]);
        }
    }

    // render thread: the library pushed a new group list
    void set(String[] enc) {
        synchronized (lines) {
            lines.clear();
            lines.addAll(Arrays.asList(enc));
        }
        save();
    }
}
