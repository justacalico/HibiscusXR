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
    // keep src/bridge/core.cpp syncUiStrings in step. The quick panel
    // block mirrors src/quick/item.h's UiQsStr: title, the state words,
    // then one label per tile in QsId order
    public static final int LIBRARY = 0;
    public static final int QS_TITLE = 1;
    public static final int QS_ON = 2;
    public static final int QS_OFF = 3;
    public static final int QS_NOTCONN = 4;
    public static final int QS_LABEL0 = 5;

    private static volatile int ver = 0;

    public static int version() { return ver; }

    public static void invalidate() { ++ver; }

    public static String[] snapshot(Context ctx) {
        return new String[]{
            ctx.getString(R.string.dock_library),
            ctx.getString(R.string.qs_title),
            ctx.getString(R.string.qs_on),
            ctx.getString(R.string.qs_off),
            ctx.getString(R.string.qs_not_connected),
            ctx.getString(R.string.qs_wifi),
            ctx.getString(R.string.qs_boundary),
            ctx.getString(R.string.qs_bluetooth),
            ctx.getString(R.string.qs_seethrough),
            ctx.getString(R.string.qs_mic),
            ctx.getString(R.string.qs_reset_view),
            ctx.getString(R.string.qs_night),
            ctx.getString(R.string.qs_dnd),
            ctx.getString(R.string.qs_airplane),
            ctx.getString(R.string.qs_saver),
            ctx.getString(R.string.qs_center),
            ctx.getString(R.string.qs_report),
            ctx.getString(R.string.qs_about),
        };
    }
}
