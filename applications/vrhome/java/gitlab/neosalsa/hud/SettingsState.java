package gitlab.neosalsa.hud;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.database.ContentObserver;
import android.media.AudioManager;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Parcel;
import android.os.PowerManager;
import android.provider.Settings;
import android.util.Log;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.List;
import java.util.Map;
import java.util.zip.ZipFile;

/*
 * The settings card's platform side. The HUD runs platform signed as
 * android.uid.system, so every write the old settings app routed through
 * a method channel is a direct call here. version() bumps on change;
 * the render thread pulls snap() while the card is up.
 */
public class SettingsState {
    private static final String TAG = "vrhud.set";

    static final int T_WIFI = 0, T_BT = 1, T_MIC = 2, T_NIGHT = 3,
            T_ADB = 4, T_STAY = 5, T_TOUCHES = 6, T_DEBUG_HUD = 7,
            T_DOF6 = 8;
    static final int SL_VOL = 0, SL_BRI = 1, SL_IPD = 2;
    static final int TX_SSID = 0, TX_THEME = 1, TX_ENV = 2, TX_LANG = 3,
            TX_MODEL = 4, TX_ANDROID = 5, TX_HBVER = 6;
    static final int SEC_WIFI = 0, SEC_BT = 1, SEC_CTRL = 2, SEC_ENV = 4;
    static final int FL_WIFI_SCAN = 1, FL_BT_SCAN = 2, FL_CTRL_SCAN = 4;
    static final int WSEC_OPEN = 0, WSEC_WEP = 1, WSEC_WPA = 2;

    private static final String SHOW_TOUCHES = "show_touches";
    private static final String DEBUG_HUD = "hibiscus_debug_hud";
    private static final String IPD = "hibiscus_ipd";
    private static final float IPD_DEFAULT_MM = 63.5f;
    private static final String DOF = "hibiscus_dof";
    private static final String THEME = "hibiscus_theme";
    private static final String HOME_ENV = "hibiscus_environment";
    private static final String SYSTEM_LOCALES = "system_locales";
    private static final String ENV_DIR = "/data/local/tmp/hibiscus/envs";
    private static final String HB_RELEASE = "/system/etc/hibiscus-release";
    private static final String XP_PREFIX = "hibiscus_xp_";

    // the Experimental section's registry: each entry is a feature flag
    // persisted in Settings.Global under XP_PREFIX + id and surfaced as
    // a row. Features land here as they ship.
    private static final class XpDef {
        final String id; final int nameRes, descRes;
        XpDef(String i, int n, int d) { id = i; nameRes = n; descRes = d; }
    }
    private static final XpDef[] XP_DEFS = {
        new XpDef("ffr", R.string.set_xp_ffr_name, R.string.set_xp_ffr_desc),
    };

    private static final String CTRL_SVC_PKG =
            "com.picovr.picovrlib.cvcontroller";
    private static final String CTRL_ACTION =
            "com.picovr.picovrlib.cvcontrollerlib.CVControllerAIDLService";
    private static final String CTRL_IFACE =
            "com.picovr.picovrlib.cvcontrollerlib.CVControllerAIDLService";
    private static final String CTRL_CB_IFACE =
            "com.picovr.picovrlib.cvcontrollerlib.ICVAIDLServiceCallback";
    private static final int TR_START_THREAD = 7, TR_STOP_THREAD = 8,
            TR_REGISTER_CB = 12, TR_CTRL_SN = 18, TR_UNBIND = 19,
            TR_PAIR_STATE = 25, TR_INTERRUPT_PAIR = 28,
            TR_DEV_BLE_MAC = 39, TR_CV2_CONN = 45, TR_START_PAIR = 50,
            TR_STOP_PAIR = 51, TR_CHARGING = 62, TR_BATTERY = 63;
    private static final int BATTERY_INDEX = 8;
    private static final long POLL_MS = 2000;
    private static final long SCAN_TIMEOUT_MS = 60_000;

    public static class Snap {
        public int toggles;
        public int flags;
        public float vol, bri, ipd;
        public String[] texts;
        public WifiNet[] wifi;
        public BtDev[] bt;
        public ImeEnt[] imes;
        public EnvEnt[] envs;
        public XpEnt[] xp;
        public CtrlSlot[] ctrl;
    }
    public static class WifiNet {
        public String ssid; public int sec; public int level;
        public boolean conn; public int saved;
    }
    public static class BtDev {
        public String name; public String addr;
        public boolean bonded; public boolean conn;
    }
    public static class ImeEnt {
        public String id; public String label; public boolean active;
    }
    public static class EnvEnt {
        public String id; public String name; public int st;
    }
    public static class XpEnt {
        public String id; public String name; public String desc;
        public boolean on;
    }
    public static class CtrlSlot {
        public int state; public int batt; public boolean chg;
        public String mac; public String serial;
    }

    private final Context ctx;
    private final Handler main = new Handler(Looper.getMainLooper());
    private volatile int ver;
    private int section = -1;

    private volatile boolean wifiScanning;
    private volatile boolean btDiscovering;
    private final Map<String, String> discovered = new LinkedHashMap<>();
    private final java.util.Set<String> aclLinked = new java.util.HashSet<>();

    private IBinder ctrlSvc;
    private ServiceConnection ctrlConn;
    private volatile boolean ctrlScanning;
    private final CtrlSlot[] ctrl = {new CtrlSlot(), new CtrlSlot()};

    private final Runnable ctrlPoll = new Runnable() {
        @Override public void run() {
            if (ctrlSvc == null || section < 0) return;
            pollCtrl();
            main.postDelayed(this, POLL_MS);
        }
    };
    private final Runnable ctrlScanOff = new Runnable() {
        @Override public void run() { ctrlScanning = false; bump(); }
    };

    private final android.os.Binder ctrlCb = new android.os.Binder() {
        @Override protected boolean onTransact(int code, Parcel data,
                Parcel reply, int flags) {
            try { data.enforceInterface(CTRL_CB_IFACE); }
            catch (Throwable ignored) {}
            return true;
        }
    };

    private final BroadcastReceiver rx = new BroadcastReceiver() {
        @Override public void onReceive(Context c, Intent i) {
            final String a = i.getAction();
            if (WifiManager.SCAN_RESULTS_AVAILABLE_ACTION.equals(a))
                wifiScanning = false;
            else if (BluetoothAdapter.ACTION_DISCOVERY_STARTED.equals(a)) {
                synchronized (discovered) { discovered.clear(); }
                btDiscovering = true;
            } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(a)) {
                btDiscovering = false;
            } else if (BluetoothDevice.ACTION_FOUND.equals(a)) {
                BluetoothDevice d =
                        i.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (d != null) {
                    String name = i.getStringExtra(BluetoothDevice.EXTRA_NAME);
                    if (name == null) name = safeName(d);
                    synchronized (discovered) {
                        discovered.put(d.getAddress(),
                                name != null ? name : d.getAddress());
                    }
                }
            } else if (BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(a)) {
                BluetoothDevice d =
                        i.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (d != null && i.getIntExtra(
                        BluetoothDevice.EXTRA_BOND_STATE,
                        BluetoothDevice.BOND_NONE) ==
                        BluetoothDevice.BOND_BONDED)
                    synchronized (discovered) {
                        discovered.remove(d.getAddress());
                    }
            } else if (BluetoothDevice.ACTION_ACL_CONNECTED.equals(a)) {
                BluetoothDevice d =
                        i.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (d != null) synchronized (aclLinked) {
                    aclLinked.add(d.getAddress());
                }
            } else if (BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(a)) {
                BluetoothDevice d =
                        i.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (d != null) synchronized (aclLinked) {
                    aclLinked.remove(d.getAddress());
                }
            }
            bump();
        }
    };

    private final ContentObserver obs =
            new ContentObserver(new Handler(Looper.getMainLooper())) {
        @Override public void onChange(boolean self) { bump(); }
    };

    SettingsState(Context c) {
        ctx = c;
        IntentFilter f = new IntentFilter();
        f.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        f.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        f.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
        f.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        f.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
        f.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
        f.addAction(BluetoothDevice.ACTION_FOUND);
        f.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        f.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        f.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        c.registerReceiver(rx, f);
        String[] keys = {Settings.System.SCREEN_BRIGHTNESS,
                AudioManager.STREAM_MUSIC + "", DEBUG_HUD, SHOW_TOUCHES,
                IPD, DOF, THEME, HOME_ENV,
                Settings.Global.STAY_ON_WHILE_PLUGGED_IN,
                Settings.Global.ADB_ENABLED, SYSTEM_LOCALES,
                Settings.Secure.DEFAULT_INPUT_METHOD,
                Settings.Secure.ENABLED_INPUT_METHODS};
        for (String k : keys)
            try {
                c.getContentResolver().registerContentObserver(
                        Settings.Global.getUriFor(k), false, obs);
            } catch (Throwable t) {
                try {
                    c.getContentResolver().registerContentObserver(
                            Settings.System.getUriFor(k), false, obs);
                } catch (Throwable t2) {
                    try {
                        c.getContentResolver().registerContentObserver(
                                Settings.Secure.getUriFor(k), false, obs);
                    } catch (Throwable ignored) {}
                }
            }
        for (String u : new String[]{"content://settings/system",
                "content://settings/secure", "content://settings/global"})
            c.getContentResolver().registerContentObserver(
                    android.net.Uri.parse(u), true, obs);
    }

    private void bump() { ver++; }

    public int version() { return ver; }

    public void section(int s) {
        section = s;
        bindCtrl(s >= 0);
        if (s == SEC_WIFI) wifiScan();
        else if (s == SEC_BT) btScan();
        bump();
    }

    public void closed() {
        section = -1;
        ctrlScanning = false;
        bindCtrl(false);
        bump();
    }

    // service teardown: drop the receiver, every settings observer, the
    // controller bind and the pending scan/poll callbacks
    public void close() {
        closed();
        try { ctx.unregisterReceiver(rx); } catch (Throwable ignored) {}
        ctx.getContentResolver().unregisterContentObserver(obs);
        main.removeCallbacks(ctrlPoll);
        main.removeCallbacks(ctrlScanOff);
    }

    // ----------------------------------------------------- snapshot

    public Snap snap() {
        Snap s = new Snap();
        final WifiManager wm = (WifiManager)
                ctx.getSystemService(Context.WIFI_SERVICE);
        final BluetoothAdapter ba = BluetoothAdapter.getDefaultAdapter();
        final AudioManager am = (AudioManager)
                ctx.getSystemService(Context.AUDIO_SERVICE);
        int t = 0;
        if (wm != null && wm.isWifiEnabled()) t |= 1 << T_WIFI;
        if (ba != null && ba.isEnabled()) t |= 1 << T_BT;
        if (am != null && !am.isMicrophoneMute()) t |= 1 << T_MIC;
        if (getInt(SHOW_TOUCHES, 0, "system") != 0) t |= 1 << T_TOUCHES;
        if (getGlobal(DEBUG_HUD, 0) != 0) t |= 1 << T_DEBUG_HUD;
        if ("6dof".equalsIgnoreCase(getGlobalStr(DOF, "")))
            t |= 1 << T_DOF6;
        s.toggles = t;
        s.vol = am == null ? 0.5f :
                (float) am.getStreamVolume(AudioManager.STREAM_MUSIC) /
                Math.max(1, am.getStreamMaxVolume(AudioManager.STREAM_MUSIC));
        s.bri = sysFloat(Settings.System.SCREEN_BRIGHTNESS, 128) / 255f;
        s.ipd = globalFloat(IPD, IPD_DEFAULT_MM);
        s.flags = (wifiScanning ? FL_WIFI_SCAN : 0)
                | (btDiscovering ? FL_BT_SCAN : 0)
                | (ctrlScanning ? FL_CTRL_SCAN : 0);
        String ssid = "";
        if (wm != null) {
            WifiInfo wi = wm.getConnectionInfo();
            if (wi != null && wi.getSSID() != null)
                ssid = wi.getSSID().replace("\"", "");
        }
        s.texts = new String[]{ssid,
                getGlobalStr(THEME, "dark"), getGlobalStr(HOME_ENV,
                "passthrough"), localeTag(), Build.MODEL,
                Build.VERSION.RELEASE, hbVer()};
        s.wifi = wifiNets(wm, ssid);
        s.bt = btDevs(ba);
        s.imes = imeList();
        s.envs = envList();
        s.xp = xpList();
        synchronized (ctrl) {
            s.ctrl = new CtrlSlot[]{copy(ctrl[0]), copy(ctrl[1])};
        }
        return s;
    }

    private static CtrlSlot copy(CtrlSlot c) {
        CtrlSlot o = new CtrlSlot();
        o.state = c.state; o.batt = c.batt; o.chg = c.chg;
        o.mac = c.mac; o.serial = c.serial;
        return o;
    }

    private int getGlobal(String key, int def) {
        try {
            return Settings.Global.getInt(ctx.getContentResolver(), key, def);
        } catch (Throwable t) { return def; }
    }

    private String getGlobalStr(String key, String def) {
        try {
            String v = Settings.Global.getString(ctx.getContentResolver(),
                    key);
            return v == null ? def : v;
        } catch (Throwable t) { return def; }
    }

    private int getInt(String key, int def, String which) {
        try {
            if ("system".equals(which))
                return Settings.System.getInt(ctx.getContentResolver(), key, def);
            return getGlobal(key, def);
        } catch (Throwable t) { return def; }
    }

    private float sysFloat(String key, float def) {
        try {
            return Settings.System.getFloat(ctx.getContentResolver(), key, def);
        } catch (Throwable t) { return def; }
    }

    private float globalFloat(String key, float def) {
        try {
            return Float.parseFloat(getGlobalStr(key, "" + def));
        } catch (Throwable t) { return def; }
    }

    private String localeTag() {
        String tag = getGlobalStr(SYSTEM_LOCALES, "");
        if (tag.isEmpty()) tag = Locale.getDefault().toLanguageTag();
        return tag;
    }

    private String hbVer() {
        try (BufferedReader r =
                new BufferedReader(new FileReader(HB_RELEASE))) {
            String line, out = "", bare = "";
            while ((line = r.readLine()) != null) {
                if (line.startsWith("VERSION="))
                    out = line.substring(8).replace("\"", "");
                else if (bare.isEmpty() && !line.trim().isEmpty())
                    bare = line.trim();
            }
            return out.isEmpty() ? bare : out;
        } catch (Throwable t) { return ""; }
    }

    // ----------------------------------------------------- wifi

    private static int secOf(ScanResult r) {
        if (r.capabilities == null) return WSEC_OPEN;
        if (r.capabilities.contains("WEP")) return WSEC_WEP;
        if (r.capabilities.contains("WPA") || r.capabilities.contains("PSK"))
            return WSEC_WPA;
        return WSEC_OPEN;
    }

    private WifiNet[] wifiNets(WifiManager wm, String connSsid) {
        List<WifiNet> out = new ArrayList<>();
        if (wm == null) return out.toArray(new WifiNet[0]);
        Map<String, Integer> saved = new LinkedHashMap<>();
        try {
            List<WifiConfiguration> cfgs = wm.getConfiguredNetworks();
            if (cfgs != null)
                for (WifiConfiguration c : cfgs)
                    if (c.SSID != null)
                        saved.put(c.SSID.replace("\"", ""), c.networkId);
        } catch (Throwable ignored) {}
        try {
            List<ScanResult> rs = wm.getScanResults();
            if (rs != null)
                for (ScanResult r : rs) {
                    if (r.SSID == null || r.SSID.isEmpty()) continue;
                    WifiNet n = new WifiNet();
                    n.ssid = r.SSID;
                    n.sec = secOf(r);
                    n.level = WifiManager.calculateSignalLevel(r.level, 5);
                    n.conn = r.SSID.equals(connSsid);
                    Integer id = saved.remove(r.SSID);
                    n.saved = id == null ? -1 : id;
                    out.add(n);
                }
        } catch (Throwable ignored) {}
        for (Map.Entry<String, Integer> en : saved.entrySet()) {
            WifiNet n = new WifiNet();
            n.ssid = en.getKey();
            n.sec = WSEC_WPA;
            n.level = -1;
            n.conn = false;
            n.saved = en.getValue();
            out.add(n);
        }
        return out.toArray(new WifiNet[0]);
    }

    public void wifiScan() {
        WifiManager wm = (WifiManager)
                ctx.getSystemService(Context.WIFI_SERVICE);
        if (wm == null || !wm.isWifiEnabled()) return;
        try {
            wifiScanning = wm.startScan();
        } catch (Throwable t) { wifiScanning = false; }
        bump();
    }

    public void wifiJoin(String ssid, int sec, String pw) {
        WifiManager wm = (WifiManager)
                ctx.getSystemService(Context.WIFI_SERVICE);
        if (wm == null) return;
        try {
            List<WifiConfiguration> cfgs = wm.getConfiguredNetworks();
            if (cfgs != null)
                for (WifiConfiguration c : cfgs) {
                    if (c.SSID == null) continue;
                    if (c.SSID.replace("\"", "").equals(ssid)) {
                        wm.enableNetwork(c.networkId, true);
                        bump();
                        return;
                    }
                }
            WifiConfiguration c = new WifiConfiguration();
            c.SSID = "\"" + ssid + "\"";
            if (sec == WSEC_OPEN) c.allowedKeyManagement.set(
                    WifiConfiguration.KeyMgmt.NONE);
            else {
                c.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
                if (pw != null && !pw.isEmpty())
                    c.preSharedKey = "\"" + pw + "\"";
            }
            int id = wm.addNetwork(c);
            if (id >= 0) {
                wm.enableNetwork(id, true);
                wm.reconnect();
            }
        } catch (Throwable t) {
            Log.w(TAG, "wifiJoin failed", t);
        }
        bump();
    }

    public void wifiForget(int netId) {
        WifiManager wm = (WifiManager)
                ctx.getSystemService(Context.WIFI_SERVICE);
        if (wm == null) return;
        try { wm.removeNetwork(netId); wm.reconnect(); }
        catch (Throwable ignored) {}
        bump();
    }

    // ----------------------------------------------------- bt

    private static String safeName(BluetoothDevice d) {
        try { return d.getName(); } catch (Throwable t) { return null; }
    }

    private BtDev[] btDevs(BluetoothAdapter ba) {
        List<BtDev> out = new ArrayList<>();
        if (ba == null) return out.toArray(new BtDev[0]);
        java.util.Set<String> linked;
        synchronized (aclLinked) { linked = new java.util.HashSet<>(aclLinked); }
        try {
            for (BluetoothDevice d : ba.getBondedDevices()) {
                BtDev b = new BtDev();
                b.name = safeName(d);
                if (b.name == null) b.name = "";
                b.addr = d.getAddress();
                b.bonded = true;
                b.conn = linked.contains(d.getAddress());
                synchronized (discovered) { discovered.remove(b.addr); }
                out.add(b);
            }
        } catch (Throwable ignored) {}
        synchronized (discovered) {
            for (Map.Entry<String, String> en : discovered.entrySet()) {
                BtDev b = new BtDev();
                b.name = en.getValue();
                b.addr = en.getKey();
                b.bonded = false;
                b.conn = linked.contains(b.addr);
                out.add(b);
            }
        }
        return out.toArray(new BtDev[0]);
    }

    public void btScan() {
        BluetoothAdapter ba = BluetoothAdapter.getDefaultAdapter();
        if (ba == null || !ba.isEnabled()) return;
        try {
            if (ba.isDiscovering()) ba.cancelDiscovery();
            ba.startDiscovery();
        } catch (Throwable ignored) {}
        bump();
    }

    public void btPair(String addr) {
        BluetoothAdapter ba = BluetoothAdapter.getDefaultAdapter();
        if (ba == null) return;
        try {
            BluetoothDevice d = ba.getRemoteDevice(addr);
            if (d != null) d.createBond();
        } catch (Throwable ignored) {}
        bump();
    }

    public void btUnpair(String addr) {
        BluetoothAdapter ba = BluetoothAdapter.getDefaultAdapter();
        if (ba == null) return;
        try {
            BluetoothDevice d = ba.getRemoteDevice(addr);
            if (d != null)
                d.getClass().getMethod("removeBond").invoke(d);
            synchronized (aclLinked) { aclLinked.remove(addr); }
        } catch (Throwable ignored) {}
        bump();
    }

    // ----------------------------------------------------- ime / env

    private ImeEnt[] imeList() {
        InputMethodManager imm = (InputMethodManager)
                ctx.getSystemService(Context.INPUT_METHOD_SERVICE);
        List<ImeEnt> out = new ArrayList<>();
        if (imm == null) return out.toArray(new ImeEnt[0]);
        String cur = Settings.Secure.getString(ctx.getContentResolver(),
                Settings.Secure.DEFAULT_INPUT_METHOD);
        try {
            for (InputMethodInfo i : imm.getEnabledInputMethodList()) {
                ImeEnt e = new ImeEnt();
                e.id = i.getId();
                CharSequence l = i.loadLabel(ctx.getPackageManager());
                e.label = l == null ? e.id : l.toString();
                e.active = e.id.equals(cur);
                out.add(e);
            }
        } catch (Throwable ignored) {}
        return out.toArray(new ImeEnt[0]);
    }

    public void imePick(String id) {
        try {
            Settings.Secure.putString(ctx.getContentResolver(),
                    Settings.Secure.DEFAULT_INPUT_METHOD, id);
            String en = Settings.Secure.getString(ctx.getContentResolver(),
                    Settings.Secure.ENABLED_INPUT_METHODS);
            if (en == null || !en.contains(id))
                Settings.Secure.putString(ctx.getContentResolver(),
                        Settings.Secure.ENABLED_INPUT_METHODS,
                        en == null || en.isEmpty() ? id : en + ":" + id);
        } catch (Throwable ignored) {}
        bump();
    }

    private EnvEnt[] envList() {
        List<EnvEnt> out = new ArrayList<>();
        File[] files = new File(ENV_DIR).listFiles();
        if (files == null) return out.toArray(new EnvEnt[0]);
        for (File f : files) {
            if (!f.getName().endsWith(".zip")) continue;
            EnvEnt e = new EnvEnt();
            e.id = f.getName().substring(0, f.getName().length() - 4);
            e.name = e.id;
            e.st = envStatus(f);
            out.add(e);
        }
        return out.toArray(new EnvEnt[0]);
    }

    private static int envStatus(File f) {
        try {
            ZipFile z = new ZipFile(f);
            boolean hasMap = z.getEntry("map.obj") != null;
            boolean hasMeta = z.getEntry("meta.ini") != null;
            z.close();
            return hasMap ? 0 : hasMeta ? 2 : 1;
        } catch (Throwable t) { return 1; }
    }

    public void envDelete(String id) {
        try { new File(ENV_DIR, id + ".zip").delete(); }
        catch (Throwable ignored) {}
        bump();
    }

    // ----------------------------------------------------- experimental

    private XpEnt[] xpList() {
        XpEnt[] out = new XpEnt[XP_DEFS.length];
        for (int i = 0; i < XP_DEFS.length; ++i) {
            XpDef d = XP_DEFS[i];
            XpEnt e = new XpEnt();
            e.id = d.id;
            e.name = ctx.getString(d.nameRes);
            e.desc = ctx.getString(d.descRes);
            e.on = "1".equals(getGlobalStr(XP_PREFIX + d.id, "0"));
            out[i] = e;
        }
        return out;
    }

    public void xpToggle(String id, boolean on) {
        for (XpDef d : XP_DEFS)
            if (d.id.equals(id)) {
                try {
                    Settings.Global.putString(ctx.getContentResolver(),
                            XP_PREFIX + id, on ? "1" : "0");
                } catch (Throwable t) {
                    Log.w(TAG, "xpToggle " + id + " denied", t);
                }
                break;
            }
        bump();
    }

    // ----------------------------------------------------- writes

    public void toggle(int id, boolean on) {
        try {
            switch (id) {
            case T_WIFI: {
                WifiManager wm = (WifiManager)
                        ctx.getSystemService(Context.WIFI_SERVICE);
                if (wm != null) wm.setWifiEnabled(on);
                if (on) wifiScan();
                break;
            }
            case T_BT: {
                BluetoothAdapter ba = BluetoothAdapter.getDefaultAdapter();
                if (ba != null) {
                    if (on) ba.enable(); else ba.disable();
                }
                break;
            }
            case T_MIC: {
                AudioManager am = (AudioManager)
                        ctx.getSystemService(Context.AUDIO_SERVICE);
                if (am != null) am.setMicrophoneMute(!on);
                break;
            }
            case T_TOUCHES:
                Settings.System.putInt(ctx.getContentResolver(),
                        SHOW_TOUCHES, on ? 1 : 0);
                break;
            case T_DEBUG_HUD:
                Settings.Global.putInt(ctx.getContentResolver(),
                        DEBUG_HUD, on ? 1 : 0);
                break;
            case T_DOF6:
                Settings.Global.putString(ctx.getContentResolver(),
                        DOF, on ? "6dof" : "3dof");
                break;
            case T_ADB:
                Settings.Global.putInt(ctx.getContentResolver(),
                        Settings.Global.ADB_ENABLED, on ? 1 : 0);
                break;
            case T_STAY:
                Settings.Global.putInt(ctx.getContentResolver(),
                        Settings.Global.STAY_ON_WHILE_PLUGGED_IN,
                        on ? 1 : 0);
                break;
            default:
                break;
            }
        } catch (Throwable t) {
            Log.w(TAG, "toggle " + id + " denied", t);
        }
        bump();
    }

    public void slider(int id, float v) {
        try {
            switch (id) {
            case SL_VOL: {
                AudioManager am = (AudioManager)
                        ctx.getSystemService(Context.AUDIO_SERVICE);
                if (am != null) {
                    int mx = am.getStreamMaxVolume(
                            AudioManager.STREAM_MUSIC);
                    am.setStreamVolume(AudioManager.STREAM_MUSIC,
                            Math.round(v * mx), 0);
                }
                break;
            }
            case SL_BRI:
                Settings.System.putInt(ctx.getContentResolver(),
                        Settings.System.SCREEN_BRIGHTNESS_MODE,
                        Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);
                Settings.System.putInt(ctx.getContentResolver(),
                        Settings.System.SCREEN_BRIGHTNESS,
                        Math.round(v * 255));
                break;
            case SL_IPD:
                Settings.Global.putFloat(ctx.getContentResolver(),
                        IPD, v);
                break;
            default:
                break;
            }
        } catch (Throwable t) {
            Log.w(TAG, "slider " + id + " denied", t);
        }
        bump();
    }

    public void text(int id, String v) {
        try {
            switch (id) {
            case TX_THEME:
                Settings.Global.putString(ctx.getContentResolver(), THEME, v);
                break;
            case TX_ENV:
                Settings.Global.putString(ctx.getContentResolver(),
                        HOME_ENV, v);
                break;
            case TX_LANG: {
                String cur = getGlobalStr(SYSTEM_LOCALES, "");
                Settings.Global.putString(ctx.getContentResolver(),
                        SYSTEM_LOCALES,
                        cur.isEmpty() ? v : v + "," + cur);
                break;
            }
            default:
                break;
            }
        } catch (Throwable t) {
            Log.w(TAG, "text " + id + " denied", t);
        }
        bump();
    }

    public void action(int id) {
        if (id == 1) unbindAll();
        bump();
    }

    public void scan(int sec) {
        if (sec == SEC_WIFI) wifiScan();
        else if (sec == SEC_BT) btScan();
        else if (sec == SEC_ENV) bump();
    }

    public void reboot() {
        try {
            PowerManager pm = (PowerManager)
                    ctx.getSystemService(Context.POWER_SERVICE);
            if (pm != null) pm.reboot(null);
        } catch (Throwable t) {
            Log.w(TAG, "reboot denied", t);
        }
    }

    // ----------------------------------------------------- ctrl svc

    private void bindCtrl(boolean on) {
        if (on && ctrlSvc == null) {
            ctrlConn = new ServiceConnection() {
                @Override public void onServiceConnected(
                        ComponentName n, IBinder b) {
                    ctrlSvc = b;
                    registerCb();
                    ctrlPoll.run();
                    bump();
                }
                @Override public void onServiceDisconnected(
                        ComponentName n) {
                    ctrlSvc = null;
                    bump();
                }
            };
            try {
                Intent i = new Intent(CTRL_ACTION);
                i.setPackage(CTRL_SVC_PKG);
                ctx.bindService(i, ctrlConn, Context.BIND_AUTO_CREATE);
            } catch (Throwable t) {
                Log.w(TAG, "ctrl bind failed", t);
            }
        } else if (!on && ctrlConn != null) {
            try { ctx.unbindService(ctrlConn); } catch (Throwable ignored) {}
            ctrlConn = null;
            ctrlSvc = null;
        }
    }

    private Parcel transact(int code, Parcel in) {
        Parcel out = Parcel.obtain();
        try {
            in.writeInterfaceToken(CTRL_IFACE);
            ctrlSvc.transact(code, in, out, 0);
            out.readException();
            return out;
        } catch (Throwable t) {
            out.recycle();
            return null;
        }
    }

    private void registerCb() {
        Parcel in = Parcel.obtain();
        try {
            in.writeInterfaceToken(CTRL_IFACE);
            in.writeStrongBinder(ctrlCb);
            ctrlSvc.transact(TR_REGISTER_CB, in, null,
                    android.os.IBinder.FLAG_ONEWAY);
        } catch (Throwable ignored) {}
        in.recycle();
    }

    private void pollCtrl() {
        synchronized (ctrl) {
            for (int i = 0; i < 2; ++i) {
                Parcel in = Parcel.obtain();
                in.writeInt(i);
                Parcel r = transact(TR_CV2_CONN, in);
                in.recycle();
                int st = 0;
                if (r != null) {
                    try { st = r.readInt(); } catch (Throwable ignored) {}
                    r.recycle();
                }
                ctrl[i].state = ctrlScanning && st == 0 ? 2 :
                        st == 0 ? 0 : 1;
                in = Parcel.obtain();
                in.writeInt(i);
                r = transact(TR_DEV_BLE_MAC, in);
                in.recycle();
                if (r != null) {
                    try { ctrl[i].mac = nullToEmpty(r.readString()); }
                    catch (Throwable ignored) {}
                    r.recycle();
                }
                in = Parcel.obtain();
                in.writeInt(i);
                r = transact(TR_CTRL_SN, in);
                in.recycle();
                if (r != null) {
                    try { ctrl[i].serial = nullToEmpty(r.readString()); }
                    catch (Throwable ignored) {}
                    r.recycle();
                }
                in = Parcel.obtain();
                in.writeInt(i);
                r = transact(TR_CHARGING, in);
                in.recycle();
                if (r != null) {
                    try { ctrl[i].chg = r.readInt() != 0; }
                    catch (Throwable ignored) {}
                    r.recycle();
                }
                in = Parcel.obtain();
                in.writeInt(i);
                r = transact(TR_BATTERY, in);
                in.recycle();
                if (r != null) {
                    try {
                        int[] b = r.createIntArray();
                        if (b != null && b.length > BATTERY_INDEX)
                            ctrl[i].batt = b[BATTERY_INDEX];
                        else if (b != null && b.length > 0)
                            ctrl[i].batt = b[0];
                    } catch (Throwable ignored) {}
                    r.recycle();
                }
            }
        }
        bump();
    }

    private static String nullToEmpty(String s) {
        return s == null ? "" : s;
    }

    public void toggleScan() {
        if (ctrlSvc == null) return;
        ctrlScanning = !ctrlScanning;
        Parcel in = Parcel.obtain();
        in.writeInt(ctrlScanning ? 1 : 0);
        Parcel r = transact(ctrlScanning ? TR_START_PAIR : TR_STOP_PAIR, in);
        if (r != null) r.recycle();
        if (ctrlScanning) {
            Parcel t = Parcel.obtain();
            Parcel rr = transact(TR_START_THREAD, t);
            if (rr != null) rr.recycle();
            main.removeCallbacks(ctrlScanOff);
            main.postDelayed(ctrlScanOff, SCAN_TIMEOUT_MS);
        }
        bump();
    }

    private void unbindAll() {
        if (ctrlSvc == null) return;
        Parcel in = Parcel.obtain();
        Parcel r = transact(TR_UNBIND, in);
        if (r != null) r.recycle();
        bump();
    }
}
