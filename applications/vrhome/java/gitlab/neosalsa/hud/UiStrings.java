package gitlab.neosalsa.hud;

import android.content.Context;

/*
 * Localized labels the GL side draws itself. Same versioned-pull shape as
 * SysMsgs and NotifService: the render thread polls version() every frame
 * and refetches snapshot() on a bump. HudService.invalidate()s from
 * onConfigurationChanged so a locale switch reaches the dock without a
 * restart.
 */
public class UiStrings {
    // slots in the snapshot array; the native pull reads them by index,
    // keep src/bridge/bridge.cpp syncUiStrings in step
    public static final int LIBRARY = 0;

    private static volatile int ver = 0;

    public static int version() { return ver; }

    public static void invalidate() { ++ver; }

    public static String[] snapshot(Context ctx) {
        return new String[]{
            ctx.getString(R.string.dock_library),
        };
    }
}
