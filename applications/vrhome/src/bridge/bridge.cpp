#include "bridge.h"

#include "../common/jni.h"
#include "../common/log.h"
#include "../hud/engine.h"
#include "../kbd/kbd.h"
#include "../panels/panel.h"
#include "../panels/layout.h"
#include "../panels/panels.h"
#include "../set/item.h"
#include "../common/config.h"
#include "../common/props.h"

#include <android/bitmap.h>

#include <cstring>
#include <deque>
#include <memory>
#include <mutex>

static std::deque<std::string> gLaunchQ;
static std::mutex gLaunchMu;
static volatile bool gWantRecenter = false;

static std::string jstr(JNIEnv* env, jstring s) {
    if (!s) return "";
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string out = c ? c : "";
    if (c) env->ReleaseStringUTFChars(s, c);
    env->DeleteLocalRef(s);
    return out;
}

class JniBackend : public DashBackend {
public:
    JniBackend(HudEngine* e, JNIEnv* env, jobject br) : vm(e->vm), ctx(e->ctx) {
        bridge = env->NewGlobalRef(br);
        jclass bc = env->GetObjectClass(br);
        mCreatePanel  = env->GetMethodID(bc, "createPanel", "(IIII)I");
        mResizePanel  = env->GetMethodID(bc, "resizePanel", "(IIII)Z");
        mPanelTex     = env->GetMethodID(bc, "panelTexture",
                            "(I)Landroid/graphics/SurfaceTexture;");
        mLaunchPkg    = env->GetMethodID(bc, "launchPackageOn",
                            "(Ljava/lang/String;I)V");
        mAdopt        = env->GetMethodID(bc, "adoptTaskOn", "(II)V");
        mReleasePanel = env->GetMethodID(bc, "releasePanel", "(I)V");
        mTakeAdopt    = env->GetMethodID(bc, "takePendingAdopt",
                            "()Lgitlab/neosalsa/hud/ShellBridge$Pending;");
        mTakeRelease  = env->GetMethodID(bc, "takePendingRelease", "()I");
        mInjectTap    = env->GetMethodID(bc, "injectTap", "(IFF)V");
        mInjectTouch  = env->GetMethodID(bc, "injectTouch", "(IFFI)V");
        mRemoveTask   = env->GetMethodID(bc, "removeTask", "(I)V");
        mRemoveDisp   = env->GetMethodID(bc, "removeTasksOnDisplay", "(I)V");
        mFocusTask    = env->GetMethodID(bc, "focusTask", "(I)V");
        mAppLabel     = env->GetMethodID(bc, "appLabel",
                            "(Ljava/lang/String;)Ljava/lang/String;");
        mIsVr         = env->GetMethodID(bc, "isVrApp",
                            "(Ljava/lang/String;)Z");
        mLaunchVr     = env->GetMethodID(bc, "launchVrApp",
                            "(Ljava/lang/String;)V");
        mIsCovered    = env->GetMethodID(bc, "isCovered", "()Z");
        mTakePins     = env->GetMethodID(bc, "takePins", "()[Ljava/lang/String;");
        mSetPins      = env->GetMethodID(bc, "setPins", "([Ljava/lang/String;)V");
        mAppIcon      = env->GetMethodID(bc, "appIcon",
                            "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
        mVrVer        = env->GetMethodID(bc, "vrVersion", "()I");
        mRunningVr    = env->GetMethodID(bc, "runningVr",
                            "()[Lgitlab/neosalsa/hud/ShellBridge$Pending;");
        mDismiss      = env->GetMethodID(bc, "dismissMenu", "()V");
        mNotifVer     = env->GetMethodID(bc, "notifVersion", "()I");
        mNotifs       = env->GetMethodID(bc, "notifs",
                            "()[Lgitlab/neosalsa/hud/NotifService$Info;");
        mDismissNotif = env->GetMethodID(bc, "dismissNotif",
                            "(Ljava/lang/String;)V");
        mToastOnly    = env->GetMethodID(bc, "toastOnly", "()Z");
        mDebugHud     = env->GetMethodID(bc, "debugHud", "()Z");
        mDebugOnly    = env->GetMethodID(bc, "debugOnly", "()Z");
        mSysStatus    = env->GetMethodID(bc, "sysStatus", "()[I");
        mSysMsgVer    = env->GetMethodID(bc, "sysMsgVersion", "()I");
        mSysMsgs      = env->GetMethodID(bc, "sysMsgs",
                            "()[Lgitlab/neosalsa/hud/SysMsgs$Msg;");
        mSysMsgClick  = env->GetMethodID(bc, "sysMsgClick", "(JI)V");
        mSysMsgDismiss= env->GetMethodID(bc, "sysMsgDismiss", "(J)V");
        mSysMsgOnly   = env->GetMethodID(bc, "sysMsgOnly", "()Z");
        mUiStrVer     = env->GetMethodID(bc, "uiStringsVersion", "()I");
        mUiStrings    = env->GetMethodID(bc, "uiStrings", "()[Ljava/lang/String;");
        mKbdCreate    = env->GetMethodID(bc, "createKbdSurface", "(IIII)Z");
        mKbdTex       = env->GetMethodID(bc, "kbdTexture",
                            "()Landroid/graphics/SurfaceTexture;");
        mKbdTakeQuery = env->GetMethodID(bc, "takeKbdQuery", "()Z");
        mKbdSend      = env->GetMethodID(bc, "sendKbdSurface", "()V");
        mKbdHide      = env->GetMethodID(bc, "sendKbdHide", "()V");
        mKbdState     = env->GetMethodID(bc, "kbdState", "()[I");
        mAppsVer      = env->GetMethodID(bc, "appsVersion", "()I");
        mApps         = env->GetMethodID(bc, "launcherApps",
                            "()[Lgitlab/neosalsa/hud/ShellBridge$LauncherApp;");
        mQsStatus     = env->GetMethodID(bc, "quickStatus",
                            "()Lgitlab/neosalsa/hud/QuickState$Snap;");
        mQsToggle     = env->GetMethodID(bc, "quickToggle", "(IZ)V");
        mQsSlider     = env->GetMethodID(bc, "quickSetSlider", "(IF)V");
        mSetVer       = env->GetMethodID(bc, "settingsVersion", "()I");
        mSetSnap      = env->GetMethodID(bc, "settingsSnap",
                          "()Lgitlab/neosalsa/hud/SettingsState$Snap;");
        mSetSection   = env->GetMethodID(bc, "settingsSection", "(I)V");
        mSetClose     = env->GetMethodID(bc, "settingsClose", "()V");
        mSetToggle    = env->GetMethodID(bc, "settingsToggle", "(IZ)V");
        mSetSlider    = env->GetMethodID(bc, "settingsSlider", "(IF)V");
        mSetText      = env->GetMethodID(bc, "settingsText",
                          "(ILjava/lang/String;)V");
        mSetAction    = env->GetMethodID(bc, "settingsAction", "(I)V");
        mSetScan      = env->GetMethodID(bc, "settingsScan", "(I)V");
        mSetScanTgl   = env->GetMethodID(bc, "settingsToggleScan", "()V");
        mWifiJoin     = env->GetMethodID(bc, "wifiJoin",
                          "(Ljava/lang/String;ILjava/lang/String;)V");
        mWifiForget   = env->GetMethodID(bc, "wifiForget", "(I)V");
        mBtPair       = env->GetMethodID(bc, "btPair",
                          "(Ljava/lang/String;)V");
        mBtUnpair     = env->GetMethodID(bc, "btUnpair",
                          "(Ljava/lang/String;)V");
        mImePick      = env->GetMethodID(bc, "imePick",
                          "(Ljava/lang/String;)V");
        mEnvDelete    = env->GetMethodID(bc, "envDelete",
                          "(Ljava/lang/String;)V");
        mSetReboot    = env->GetMethodID(bc, "settingsReboot", "()V");
        jclass stc = env->FindClass("android/graphics/SurfaceTexture");
        stUpdate = env->GetMethodID(stc, "updateTexImage", "()V");
        stMatrix = env->GetMethodID(stc, "getTransformMatrix", "([F)V");
        pendingCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.ShellBridge$Pending"));
        fPendTask = env->GetFieldID(pendingCls, "taskId", "I");
        fPendPkg  = env->GetFieldID(pendingCls, "pkg", "Ljava/lang/String;");
        appCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.ShellBridge$LauncherApp"));
        fAppPkg = env->GetFieldID(appCls, "pkg", "Ljava/lang/String;");
        fAppLabel = env->GetFieldID(appCls, "label", "Ljava/lang/String;");
        notifCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.NotifService$Info"));
        fNotifKey = env->GetFieldID(notifCls, "key", "Ljava/lang/String;");
        fNotifPkg = env->GetFieldID(notifCls, "pkg", "Ljava/lang/String;");
        fNotifTitle = env->GetFieldID(notifCls, "title", "Ljava/lang/String;");
        fNotifText = env->GetFieldID(notifCls, "text", "Ljava/lang/String;");
        fNotifMs = env->GetFieldID(notifCls, "postMs", "J");
        fNotifClear = env->GetFieldID(notifCls, "clearable", "Z");
        sysMsgCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.SysMsgs$Msg"));
        fMsgId = env->GetFieldID(sysMsgCls, "id", "J");
        fMsgPkg = env->GetFieldID(sysMsgCls, "pkg", "Ljava/lang/String;");
        fMsgTitle = env->GetFieldID(sysMsgCls, "title", "Ljava/lang/String;");
        fMsgText = env->GetFieldID(sysMsgCls, "text", "Ljava/lang/String;");
        fMsgBtns = env->GetFieldID(sysMsgCls, "buttons", "[Ljava/lang/String;");
        qsSnapCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.QuickState$Snap"));
        fQsToggles = env->GetFieldID(qsSnapCls, "toggles", "I");
        fQsBatt = env->GetFieldID(qsSnapCls, "batt", "I");
        fQsChg = env->GetFieldID(qsSnapCls, "chg", "I");
        fQsVol = env->GetFieldID(qsSnapCls, "vol", "F");
        fQsBri = env->GetFieldID(qsSnapCls, "bri", "F");
        fQsNet = env->GetFieldID(qsSnapCls, "net", "I");
        fQsSsid = env->GetFieldID(qsSnapCls, "ssid", "Ljava/lang/String;");
        fQsBt = env->GetFieldID(qsSnapCls, "bt", "Ljava/lang/String;");
        setSnapCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.SettingsState$Snap"));
        fSetToggles = env->GetFieldID(setSnapCls, "toggles", "I");
        fSetFlags = env->GetFieldID(setSnapCls, "flags", "I");
        fSetVol = env->GetFieldID(setSnapCls, "vol", "F");
        fSetBri = env->GetFieldID(setSnapCls, "bri", "F");
        fSetIpd = env->GetFieldID(setSnapCls, "ipd", "F");
        fSetTexts = env->GetFieldID(setSnapCls, "texts",
                      "[Ljava/lang/String;");
        fSetWifi = env->GetFieldID(setSnapCls, "wifi",
                     "[Lgitlab/neosalsa/hud/SettingsState$WifiNet;");
        fSetBt = env->GetFieldID(setSnapCls, "bt",
                   "[Lgitlab/neosalsa/hud/SettingsState$BtDev;");
        fSetIme = env->GetFieldID(setSnapCls, "imes",
                    "[Lgitlab/neosalsa/hud/SettingsState$ImeEnt;");
        fSetEnv = env->GetFieldID(setSnapCls, "envs",
                    "[Lgitlab/neosalsa/hud/SettingsState$EnvEnt;");
        fSetCtrl = env->GetFieldID(setSnapCls, "ctrl",
                     "[Lgitlab/neosalsa/hud/SettingsState$CtrlSlot;");
        wifiCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.SettingsState$WifiNet"));
        fWifiSsid = env->GetFieldID(wifiCls, "ssid", "Ljava/lang/String;");
        fWifiSec = env->GetFieldID(wifiCls, "sec", "I");
        fWifiLevel = env->GetFieldID(wifiCls, "level", "I");
        fWifiConn = env->GetFieldID(wifiCls, "conn", "Z");
        fWifiSaved = env->GetFieldID(wifiCls, "saved", "I");
        btCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.SettingsState$BtDev"));
        fBtName = env->GetFieldID(btCls, "name", "Ljava/lang/String;");
        fBtAddr = env->GetFieldID(btCls, "addr", "Ljava/lang/String;");
        fBtBonded = env->GetFieldID(btCls, "bonded", "Z");
        fBtConn = env->GetFieldID(btCls, "conn", "Z");
        imeCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.SettingsState$ImeEnt"));
        fImeId = env->GetFieldID(imeCls, "id", "Ljava/lang/String;");
        fImeLabel = env->GetFieldID(imeCls, "label", "Ljava/lang/String;");
        fImeActive = env->GetFieldID(imeCls, "active", "Z");
        envCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.SettingsState$EnvEnt"));
        fEnvId = env->GetFieldID(envCls, "id", "Ljava/lang/String;");
        fEnvName = env->GetFieldID(envCls, "name", "Ljava/lang/String;");
        fEnvSt = env->GetFieldID(envCls, "st", "I");
        ctrlCls = (jclass)env->NewGlobalRef(loadAppClass(env, ctx,
            "gitlab.neosalsa.hud.SettingsState$CtrlSlot"));
        fCtrlState = env->GetFieldID(ctrlCls, "state", "I");
        fCtrlBatt = env->GetFieldID(ctrlCls, "batt", "I");
        fCtrlChg = env->GetFieldID(ctrlCls, "chg", "Z");
        fCtrlMac = env->GetFieldID(ctrlCls, "mac", "Ljava/lang/String;");
        fCtrlSn = env->GetFieldID(ctrlCls, "serial", "Ljava/lang/String;");
    }

    JNIEnv* env() { return threadEnv(vm); }
    bool fail(JNIEnv* e) {
        if (!e->ExceptionCheck()) return false;
        e->ExceptionClear();
        return true;
    }

    int createPanel(Panel* p, unsigned tex, int w, int h, int dpi) override {
        JNIEnv* e = env();
        int id = e->CallIntMethod(bridge, mCreatePanel, (jint)tex, w, h, dpi);
        if (fail(e) || id < 0) return -1;
        jobject st = e->CallObjectMethod(bridge, mPanelTex, id);
        if (fail(e) || !st) {
            e->CallVoidMethod(bridge, mReleasePanel, id);
            fail(e);
            return -1;
        }
        p->displayId = id;
        p->st = e->NewGlobalRef(st);
        p->stArr = e->NewGlobalRef(e->NewFloatArray(16));
        e->DeleteLocalRef(st);
        return id;
    }

    bool resizePanel(Panel* p, int w, int h, int dpi) override {
        JNIEnv* e = env();
        const bool ok = e->CallBooleanMethod(bridge, mResizePanel,
                p->displayId, w, h, dpi) == JNI_TRUE;
        fail(e);
        return ok;
    }

    void releasePanel(Panel* p) override {
        JNIEnv* e = env();
        if (p->displayId >= 0) {
            e->CallVoidMethod(bridge, mReleasePanel, p->displayId);
            fail(e);
        }
        if (p->st) e->DeleteGlobalRef((jobject)p->st);
        if (p->stArr) e->DeleteGlobalRef((jobject)p->stArr);
    }

    void updatePanel(Panel* p) override {
        if (!p->st) return;
        JNIEnv* e = env();
        e->CallVoidMethod((jobject)p->st, stUpdate);
        e->CallVoidMethod((jobject)p->st, stMatrix, (jfloatArray)p->stArr);
        e->GetFloatArrayRegion((jfloatArray)p->stArr, 0, 16, p->stMat);
        fail(e);
    }

    bool createKbdSurface(Kbd* k, unsigned tex, int w, int h, int dpi) override {
        JNIEnv* e = env();
        if (e->CallBooleanMethod(bridge, mKbdCreate, (jint)tex, w, h, dpi)
                != JNI_TRUE || fail(e)) return false;
        jobject st = e->CallObjectMethod(bridge, mKbdTex);
        if (fail(e) || !st) return false;
        k->st = e->NewGlobalRef(st);
        k->stArr = e->NewGlobalRef(e->NewFloatArray(16));
        e->DeleteLocalRef(st);
        return true;
    }

    void updateKbd(Kbd* k) override {
        JNIEnv* e = env();
        e->CallVoidMethod((jobject)k->st, stUpdate);
        e->CallVoidMethod((jobject)k->st, stMatrix, (jfloatArray)k->stArr);
        e->GetFloatArrayRegion((jfloatArray)k->stArr, 0, 16, k->stMat);
        fail(e);
    }

    bool takeKbdQuery() override {
        JNIEnv* e = env();
        bool r = e->CallBooleanMethod(bridge, mKbdTakeQuery) == JNI_TRUE;
        fail(e);
        return r;
    }
    void sendKbdSurface() override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mKbdSend); fail(e);
    }
    void sendKbdHide() override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mKbdHide); fail(e);
    }
    KbdState kbdState() override {
        JNIEnv* e = env();
        jintArray a = (jintArray)e->CallObjectMethod(bridge, mKbdState);
        KbdState s;
        if (a && e->GetArrayLength(a) >= 3) {
            jint v[3];
            e->GetIntArrayRegion(a, 0, 3, v);
            s.shown = v[0] != 0; s.displayId = v[1]; s.only = v[2] != 0;
        }
        if (a) e->DeleteLocalRef(a);
        fail(e);
        return s;
    }

    bool takePins(std::vector<std::string>* out) override {
        JNIEnv* e = env();
        jobjectArray arr =
            (jobjectArray)e->CallObjectMethod(bridge, mTakePins);
        if (fail(e) || !arr) return false;
        out->clear();
        const int n = e->GetArrayLength(arr);
        for (int i = 0; i < n; ++i)
            out->push_back(jstr(e, (jstring)e->GetObjectArrayElement(arr, i)));
        e->DeleteLocalRef(arr);
        return true;
    }

    void setPins(const std::vector<std::string>& pins) override {
        JNIEnv* e = env();
        jclass sc = e->FindClass("java/lang/String");
        jobjectArray a = e->NewObjectArray((int)pins.size(), sc, nullptr);
        for (int i = 0; i < (int)pins.size(); ++i)
            e->SetObjectArrayElement(a, i, e->NewStringUTF(pins[i].c_str()));
        e->CallVoidMethod(bridge, mSetPins, a);
        fail(e);
        e->DeleteLocalRef(a); e->DeleteLocalRef(sc);
    }

    int vrVersion() override {
        JNIEnv* e = env();
        int v = e->CallIntMethod(bridge, mVrVer);
        fail(e);
        return v;
    }

    void runningVr(std::vector<XrTask>* out) override {
        out->clear();
        JNIEnv* e = env();
        jobjectArray arr =
            (jobjectArray)e->CallObjectMethod(bridge, mRunningVr);
        if (fail(e) || !arr) return;
        const int n = e->GetArrayLength(arr);
        for (int i = 0; i < n; ++i) {
            jobject p = e->GetObjectArrayElement(arr, i);
            XrTask t;
            t.taskId = e->GetIntField(p, fPendTask);
            t.pkg = jstr(e, (jstring)e->GetObjectField(p, fPendPkg));
            out->push_back(t);
            e->DeleteLocalRef(p);
        }
        e->DeleteLocalRef(arr);
    }

    void sysStatus(int out[3]) override {
        JNIEnv* e = env();
        jintArray a = (jintArray)e->CallObjectMethod(bridge, mSysStatus);
        if (a && e->GetArrayLength(a) >= 3) {
            jint v[3]; e->GetIntArrayRegion(a, 0, 3, v);
            out[0] = v[0]; out[1] = v[1]; out[2] = v[2];
        }
        if (a) e->DeleteLocalRef(a);
        fail(e);
    }

    bool iconPixels(const std::string& pkg, IconPixels* out) override {
        JNIEnv* e = env();
        jstring jp = e->NewStringUTF(pkg.c_str());
        jobject bmp = e->CallObjectMethod(bridge, mAppIcon, jp);
        e->DeleteLocalRef(jp);
        if (fail(e) || !bmp) return false;
        AndroidBitmapInfo info;
        void* px = nullptr;
        bool ok = AndroidBitmap_getInfo(e, bmp, &info) ==
                  ANDROID_BITMAP_RESULT_SUCCESS &&
                  info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 &&
                  AndroidBitmap_lockPixels(e, bmp, &px) ==
                  ANDROID_BITMAP_RESULT_SUCCESS && px;
        if (ok) {
            out->w = info.width; out->h = info.height;
            out->rgba.assign((unsigned char*)px,
                (unsigned char*)px + info.width * info.height * 4);
            AndroidBitmap_unlockPixels(e, bmp);
        }
        e->DeleteLocalRef(bmp);
        return ok;
    }

    std::string appLabel(const std::string& pkg) override {
        JNIEnv* e = env();
        jstring jp = e->NewStringUTF(pkg.c_str());
        std::string out = jstr(e,
            (jstring)e->CallObjectMethod(bridge, mAppLabel, jp));
        e->DeleteLocalRef(jp);
        fail(e);
        return out.empty() ? pkg : out;
    }

    bool isVrApp(const std::string& pkg) override {
        JNIEnv* e = env();
        jstring jp = e->NewStringUTF(pkg.c_str());
        bool r = e->CallBooleanMethod(bridge, mIsVr, jp) == JNI_TRUE;
        e->DeleteLocalRef(jp); fail(e);
        return r;
    }

    int appsVersion() override {
        JNIEnv* e = env(); int v = e->CallIntMethod(bridge, mAppsVer); fail(e); return v;
    }
    void launcherApps(std::vector<GridItem>* out) override {
        out->clear();
        JNIEnv* e = env();
        jobjectArray a = (jobjectArray)e->CallObjectMethod(bridge, mApps);
        if (fail(e) || !a) return;
        const int n = e->GetArrayLength(a);
        for (int i = 0; i < n; ++i) {
            jobject o = e->GetObjectArrayElement(a, i);
            GridItem it;
            it.pkg = jstr(e, (jstring)e->GetObjectField(o, fAppPkg));
            it.label = jstr(e, (jstring)e->GetObjectField(o, fAppLabel));
            if (!it.pkg.empty()) out->push_back(it);
            e->DeleteLocalRef(o);
        }
        e->DeleteLocalRef(a);
    }

    int notifVersion() override {
        JNIEnv* e = env(); int v = e->CallIntMethod(bridge, mNotifVer); fail(e); return v;
    }
    void notifs(std::vector<NotifItem>* out) override {
        out->clear();
        JNIEnv* e = env();
        jobjectArray a = (jobjectArray)e->CallObjectMethod(bridge, mNotifs);
        if (fail(e) || !a) return;
        const int n = e->GetArrayLength(a);
        for (int i = 0; i < n; ++i) {
            jobject o = e->GetObjectArrayElement(a, i);
            NotifItem it;
            it.key = jstr(e, (jstring)e->GetObjectField(o, fNotifKey));
            it.pkg = jstr(e, (jstring)e->GetObjectField(o, fNotifPkg));
            it.title = jstr(e, (jstring)e->GetObjectField(o, fNotifTitle));
            it.text = jstr(e, (jstring)e->GetObjectField(o, fNotifText));
            it.postMs = e->GetLongField(o, fNotifMs);
            it.clearable = e->GetBooleanField(o, fNotifClear) == JNI_TRUE;
            if (!it.key.empty()) out->push_back(it);
            e->DeleteLocalRef(o);
        }
        e->DeleteLocalRef(a);
    }
    void dismissNotif(const std::string& key) override {
        JNIEnv* e = env(); jstring k = e->NewStringUTF(key.c_str());
        e->CallVoidMethod(bridge, mDismissNotif, k); e->DeleteLocalRef(k); fail(e);
    }

    int sysMsgVersion() override {
        JNIEnv* e = env(); int v = e->CallIntMethod(bridge, mSysMsgVer); fail(e); return v;
    }
    void sysMsgs(std::vector<SysMsgItem>* out) override {
        out->clear();
        JNIEnv* e = env();
        jobjectArray a = (jobjectArray)e->CallObjectMethod(bridge, mSysMsgs);
        if (fail(e) || !a) return;
        const int n = e->GetArrayLength(a);
        for (int i = 0; i < n; ++i) {
            jobject o = e->GetObjectArrayElement(a, i);
            SysMsgItem it;
            it.id = e->GetLongField(o, fMsgId);
            it.pkg = jstr(e, (jstring)e->GetObjectField(o, fMsgPkg));
            it.title = jstr(e, (jstring)e->GetObjectField(o, fMsgTitle));
            it.text = jstr(e, (jstring)e->GetObjectField(o, fMsgText));
            jobjectArray bs = (jobjectArray)e->GetObjectField(o, fMsgBtns);
            if (bs) {
                const int nb = e->GetArrayLength(bs);
                for (int b = 0; b < nb && b < kSysMsgMaxBtn; ++b)
                    it.buttons.push_back(jstr(e,
                        (jstring)e->GetObjectArrayElement(bs, b)));
                e->DeleteLocalRef(bs);
            }
            out->push_back(it); e->DeleteLocalRef(o);
        }
        e->DeleteLocalRef(a);
    }
    void sysMsgClick(long long id, int btn) override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mSysMsgClick, (jlong)id, btn); fail(e);
    }
    void sysMsgDismiss(long long id) override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mSysMsgDismiss, (jlong)id); fail(e);
    }

    int uiStringsVersion() override {
        JNIEnv* e = env(); int v = e->CallIntMethod(bridge, mUiStrVer); fail(e); return v;
    }
    void uiStrings(std::vector<std::string>* out) override {
        out->clear();
        JNIEnv* e = env();
        jobjectArray a = (jobjectArray)e->CallObjectMethod(bridge, mUiStrings);
        if (fail(e) || !a) return;
        const int n = e->GetArrayLength(a);
        for (int i = 0; i < n; ++i)
            out->push_back(jstr(e, (jstring)e->GetObjectArrayElement(a, i)));
        e->DeleteLocalRef(a);
    }

    void quickStatus(QsStatus* out) override {
        JNIEnv* e = env();
        jobject s = e->CallObjectMethod(bridge, mQsStatus);
        if (fail(e) || !s) return;
        out->toggles = (unsigned)e->GetIntField(s, fQsToggles);
        out->batt = e->GetIntField(s, fQsBatt);
        out->chg = e->GetIntField(s, fQsChg);
        out->net = e->GetIntField(s, fQsNet);
        out->volume = e->GetFloatField(s, fQsVol);
        out->brightness = e->GetFloatField(s, fQsBri);
        out->ssid = jstr(e, (jstring)e->GetObjectField(s, fQsSsid));
        out->btName = jstr(e, (jstring)e->GetObjectField(s, fQsBt));
        e->DeleteLocalRef(s);
    }
    void quickToggle(int id, bool on) override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mQsToggle, id, on ? JNI_TRUE : JNI_FALSE);
        fail(e);
    }
    void quickSetSlider(int slider, float v) override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mQsSlider, slider, v);
        fail(e);
    }

    int settingsVersion() override {
        JNIEnv* e = env(); int v = e->CallIntMethod(bridge, mSetVer);
        fail(e); return v;
    }
    void settingsStatus(SetSnap* out) override {
        JNIEnv* e = env();
        jobject s = e->CallObjectMethod(bridge, mSetSnap);
        if (fail(e) || !s) return;
        out->toggles = (unsigned)e->GetIntField(s, fSetToggles);
        out->flags = e->GetIntField(s, fSetFlags);
        out->sld[SL_VOL] = e->GetFloatField(s, fSetVol);
        out->sld[SL_BRI] = e->GetFloatField(s, fSetBri);
        out->sld[SL_IPD] = e->GetFloatField(s, fSetIpd);
        jobjectArray tx = (jobjectArray)e->GetObjectField(s, fSetTexts);
        if (tx) {
            const int n = e->GetArrayLength(tx);
            for (int i = 0; i < n && i < TX_COUNT; ++i)
                out->txt[i] = jstr(e,
                    (jstring)e->GetObjectArrayElement(tx, i));
            e->DeleteLocalRef(tx);
        }
        jobjectArray wf = (jobjectArray)e->GetObjectField(s, fSetWifi);
        out->wifi.clear();
        if (wf) {
            const int n = e->GetArrayLength(wf);
            for (int i = 0; i < n; ++i) {
                jobject o = e->GetObjectArrayElement(wf, i);
                SetWifi w;
                w.ssid = jstr(e,(jstring)e->GetObjectField(o, fWifiSsid));
                w.sec = e->GetIntField(o, fWifiSec);
                w.level = e->GetIntField(o, fWifiLevel);
                w.conn = e->GetBooleanField(o, fWifiConn) == JNI_TRUE;
                w.saved = e->GetIntField(o, fWifiSaved);
                if (!w.ssid.empty()) out->wifi.push_back(w);
                e->DeleteLocalRef(o);
            }
            e->DeleteLocalRef(wf);
        }
        jobjectArray ba = (jobjectArray)e->GetObjectField(s, fSetBt);
        out->bt.clear();
        if (ba) {
            const int n = e->GetArrayLength(ba);
            for (int i = 0; i < n; ++i) {
                jobject o = e->GetObjectArrayElement(ba, i);
                SetBtDev d;
                d.name = jstr(e,(jstring)e->GetObjectField(o, fBtName));
                d.addr = jstr(e,(jstring)e->GetObjectField(o, fBtAddr));
                d.bonded = e->GetBooleanField(o, fBtBonded) == JNI_TRUE;
                d.conn = e->GetBooleanField(o, fBtConn) == JNI_TRUE;
                out->bt.push_back(d);
                e->DeleteLocalRef(o);
            }
            e->DeleteLocalRef(ba);
        }
        jobjectArray ia = (jobjectArray)e->GetObjectField(s, fSetIme);
        out->imes.clear();
        if (ia) {
            const int n = e->GetArrayLength(ia);
            for (int i = 0; i < n; ++i) {
                jobject o = e->GetObjectArrayElement(ia, i);
                SetImeOpt im;
                im.id = jstr(e,(jstring)e->GetObjectField(o, fImeId));
                im.label = jstr(e,(jstring)e->GetObjectField(o, fImeLabel));
                im.active = e->GetBooleanField(o, fImeActive) == JNI_TRUE;
                out->imes.push_back(im);
                e->DeleteLocalRef(o);
            }
            e->DeleteLocalRef(ia);
        }
        jobjectArray ea = (jobjectArray)e->GetObjectField(s, fSetEnv);
        out->envs.clear();
        if (ea) {
            const int n = e->GetArrayLength(ea);
            for (int i = 0; i < n; ++i) {
                jobject o = e->GetObjectArrayElement(ea, i);
                SetEnvOpt ev;
                ev.id = jstr(e,(jstring)e->GetObjectField(o, fEnvId));
                ev.name = jstr(e,(jstring)e->GetObjectField(o, fEnvName));
                ev.st = e->GetIntField(o, fEnvSt);
                out->envs.push_back(ev);
                e->DeleteLocalRef(o);
            }
            e->DeleteLocalRef(ea);
        }
        jobjectArray ca = (jobjectArray)e->GetObjectField(s, fSetCtrl);
        if (ca) {
            const int n = e->GetArrayLength(ca);
            for (int i = 0; i < n && i < 2; ++i) {
                jobject o = e->GetObjectArrayElement(ca, i);
                out->ctrl[i].state = e->GetIntField(o, fCtrlState);
                out->ctrl[i].batt = e->GetIntField(o, fCtrlBatt);
                out->ctrl[i].chg =
                    e->GetBooleanField(o, fCtrlChg) == JNI_TRUE;
                out->ctrl[i].mac =
                    jstr(e,(jstring)e->GetObjectField(o, fCtrlMac));
                out->ctrl[i].serial =
                    jstr(e,(jstring)e->GetObjectField(o, fCtrlSn));
                e->DeleteLocalRef(o);
            }
            e->DeleteLocalRef(ca);
        }
        e->DeleteLocalRef(s);
    }
    void settingsSection(int sec) override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mSetSection, sec); fail(e);
    }
    void settingsClose() override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mSetClose); fail(e);
    }
    void settingsToggle(int id, bool on) override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mSetToggle, id, on ? JNI_TRUE : JNI_FALSE);
        fail(e);
    }
    void settingsSlider(int id, float v) override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mSetSlider, id, v); fail(e);
    }
    void settingsText(int id, const std::string& v) override {
        JNIEnv* e = env(); jstring s = e->NewStringUTF(v.c_str());
        e->CallVoidMethod(bridge, mSetText, id, s);
        e->DeleteLocalRef(s); fail(e);
    }
    void settingsAction(int id) override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mSetAction, id); fail(e);
    }
    void settingsScan(int sec) override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mSetScan, sec); fail(e);
    }
    void settingsToggleScan() override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mSetScanTgl); fail(e);
    }
    void wifiJoin(const std::string& ssid, int sec,
                  const std::string& pw) override {
        JNIEnv* e = env();
        jstring a = e->NewStringUTF(ssid.c_str());
        jstring b = e->NewStringUTF(pw.c_str());
        e->CallVoidMethod(bridge, mWifiJoin, a, sec, b);
        e->DeleteLocalRef(a); e->DeleteLocalRef(b); fail(e);
    }
    void wifiForget(int netId) override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mWifiForget, netId); fail(e);
    }
    void btPair(const std::string& addr) override {
        JNIEnv* e = env(); jstring a = e->NewStringUTF(addr.c_str());
        e->CallVoidMethod(bridge, mBtPair, a); e->DeleteLocalRef(a); fail(e);
    }
    void btUnpair(const std::string& addr) override {
        JNIEnv* e = env(); jstring a = e->NewStringUTF(addr.c_str());
        e->CallVoidMethod(bridge, mBtUnpair, a); e->DeleteLocalRef(a); fail(e);
    }
    void imePick(const std::string& id) override {
        JNIEnv* e = env(); jstring a = e->NewStringUTF(id.c_str());
        e->CallVoidMethod(bridge, mImePick, a); e->DeleteLocalRef(a); fail(e);
    }
    void envDelete(const std::string& id) override {
        JNIEnv* e = env(); jstring a = e->NewStringUTF(id.c_str());
        e->CallVoidMethod(bridge, mEnvDelete, a); e->DeleteLocalRef(a); fail(e);
    }
    void settingsReboot() override {
        JNIEnv* e = env();
        e->CallVoidMethod(bridge, mSetReboot); fail(e);
    }

    void launchPackageOn(const std::string& pkg, int displayId) override {
        JNIEnv* e = env(); jstring p = e->NewStringUTF(pkg.c_str());
        e->CallVoidMethod(bridge, mLaunchPkg, p, displayId); e->DeleteLocalRef(p); fail(e);
    }
    void launchVrApp(const std::string& pkg) override {
        JNIEnv* e = env(); jstring p = e->NewStringUTF(pkg.c_str());
        e->CallVoidMethod(bridge, mLaunchVr, p); e->DeleteLocalRef(p); fail(e);
    }
    void adoptTaskOn(int taskId, int displayId) override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mAdopt, taskId, displayId); fail(e);
    }
    void focusTask(int taskId) override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mFocusTask, taskId); fail(e);
    }
    void removeTask(int taskId) override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mRemoveTask, taskId); fail(e);
    }
    void removeTasksOnDisplay(int displayId) override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mRemoveDisp, displayId); fail(e);
    }
    void injectTouch(int displayId, float x, float y, int action) override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mInjectTouch, displayId, x, y, action); fail(e);
    }
    void injectTap(int displayId, float x, float y) override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mInjectTap, displayId, x, y); fail(e);
    }
    void dismissMenu() override {
        JNIEnv* e = env(); e->CallVoidMethod(bridge, mDismiss); fail(e);
    }

    bool isCovered() override { JNIEnv* e = env(); bool r=e->CallBooleanMethod(bridge,mIsCovered)==JNI_TRUE; fail(e); return r; }
    bool toastOnly() override { JNIEnv* e = env(); bool r=e->CallBooleanMethod(bridge,mToastOnly)==JNI_TRUE; fail(e); return r; }
    bool debugHud() override { JNIEnv* e = env(); bool r=e->CallBooleanMethod(bridge,mDebugHud)==JNI_TRUE; fail(e); return r; }
    bool debugOnly() override { JNIEnv* e = env(); bool r=e->CallBooleanMethod(bridge,mDebugOnly)==JNI_TRUE; fail(e); return r; }
    bool sysMsgOnly() override { JNIEnv* e = env(); bool r=e->CallBooleanMethod(bridge,mSysMsgOnly)==JNI_TRUE; fail(e); return r; }

    bool takePendingAdopt(PendingPanel* out) override {
        JNIEnv* e = env(); jobject p = e->CallObjectMethod(bridge, mTakeAdopt);
        if (fail(e) || !p) return false;
        out->taskId = e->GetIntField(p, fPendTask);
        out->pkg = jstr(e, (jstring)e->GetObjectField(p, fPendPkg));
        e->DeleteLocalRef(p); return true;
    }
    int takePendingRelease() override {
        JNIEnv* e = env(); int id=e->CallIntMethod(bridge,mTakeRelease); fail(e); return id;
    }

    void callCtx(const char* name, const char* sig, jint arg = 0) {
        if (!ctx) return;
        JNIEnv* e = env(); jclass c=e->GetObjectClass(ctx);
        jmethodID m=e->GetMethodID(c,name,sig);
        if (m) e->CallVoidMethod(ctx,m,arg); fail(e); e->DeleteLocalRef(c);
    }
    void pokeCtrlThread() override { callCtx("pokeCtrlThread", "()V"); }
    void debugSummon() override { callCtx("debugSummon", "()V"); }
    void debugSummonKey(int action) override { callCtx("debugSummonKey", "(I)V", action); }

private:
    JavaVM* vm;
    jobject ctx, bridge;
    jmethodID mCreatePanel, mResizePanel, mPanelTex, mLaunchPkg, mAdopt, mReleasePanel,
        mTakeAdopt, mTakeRelease, mInjectTap, mInjectTouch, mRemoveTask,
        mRemoveDisp, mFocusTask, mAppLabel, mIsVr, mLaunchVr, mIsCovered,
        mTakePins, mSetPins, mAppIcon, mVrVer, mRunningVr, mDismiss,
        mNotifVer, mNotifs, mDismissNotif, mToastOnly, mDebugHud,
        mDebugOnly, mSysStatus, mSysMsgVer, mSysMsgs, mSysMsgClick,
        mSysMsgDismiss, mSysMsgOnly, mUiStrVer, mUiStrings,
        mKbdCreate, mKbdTex, mKbdTakeQuery, mKbdSend, mKbdHide, mKbdState,
        mAppsVer, mApps, mQsStatus, mQsToggle, mQsSlider, stUpdate, stMatrix,
        mSetVer, mSetSnap, mSetSection, mSetClose, mSetToggle, mSetSlider,
        mSetText, mSetAction, mSetScan, mSetScanTgl, mWifiJoin,
        mWifiForget, mBtPair, mBtUnpair, mImePick, mEnvDelete,
        mSetReboot;
    jclass pendingCls, appCls, notifCls, sysMsgCls, qsSnapCls, setSnapCls,
        wifiCls, btCls, imeCls, envCls, ctrlCls;
    jfieldID fPendTask, fPendPkg, fAppPkg, fAppLabel, fNotifKey, fNotifPkg,
        fNotifTitle, fNotifText, fNotifMs, fNotifClear, fMsgId, fMsgPkg,
        fMsgTitle, fMsgText, fMsgBtns, fQsToggles, fQsBatt, fQsChg, fQsVol,
        fQsBri, fQsNet, fQsSsid, fQsBt,
        fSetToggles, fSetFlags, fSetVol, fSetBri, fSetIpd, fSetTexts,
        fSetWifi, fSetBt, fSetIme, fSetEnv, fSetCtrl,
        fWifiSsid, fWifiSec, fWifiLevel, fWifiConn, fWifiSaved,
        fBtName, fBtAddr, fBtBonded, fBtConn,
        fImeId, fImeLabel, fImeActive, fEnvId, fEnvName, fEnvSt,
        fCtrlState, fCtrlBatt, fCtrlChg, fCtrlMac, fCtrlSn;
};

extern "C" JNIEXPORT void JNICALL
Java_gitlab_neosalsa_hud_ShellBridge_nativeQueueLaunch(JNIEnv* env, jclass,
                                                      jstring pkg) {
    const char* p = env->GetStringUTFChars(pkg, nullptr);
    queueLaunch(p);
    env->ReleaseStringUTFChars(pkg, p);
}

void initBridge(HudEngine* e, JNIEnv* env, jobject br) {
    e->backend = new JniBackend(e, env, br);
    LOGI("bridge ready");
}
