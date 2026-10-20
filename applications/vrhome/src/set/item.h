#pragma once

#include <string>
#include <vector>

// The settings overlay: the full settings surface as a dash-native card.
// Sections ride the left rail, the picked section's rows fill the right
// pane and scroll inside it. State comes from the host process through
// the backend's settings pull; writes go back through the same channel.

// sidebar sections, top to bottom in display order
enum SetSec {
    SEC_WIFI = 0, SEC_BT, SEC_CTRL, SEC_DISP, SEC_ENV, SEC_SOUND,
    SEC_LANG, SEC_TIME, SEC_IME, SEC_DEV, SEC_ABOUT, SEC_COUNT
};

// toggle bits in SetSnap.toggles - the wire contract with
// SettingsState.java's switch, keep both files in step
enum SetTgl {
    TG_WIFI = 0, TG_BT, TG_MIC, TG_NIGHT, TG_ADB, TG_STAY,
    TG_TOUCHES, TG_DEBUG_HUD, TG_DOF6, TG_COUNT
};

// the three slider rows, by SetSnap.sld index: volume and brightness are
// normalized 0..1, ipd crosses in millimetres
enum SetSld { SL_VOL = 0, SL_BRI, SL_IPD, SL_COUNT };
constexpr float kIpdMinMm = 56.0f, kIpdMaxMm = 74.0f;

// SetSnap.texts slots
enum SetTxt {
    TX_SSID = 0, TX_THEME, TX_ENV, TX_LANG, TX_MODEL, TX_ANDROID,
    TX_HBVER, TX_COUNT
};

// SetSnap.flags
enum SetFlagBits {
    SFL_WIFI_SCAN = 1,
    SFL_BT_SCAN = 2,
    SFL_CTRL_SCAN = 4,
};

// wifi security buckets, the flutter app's split of the caps string
enum SetWSec { WSEC_OPEN = 0, WSEC_WEP, WSEC_WPA };

// one-shot row ids for backend->settingsAction
enum SetAct { ACT_TIMEZONE = 0, ACT_CTRL_UNBIND, ACT_COUNT };

// segmented pickers: theme and language write the text channel, device
// mode the toggle one (reboot-gated)
enum SetPickId { PK_THEME = 0, PK_LANG, PK_DOF, PK_COUNT };

// named rows, in catalog order: the label/description string slots index
// off this (UISET_LBL0 + rid, UISET_DESC0 + rid)
enum SetRowId {
    RID_WIFI_ON = 0, RID_WIFI_SSID, RID_WIFI_LIST,
    RID_BT_ON, RID_BT_LIST,
    RID_CTRL_SCAN, RID_CTRL_L, RID_CTRL_R, RID_CTRL_UNBIND,
    RID_BRI, RID_THEME, RID_IPD, RID_DOF, RID_NIGHT,
    RID_ENV_LIST,
    RID_VOL, RID_MIC,
    RID_LANG,
    RID_TZ,
    RID_IME_LIST,
    RID_ADB, RID_STAY, RID_TOUCHES, RID_DEBUGHUD,
    RID_BRAND, RID_MODEL, RID_ANDROID, RID_HBVER,
    RID_SYSTEM,
    RID_COUNT
};

// what a row draws and how a tap resolves
enum SetRowKind {
    RK_TOGGLE = 0,  // id: SetTgl; switch on the right
    RK_SLIDER,      // id: SetSld; track + knob on the right
    RK_INFO,        // id: SetTxt; read-only value on the right
    RK_ACTION,      // id: SetAct; chevron row
    RK_SEG,         // id: SetPickId; segmented options on the right
    RK_SCANHDR,     // id: SetSec (wifi/bt/ctrl): status + refresh button
    RK_NET,         // aux: index into snap.wifi
    RK_BTROW,       // aux: index into snap.bt
    RK_IME,         // aux: index into snap.imes
    RK_ENV,         // aux: 0 passthrough, 1 builtin, 2+n snap.envs[n]
    RK_CTRL,        // aux: controller slot 0/1
    RK_BRAND,       // the about card
};

struct SetRow {
    int kind = RK_INFO;
    int id = -1;
    int aux = 0;
};

// wifi list entry the platform reports
struct SetWifi {
    std::string ssid;
    int sec = WSEC_OPEN;
    int level = 0;      // signal 0..4
    bool conn = false;
    int saved = -1;     // configured-network id, -1 unknown
};

struct SetBtDev {
    std::string name, addr;
    bool bonded = false, conn = false;
};

struct SetImeOpt {
    std::string id, label;
    bool active = false;
};

struct SetEnvOpt {
    std::string id, name;
    int st = 0;         // 0 ok, 1 unreadable zip, 2 no map.obj
};

// one controller slot: state 0 idle / 1 connected / 2 pairing, -1 unknown
struct SetCtrl {
    int state = 0;
    int batt = -1;      // 0..5 bars, -1 unknown
    bool chg = false;
    std::string mac, serial;
};

// one platform pull of everything the card can draw
struct SetSnap {
    unsigned toggles = 0;
    int flags = 0;
    float sld[SL_COUNT] = {};
    std::string txt[TX_COUNT];
    std::vector<SetWifi> wifi;
    std::vector<SetBtDev> bt;
    std::vector<SetImeOpt> imes;
    std::vector<SetEnvOpt> envs;
    SetCtrl ctrl[2];
};

// where a gaze hit on the card lands
enum SetZone {
    SZONE_NONE = -1,
    SZONE_SEC = 0,    // a sidebar row; idx = SetSec
    SZONE_ROW,        // a content row; idx = its row position
    SZONE_SLIDER,     // a slider row's track; idx = row position
    SZONE_SEG,        // a segmented cell; idx = row position, aux = option
    SZONE_DEL,        // an env row's delete disc; idx = row position
    SZONE_SCAN,       // a scan card's refresh button; idx = row position
    SZONE_CLOSE,      // the title row's close disc
    SZONE_BODY,       // card body: eats the tap, holds the scroll drag
    SZONE_KEY,        // dialog mini-keyboard key; idx = key index
    SZONE_BTN,        // dialog button; idx = button index
};

// which modal the card is hosting
enum SetDlg {
    DLG_NONE = 0,
    DLG_WIFI,       // join/manage the net at dlgArg (password field when new)
    DLG_BT,         // confirm dropping the bond at dlgArg
    DLG_ENV,        // confirm deleting the env zip at dlgArg - 2
    DLG_REBOOT,     // the pending TG_DOF6 pick in dlgAux applies on reboot
};

// special key codes in the dialog keyboard table (positive code = utf8 cp)
enum SetKeyCode {
    SK_SHIFT = -1,
    SK_BKSP = -2,
    SK_PAGE = -3,
    SK_DONE = -4,
};

struct SetHit {
    int zone = SZONE_NONE;
    int idx = -1;
    int aux = -1;         // SZONE_SEG: the option under the ray
    bool hit = false;     // the ray met the card at all (blocks panels)
    float u = 0, v = 0;   // hit point in card coords, -1..1
    float t = 1e9f;       // ray distance, for pick arbitration
};

// live state on the engine. rows is rebuilt when the section or the snap
// changes; dialog state sits inside the overlay so a press survives a
// background snap refresh
struct SetOverlay {
    bool shown = false;
    float openT = 0.0f;          // open progress 0..1: chases `shown`
    int section = SEC_WIFI;
    bool android = true;         // the catalog the host can serve
    SetSnap snap;
    int snapVer = -1;            // last pulled backend version
    std::vector<SetRow> rows;
    bool rowsDirty = true;
    float scroll = 0.0f;         // metres the content pane is scrolled
    int hover = -1;
    int zone = SZONE_NONE;
    int aux = -1;
    float u = 0, v = 0;
    int press = -1;
    int pressZone = SZONE_NONE;
    int pressAux = -1;
    bool scrollHeld = false;
    float grabV = 0.0f;
    float grabScroll = 0.0f;
    int slideHeld = -1;          // row position of the held slider
    int dlg = DLG_NONE;
    int dlgArg = -1;
    int dlgAux = -1;             // DLG_REBOOT: the pending toggle value
    std::string edit;            // DLG_WIFI password buffer
    int kbdPage = 0;             // 0 letters/digits, 1 symbols
    bool kbdShift = false;
};

// ui strings the card draws: slots in the backend's uiStrings pull, the
// block follows the quick panel's. Keep UiStrings.java's snapshot order
// in step
enum UiSetStr {
    UISET_TITLE = 0,
    UISET_SEC0,                            // SEC_COUNT section names
    UISET_LBL0 = UISET_SEC0 + SEC_COUNT,   // RID_COUNT row labels
    UISET_DESC0 = UISET_LBL0 + RID_COUNT,  // RID_COUNT row descriptions
    UISET_VALUE0 = UISET_DESC0 + RID_COUNT,
    UISET_CONNECTED = UISET_VALUE0,  UISET_DISCONNECTED,
    UISET_PAIRING,     UISET_PAIRED,
    UISET_SCAN_WIFI,   UISET_SCAN_BT,    UISET_SCAN_CTRL,
    UISET_IDLE_WIFI,   UISET_IDLE_BT,    UISET_IDLE_CTRL,
    UISET_EMPTY_WIFI,  UISET_EMPTY_BT,   UISET_EMPTY_ENV,
    UISET_SECURED,     UISET_OPENNET,    UISET_SAVED,
    UISET_REFRESH,     UISET_PAIR,       UISET_FORGET,
    UISET_CONNECT,     UISET_CANCEL,     UISET_REMOVE,   UISET_REBOOT,
    UISET_JOIN_FMT,    UISET_REMOVE_FMT, UISET_REMOVE_BODY,
    UISET_REBOOT_T,    UISET_REBOOT_B,
    UISET_PASSWORD,    UISET_UNKNOWN,
    UISET_3DOF,        UISET_6DOF,
    UISET_THEME_DARK,  UISET_THEME_LIGHT, UISET_THEME_OLED,
    UISET_LANG_EN,     UISET_LANG_ZH,
    UISET_ENV_PT,      UISET_ENV_PT_D,   UISET_ENV_BUILTIN,
    UISET_ENV_BUILTIN_D,
    UISET_ENV_BAD,     UISET_ENV_NOMAP,
    UISET_IME_ACTIVE,  UISET_CTRL_BATT_FMT, UISET_CTRL_CHG,
    UISET_SPACE,
    UISET_COUNT
};
