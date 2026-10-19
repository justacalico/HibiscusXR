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
    // the settings card's block follows the quick panel's; slot order
    // matches src/set/item.h's UiSetStr
    public static final int SET_TITLE = QS_LABEL0 + 12;

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
            ctx.getString(R.string.qs_report),
            ctx.getString(R.string.qs_about),
            ctx.getString(R.string.set_title),
            ctx.getString(R.string.set_sec_wifi),
            ctx.getString(R.string.set_sec_bt),
            ctx.getString(R.string.set_sec_ctrl),
            ctx.getString(R.string.set_sec_disp),
            ctx.getString(R.string.set_sec_env),
            ctx.getString(R.string.set_sec_sound),
            ctx.getString(R.string.set_sec_lang),
            ctx.getString(R.string.set_sec_time),
            ctx.getString(R.string.set_sec_ime),
            ctx.getString(R.string.set_sec_dev),
            ctx.getString(R.string.set_sec_about),
            ctx.getString(R.string.set_l_wifi_on),
            ctx.getString(R.string.set_l_wifi_ssid),
            ctx.getString(R.string.set_l_wifi_list),
            ctx.getString(R.string.set_l_bt_on),
            ctx.getString(R.string.set_l_bt_list),
            ctx.getString(R.string.set_l_ctrl_scan),
            ctx.getString(R.string.set_l_ctrl_l),
            ctx.getString(R.string.set_l_ctrl_r),
            ctx.getString(R.string.set_l_ctrl_unbind),
            ctx.getString(R.string.set_l_bri),
            ctx.getString(R.string.set_l_theme),
            ctx.getString(R.string.set_l_ipd),
            ctx.getString(R.string.set_l_dof),
            ctx.getString(R.string.set_l_night),
            ctx.getString(R.string.set_l_env_list),
            ctx.getString(R.string.set_l_home_env),
            ctx.getString(R.string.set_l_vol),
            ctx.getString(R.string.set_l_mic),
            ctx.getString(R.string.set_l_lang),
            ctx.getString(R.string.set_l_tz),
            ctx.getString(R.string.set_l_ime_list),
            ctx.getString(R.string.set_l_adb),
            ctx.getString(R.string.set_l_stay),
            ctx.getString(R.string.set_l_touches),
            ctx.getString(R.string.set_l_dbghud),
            ctx.getString(R.string.set_l_brand),
            ctx.getString(R.string.set_l_model),
            ctx.getString(R.string.set_l_android),
            ctx.getString(R.string.set_l_hbver),
            ctx.getString(R.string.set_d_wifi_on),
            ctx.getString(R.string.set_d_wifi_ssid),
            ctx.getString(R.string.set_d_wifi_list),
            ctx.getString(R.string.set_d_bt_on),
            ctx.getString(R.string.set_d_bt_list),
            ctx.getString(R.string.set_d_ctrl_scan),
            ctx.getString(R.string.set_d_ctrl_l),
            ctx.getString(R.string.set_d_ctrl_r),
            ctx.getString(R.string.set_d_ctrl_unbind),
            ctx.getString(R.string.set_d_bri),
            ctx.getString(R.string.set_d_theme),
            ctx.getString(R.string.set_d_ipd),
            ctx.getString(R.string.set_d_dof),
            ctx.getString(R.string.set_d_night),
            ctx.getString(R.string.set_d_env_list),
            ctx.getString(R.string.set_d_home_env),
            ctx.getString(R.string.set_d_vol),
            ctx.getString(R.string.set_d_mic),
            ctx.getString(R.string.set_d_lang),
            ctx.getString(R.string.set_d_tz),
            ctx.getString(R.string.set_d_ime_list),
            ctx.getString(R.string.set_d_adb),
            ctx.getString(R.string.set_d_stay),
            ctx.getString(R.string.set_d_touches),
            ctx.getString(R.string.set_d_dbghud),
            ctx.getString(R.string.set_d_brand),
            ctx.getString(R.string.set_d_model),
            ctx.getString(R.string.set_d_android),
            ctx.getString(R.string.set_d_hbver),
            ctx.getString(R.string.set_v_connected),
            ctx.getString(R.string.set_v_disconnected),
            ctx.getString(R.string.set_v_pairing),
            ctx.getString(R.string.set_v_paired),
            ctx.getString(R.string.set_v_scan_wifi),
            ctx.getString(R.string.set_v_scan_bt),
            ctx.getString(R.string.set_v_scan_ctrl),
            ctx.getString(R.string.set_v_idle_wifi),
            ctx.getString(R.string.set_v_idle_bt),
            ctx.getString(R.string.set_v_idle_ctrl),
            ctx.getString(R.string.set_v_empty_wifi),
            ctx.getString(R.string.set_v_empty_bt),
            ctx.getString(R.string.set_v_empty_env),
            ctx.getString(R.string.set_v_secured),
            ctx.getString(R.string.set_v_opennet),
            ctx.getString(R.string.set_v_saved),
            ctx.getString(R.string.set_v_refresh),
            ctx.getString(R.string.set_v_pair),
            ctx.getString(R.string.set_v_forget),
            ctx.getString(R.string.set_v_connect),
            ctx.getString(R.string.set_v_cancel),
            ctx.getString(R.string.set_v_remove),
            ctx.getString(R.string.set_v_reboot),
            ctx.getString(R.string.set_v_join_fmt),
            ctx.getString(R.string.set_v_remove_fmt),
            ctx.getString(R.string.set_v_remove_body),
            ctx.getString(R.string.set_v_reboot_t),
            ctx.getString(R.string.set_v_reboot_b),
            ctx.getString(R.string.set_v_password),
            ctx.getString(R.string.set_v_unknown),
            ctx.getString(R.string.set_v_3dof),
            ctx.getString(R.string.set_v_6dof),
            ctx.getString(R.string.set_v_theme_dark),
            ctx.getString(R.string.set_v_theme_light),
            ctx.getString(R.string.set_v_theme_oled),
            ctx.getString(R.string.set_v_lang_en),
            ctx.getString(R.string.set_v_lang_zh),
            ctx.getString(R.string.set_v_env_pt),
            ctx.getString(R.string.set_v_env_pt_d),
            ctx.getString(R.string.set_v_env_builtin),
            ctx.getString(R.string.set_v_env_builtin_d),
            ctx.getString(R.string.set_v_env_bad),
            ctx.getString(R.string.set_v_env_nomap),
            ctx.getString(R.string.set_v_ime_active),
            ctx.getString(R.string.set_v_batt_fmt),
            ctx.getString(R.string.set_v_ctrl_chg),
            ctx.getString(R.string.set_v_space),
        };
    }
}
