package gitlab.neosalsa.hud;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.app.UiModeManager;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.BatteryManager;
import android.os.PowerManager;
import android.provider.Settings;
import android.util.Log;

import java.util.HashSet;
import java.util.Set;

/*
 * The quick panel's platform side. snap() reads everything fresh - the
 * render thread only pulls while the card is up, so there's no version
 * counter to keep warm. toggle()/setSlider() write straight to the
 * framework: the HUD runs as android.uid.system and platform-signed, so
 * the radio toggles the old flutter panel had to delegate to system
 * panels are direct calls here. Every write is wrapped - a denied call
 * leaves the tile's optimistic flip for the next snap() to correct.
 */
public class QuickState {
    private static final String TAG = "vrhud.qs";

    // tile ids mirror src/quick/item.h's QsId - keep the switch in step
    static final int WIFI = 0, BOUNDARY = 1, BLUETOOTH = 2,
            SEETHROUGH = 3, MIC = 4, RESET_VIEW = 5, NIGHT = 6, DND = 7,
            AIRPLANE = 8, SAVER = 9, REPORT = 10, ABOUT = 11;
    // slider ids mirror QsSlider
    static final int SLIDER_BRIGHTNESS = 0, SLIDER_VOLUME = 1;

    public static class Snap {
        public int toggles;
        public int net;         // uplink kind: 0 none, 1 wifi, 2 wired
        public int batt = -1;   // -1: the device has no battery
        public int chg;
        public float vol;
        public float bri;
        public String ssid;
        public String bt;
    }

    private final Context ctx;
    // connected bluetooth names ride the profile proxies, same way the
    // flutter panel warmed them
    private final Set<String> btNames = new HashSet<>();
    private final BluetoothProfile.ServiceListener profileListener =
            new BluetoothProfile.ServiceListener() {
        @Override public void onServiceConnected(int p,
                                                 BluetoothProfile proxy) {
            try {
                synchronized (btNames) {
                    for (BluetoothDevice d : proxy.getConnectedDevices())
                        if (d != null && d.getName() != null)
                            btNames.add(d.getName());
                }
            } catch (Throwable ignored) {}
        }
        @Override public void onServiceDisconnected(int p) {}
    };

    public QuickState(Context c) {
        ctx = c;
        try {
            BluetoothAdapter a = ((BluetoothManager)
                    c.getSystemService(Context.BLUETOOTH_SERVICE))
                    .getAdapter();
            if (a != null) {
                a.getProfileProxy(c, profileListener, BluetoothProfile.A2DP);
                a.getProfileProxy(c, profileListener, BluetoothProfile.HEADSET);
            }
        } catch (Throwable ignored) {}
    }

    private WifiManager wifi() {
        return (WifiManager) ctx.getApplicationContext()
                .getSystemService(Context.WIFI_SERVICE);
    }

    private AudioManager audio() {
        return (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
    }

    public Snap snap() {
        Snap s = new Snap();
        int m = 0;
        try {
            WifiManager w = wifi();
            if (w != null && w.isWifiEnabled()) m |= 1 << WIFI;
            if (w != null) {
                WifiInfo i = w.getConnectionInfo();
                String ssid = i == null ? null : i.getSSID();
                if (ssid != null) {
                    if (ssid.startsWith("\"") && ssid.endsWith("\""))
                        ssid = ssid.substring(1, ssid.length() - 1);
                    if (!ssid.isEmpty() && !ssid.equals("<unknown ssid>"))
                        s.ssid = ssid;
                }
            }
        } catch (Throwable ignored) {}
        try {
            ConnectivityManager cm = (ConnectivityManager)
                    ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo ei =
                    cm.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);
            if (ei != null && ei.isConnected()) s.net = 2;
            if (s.net == 0) {
                NetworkInfo wi =
                        cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
                if (wi != null && wi.isConnected()) s.net = 1;
            }
        } catch (Throwable ignored) {}
        try {
            BluetoothAdapter a = ((BluetoothManager)
                    ctx.getSystemService(Context.BLUETOOTH_SERVICE))
                    .getAdapter();
            if (a != null && a.isEnabled()) m |= 1 << BLUETOOTH;
            synchronized (btNames) {
                if (!btNames.isEmpty()) s.bt = btNames.iterator().next();
            }
        } catch (Throwable ignored) {}
        try {
            if (!audio().isMicrophoneMute()) m |= 1 << MIC;
        } catch (Throwable ignored) {}
        try {
            UiModeManager um = (UiModeManager)
                    ctx.getSystemService(Context.UI_MODE_SERVICE);
            if (um != null && um.getNightMode() ==
                    UiModeManager.MODE_NIGHT_YES) m |= 1 << NIGHT;
        } catch (Throwable ignored) {}
        try {
            NotificationManager nm = (NotificationManager)
                    ctx.getSystemService(Context.NOTIFICATION_SERVICE);
            if (nm != null && nm.getCurrentInterruptionFilter() !=
                    NotificationManager.INTERRUPTION_FILTER_ALL)
                m |= 1 << DND;
        } catch (Throwable ignored) {}
        try {
            if (Settings.Global.getInt(ctx.getContentResolver(),
                    Settings.Global.AIRPLANE_MODE_ON, 0) == 1)
                m |= 1 << AIRPLANE;
        } catch (Throwable ignored) {}
        try {
            PowerManager pm = (PowerManager)
                    ctx.getSystemService(Context.POWER_SERVICE);
            if (pm != null && pm.isPowerSaveMode()) m |= 1 << SAVER;
        } catch (Throwable ignored) {}
        try {
            Intent b = ctx.registerReceiver(null,
                    new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
            if (b != null && b.getBooleanExtra(
                    BatteryManager.EXTRA_PRESENT, true)) {
                s.batt = 0;
                int lv = b.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
                int sc = b.getIntExtra(BatteryManager.EXTRA_SCALE, 100);
                if (lv >= 0 && sc > 0) s.batt = lv * 100 / sc;
                int st = b.getIntExtra(BatteryManager.EXTRA_STATUS, -1);
                if (st == BatteryManager.BATTERY_STATUS_CHARGING ||
                        st == BatteryManager.BATTERY_STATUS_FULL) s.chg = 1;
            }
        } catch (Throwable ignored) {}
        try {
            AudioManager am = audio();
            int max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            if (max > 0)
                s.vol = am.getStreamVolume(AudioManager.STREAM_MUSIC) /
                        (float) max;
        } catch (Throwable ignored) {}
        try {
            s.bri = Settings.System.getInt(ctx.getContentResolver(),
                    Settings.System.SCREEN_BRIGHTNESS, 128) / 255.0f;
        } catch (Throwable ignored) {}
        s.toggles = m;
        return s;
    }

    public void toggle(int id, boolean on) {
        try {
            switch (id) {
            case WIFI: {
                WifiManager w = wifi();
                if (w != null) w.setWifiEnabled(on);
                break;
            }
            case BLUETOOTH: {
                BluetoothAdapter a = ((BluetoothManager)
                        ctx.getSystemService(Context.BLUETOOTH_SERVICE))
                        .getAdapter();
                if (a != null) {
                    if (on) a.enable(); else a.disable();
                }
                break;
            }
            case MIC:
                audio().setMicrophoneMute(!on);
                break;
            case NIGHT: {
                UiModeManager um = (UiModeManager)
                        ctx.getSystemService(Context.UI_MODE_SERVICE);
                if (um != null)
                    um.setNightMode(on ? UiModeManager.MODE_NIGHT_YES
                                       : UiModeManager.MODE_NIGHT_NO);
                break;
            }
            case DND: {
                NotificationManager nm = (NotificationManager)
                        ctx.getSystemService(Context.NOTIFICATION_SERVICE);
                if (nm != null)
                    nm.setInterruptionFilter(on
                            ? NotificationManager.INTERRUPTION_FILTER_NONE
                            : NotificationManager.INTERRUPTION_FILTER_ALL);
                break;
            }
            case AIRPLANE:
                Settings.Global.putInt(ctx.getContentResolver(),
                        Settings.Global.AIRPLANE_MODE_ON, on ? 1 : 0);
                ctx.sendBroadcast(
                        new Intent(Intent.ACTION_AIRPLANE_MODE_CHANGED)
                                .putExtra("state", on));
                break;
            case SAVER:
                // BatterySaverController observes the key and drives the
                // mode change itself
                Settings.Global.putInt(ctx.getContentResolver(),
                        "low_power_mode", on ? 1 : 0);
                break;
            default:
                break;
            }
        } catch (Throwable t) {
            Log.w(TAG, "toggle " + id + " denied", t);
        }
    }

    public void setSlider(int id, float v) {
        try {
            if (id == SLIDER_VOLUME) {
                AudioManager am = audio();
                int max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
                am.setStreamVolume(AudioManager.STREAM_MUSIC,
                        Math.round(v * max), 0);
            } else {
                Settings.System.putInt(ctx.getContentResolver(),
                        Settings.System.SCREEN_BRIGHTNESS,
                        Math.round(v * 255.0f));
            }
        } catch (Throwable t) {
            Log.w(TAG, "slider " + id + " denied", t);
        }
    }
}
