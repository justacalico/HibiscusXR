// Host tests for the settings card's pure module: the catalog build,
// geometry, picking and the dialog keyboard all run without GL.

#include "test.h"

#include "set/item.h"
#include "set/layout.h"
#include "../src/common/config.h"

#include <cmath>
#include <vector>

static void rowsFor(int sec, const SetSnap& snap, SetOverlay* ov) {
    ov->section = sec;
    ov->snap = snap;
    setBuildRows(sec, ov->android, snap, &ov->rows);
}

// ------------------------------------------------------------- catalog


void testSet() {
    // ---- catalogSectionSizes
    {
    SetSnap snap;
    snap.wifi.resize(3);
    snap.bt.resize(2);
    snap.envs.resize(1);
    snap.imes.resize(2);
    SetOverlay ov;

    rowsFor(SEC_WIFI, snap, &ov);
    CHECK(ov.rows.size() == 6);   // toggle + ssid + scan + 3 nets
    CHECK(ov.rows[0].kind == RK_TOGGLE && ov.rows[0].id == TG_WIFI);
    CHECK(ov.rows[3].kind == RK_NET && ov.rows[3].aux == 0);

    rowsFor(SEC_BT, snap, &ov);
    CHECK(ov.rows.size() == 4);

    rowsFor(SEC_CTRL, snap, &ov);
    CHECK(ov.rows.size() == 4);   // scan + left + right + unbind
    CHECK(ov.rows[3].id == ACT_CTRL_UNBIND);

    rowsFor(SEC_DISP, snap, &ov);
    CHECK(ov.rows.size() == 5);
    CHECK(ov.rows[1].kind == RK_SEG && ov.rows[1].id == PK_THEME);
    CHECK(ov.rows[3].id == PK_DOF);

    rowsFor(SEC_ENV, snap, &ov);
    CHECK(ov.rows.size() == 4);   // scan + pt + builtin + 1 zip
    CHECK(ov.rows[3].kind == RK_ENV && ov.rows[3].aux == 2);

    rowsFor(SEC_SOUND, snap, &ov);
    CHECK(ov.rows.size() == 2 && ov.rows[0].id == SL_VOL);

    rowsFor(SEC_LANG, snap, &ov);
    CHECK(ov.rows.size() == 1 && ov.rows[0].id == PK_LANG);

    rowsFor(SEC_IME, snap, &ov);
    CHECK(ov.rows.size() == 3);

    rowsFor(SEC_DEV, snap, &ov);
    CHECK(ov.rows.size() == 4);

    rowsFor(SEC_ABOUT, snap, &ov);
    CHECK(ov.rows.size() == 4 && ov.rows[0].kind == RK_BRAND);

    // ---- catalogDesktopDropsAndroidRows
    ov.android = false;
    CHECK(setSecCount(true) == SEC_COUNT);
    CHECK(setSecCount(false) == 6);
    CHECK(setSecAt(false, 0) == SEC_WIFI &&
          setSecAt(false, 5) == SEC_ABOUT);
    CHECK(setSecAt(false, -1) == -1 && setSecAt(false, 6) == -1);
    CHECK(setSecAt(true, SEC_DEV) == SEC_DEV);
    CHECK(setSecSlot(false, SEC_ABOUT) == 5);
    CHECK(setSecSlot(false, SEC_DEV) == -1);
    CHECK(setSecSlot(true, SEC_DEV) == SEC_DEV);
    rowsFor(SEC_DISP, snap, &ov);
    CHECK(ov.rows.size() == 2);   // brightness + theme only: no ipd/dof
    rowsFor(SEC_ABOUT, snap, &ov);
    CHECK(ov.rows.size() == 3);   // no android version row
    CHECK(ov.rows[2].aux == RID_HBVER);
    ov.android = true;
}

// ------------------------------------------------------------- geometry    }

    // ---- secRectStack
    {
    float x0, y0, hw0, hh0, x1, y1, hw1, hh1;
    setSecRect(0, &x0, &y0, &hw0, &hh0);
    setSecRect(1, &x1, &y1, &hw1, &hh1);
    CHECK_F(x0, x1, 1e-5f);
    CHECK(y0 > y1);                        // stacked downward
    CHECK_F(y0 - y1, kSetSecRowH, 1e-5f);
    CHECK(y1 - hh1 > -kSetHH);             // inside the card
    // label bound: the pill's right edge stays left of the divider
    CHECK(x0 + hw0 - 0.010f < setSideR() + kSetPad * 0.4f);
    // and leaves room for icon + label inside the pill
    CHECK(x0 - hw0 + 0.058f < x0 + hw0 - 0.010f);
    }

    // ---- rowRectScrolls
    {
    SetOverlay ov;
    rowsFor(SEC_DISP, SetSnap(), &ov);
    float x0, y0, hw0, hh0, x1, y1, hw1, hh1;
    setRowRect(ov, 0, &x0, &y0, &hw0, &hh0);
    ov.scroll = 0.1f;
    setRowRect(ov, 0, &x1, &y1, &hw1, &hh1);
    CHECK_F(y1 - y0, 0.1f, 1e-5f);      // scroll lifts the strip
    CHECK_F(x0, x1, 1e-5f);
    float sx, sy, shw, shh;
    setSecRect(0, &sx, &sy, &shw, &shh);
    CHECK(x0 - hw0 > sx + shw);            // pane right of the rail
    }

    // ---- clipBandAndShown
    {
    SetOverlay ov;
    SetSnap snap;
    snap.wifi.resize(30);
    rowsFor(SEC_WIFI, snap, &ov);
    float lo, hi;
    setClipBand(&lo, &hi);
    CHECK(hi > lo);
    CHECK(setRowShown(ov, 0));
    CHECK(!setRowShown(ov, 32));           // scrolled far past the band
    ov.scroll = setScrollMax(ov);
    CHECK(setRowShown(ov, 32));
    }

    // ---- scrollClamp
    {
    SetOverlay ov;
    SetSnap snap;
    rowsFor(SEC_WIFI, snap, &ov);           // short list: nothing to scroll
    CHECK_F(setScrollMax(ov), 0.0f, 1e-5f);
    CHECK_F(setClampScroll(ov, 0.5f), 0.0f, 1e-5f);
    snap.wifi.resize(40);
    rowsFor(SEC_WIFI, snap, &ov);
    CHECK(setScrollMax(ov) > 0.1f);
    CHECK_F(setClampScroll(ov, -1.0f), 0.0f, 1e-5f);
    CHECK_F(setClampScroll(ov, 99.0f), setScrollMax(ov), 1e-5f);
    const float mid = setScrollMax(ov) * 0.5f;
    CHECK_F(setScrollBy(ov, 1.0f) - setScrollBy(ov, 0.0f),
                kSetWheelStep, 1e-5f);
    ov.scroll = mid;
    CHECK_F(setScrollBy(ov, 0.0f), mid, 1e-5f);
    }

    // ---- slopPassed
    {
    CHECK(!setSlopPassed(0.0f, 0.0f));
    CHECK(!setSlopPassed(0.0f, kSetDragSlop * 0.5f / kSetHH));
    CHECK(setSlopPassed(0.0f, kSetDragSlop * 2.0f / kSetHH));
    }

    // ---- sliderValue
    {
    float l, r;
    setSliderTrack(&l, &r);
    CHECK(l < r);
    CHECK_F(setSliderValue(l / kSetHW), 0.0f, 1e-5f);
    CHECK_F(setSliderValue(r / kSetHW), 1.0f, 1e-5f);
    CHECK_F(setSliderValue(2.0f), 1.0f, 1e-5f);   // past the track clamps
    CHECK_F(setSliderValue(-2.0f), 0.0f, 1e-5f);

    // ---- scanPillFitsLabel
    {
    float x0, hw0, x1, hw1, x2, hw2;
    setScanPill(0.0f, &x0, &hw0);
    CHECK_F(hw0, kSetPillHW0, 1e-5f);        // empty label keeps a tap size
    const float rowR = (kSetHW - kSetPad);
    CHECK_F(x0 + hw0, rowR - kSetPillMR, 1e-5f);   // right-anchored
    setScanPill(0.10f, &x1, &hw1);
    CHECK_F(hw1, 0.05f + kSetPillPadX, 1e-5f);     // grows with the label
    CHECK(x1 - hw1 < rowR - 0.10f);            // reaches past the old pill
    CHECK_F(x1 + hw1, x0 + hw0, 1e-5f);            // same right edge
    setScanPill(9.0f, &x2, &hw2);
    CHECK_F(hw2, kSetPillHW1, 1e-5f);              // wide label clamps
    CHECK_F(setScanPillL(), x2 - hw2, 1e-5f);      // zone edge = widest pill
    CHECK(x1 - hw1 > setScanPillL());              // any pill stays in zone
    }
}

// ------------------------------------------------------------- policy    }

    // ---- segTables
    {
    CHECK(setSegCount(PK_THEME) == 3);
    CHECK(setSegCount(PK_LANG) == 2);
    CHECK(setSegCount(PK_DOF) == 2);
    CHECK(std::string(setSegValue(PK_THEME, 2)) == "oled");
    CHECK(std::string(setSegValue(PK_LANG, 1)) == "zh-CN");
    CHECK(std::string(setSegValue(PK_DOF, 1)) == "6dof");
    }

    // ---- segCurrent
    {
    SetSnap snap;
    snap.txt[TX_THEME] = "light";
    CHECK(setSegCurrent(PK_THEME, snap) == 1);
    snap.txt[TX_THEME] = "oled";
    CHECK(setSegCurrent(PK_THEME, snap) == 2);
    snap.txt[TX_LANG] = "en-US";
    CHECK(setSegCurrent(PK_LANG, snap) == 0);
    snap.txt[TX_LANG] = "zh-CN";
    CHECK(setSegCurrent(PK_LANG, snap) == 1);
    snap.txt[TX_LANG] = "zh-Hans-SG";
    CHECK(setSegCurrent(PK_LANG, snap) == 1);
    snap.txt[TX_LANG] = "fr";
    CHECK(setSegCurrent(PK_LANG, snap) == -1);
    snap.toggles = 1u << TG_DOF6;
    CHECK(setSegCurrent(PK_DOF, snap) == 1);
    snap.toggles = 0;
    CHECK(setSegCurrent(PK_DOF, snap) == 0);
    }

    // ---- implementedGate
    {
    CHECK(!setImplemented(RID_NIGHT));
    CHECK(!setImplemented(RID_ADB));
    CHECK(!setImplemented(RID_STAY));
    CHECK(!setImplemented(RID_TOUCHES));
    CHECK(setImplemented(RID_WIFI_ON));
    CHECK(setImplemented(RID_DEBUGHUD));
    }

    // ---- wifiNeedsPw
    {
    SetWifi n;
    n.sec = WSEC_WPA; n.saved = -1; n.conn = false;
    CHECK(setWifiNeedsPw(n));
    n.sec = WSEC_OPEN;
    CHECK(!setWifiNeedsPw(n));             // open net: join straight away
    n.sec = WSEC_WPA; n.saved = 3;
    CHECK(!setWifiNeedsPw(n));             // saved: no password prompt
    n.saved = -1; n.conn = true;
    CHECK(!setWifiNeedsPw(n));             // live net: manage dialog
    }

    // ---- envIds
    {
    SetSnap snap;
    SetEnvOpt e; e.id = "alpine";
    snap.envs.push_back(e);
    CHECK(std::string(setEnvId(snap, 0)) == "passthrough");
    CHECK(std::string(setEnvId(snap, 1)) == "builtin");
    CHECK(std::string(setEnvId(snap, 2)) == "alpine");
    CHECK(std::string(setEnvId(snap, 9)) == "");
    snap.txt[TX_ENV] = "alpine";
    CHECK(setEnvActive(snap, 2));
    CHECK(!setEnvActive(snap, 0));
    snap.txt[TX_ENV] = "";
    CHECK(setEnvActive(snap, 0));          // empty means passthrough
    }

    // ---- btConfirm
    {
    SetBtDev d;
    CHECK(!setBtNeedsConfirm(d));          // unbonded pairs immediately
    d.bonded = true;
    CHECK(setBtNeedsConfirm(d));
}

// ------------------------------------------------------------- dialog    }

    // ---- dlgButtons
    {
    SetOverlay ov;
    ov.dlg = DLG_REBOOT;
    CHECK(setDlgBtnCount(DLG_REBOOT, ov) == 2);
    SetWifi n;
    n.sec = WSEC_WPA; n.saved = -1;
    ov.snap.wifi.push_back(n);
    ov.dlg = DLG_WIFI; ov.dlgArg = 0;
    CHECK(setDlgBtnCount(DLG_WIFI, ov) == 2);   // cancel + connect
    ov.snap.wifi[0].saved = 2;
    CHECK(setDlgBtnCount(DLG_WIFI, ov) == 3);   // + forget
    ov.snap.wifi[0].conn = true;
    CHECK(setDlgBtnCount(DLG_WIFI, ov) == 2);   // forget + cancel
    float x, y, hw, hh, x2, y2, hw2, hh2;
    setDlgBtnRect(DLG_WIFI, true, 0, &x, &y, &hw, &hh);
    setDlgBtnRect(DLG_WIFI, true, 2, &x2, &y2, &hw2, &hh2);
    CHECK(x < x2);                          // destructive left of confirm
    CHECK_F(y, y2, 1e-5f);
    }

    // ---- dlgRectKeyboardGrows
    {
    float x0, y0, hw0, hh0, x1, y1, hw1, hh1;
    setDlgRect(DLG_WIFI, false, &x0, &y0, &hw0, &hh0);
    setDlgRect(DLG_WIFI, true, &x1, &y1, &hw1, &hh1);
    CHECK(hh1 > hh0);                       // the password dialog is tall
    CHECK(y1 < y0);                         // and slides low on the card
}

// ------------------------------------------------------------- kbd    }

    // ---- kbdPages
    {
    CHECK(setKbdCount(0) > 30);
    CHECK(setKbdCount(1) > 20);
    CHECK(setKbdCount(9) == 0);
    int foundShift = 0, foundDone = 0, foundPage = 0, foundBksp = 0;
    for (int i = 0; i < setKbdCount(0); ++i) {
        const int c = setKeyCode(0, i, false);
        if (c == SK_SHIFT) ++foundShift;
        if (c == SK_DONE) ++foundDone;
        if (c == SK_PAGE) ++foundPage;
        if (c == SK_BKSP) ++foundBksp;
    }
    CHECK(foundShift == 1 && foundDone >= 1 && foundPage == 1 &&
           foundBksp == 1);
    // shift uppercases letters on the letters page only
    int q = -1;
    for (int i = 0; i < setKbdCount(0); ++i)
        if (setKeyCode(0, i, false) == 'q') { q = i; break; }
    CHECK(q >= 0);
    CHECK(setKeyCode(0, q, true) == 'Q');
    CHECK(setKeyCode(1, q, true) != 'Q' || setKeyCode(1, q, true) == 0 ||
           setKeyCode(1, q, false) == setKeyCode(1, q, true));
    CHECK(std::string(setKeyLabel(0, 0)).empty() ||
           setKeyLabel(0, 0) != nullptr);
    }

    // ---- kbdRectsInside
    {
    float x, y, hw, hh;
    for (int i = 0; i < setKbdCount(0); ++i) {
        setKeyRect(0, i, &x, &y, &hw, &hh);
        CHECK(hw > 0.0f && hh > 0.0f);
        CHECK(std::fabs(x) <= kSetHW);      // every key sits on the card
        CHECK(std::fabs(y) <= kSetHH);
    }
    float x0, y0, hw0, hh0, x1, y1, hw1, hh1;
    setKeyRect(0, 0, &x0, &y0, &hw0, &hh0);
    setKeyRect(0, 10, &x1, &y1, &hw1, &hh1);
    CHECK_F(x0, x1, 0.01f);              // same column, next row
    CHECK(y0 > y1);
}

// ------------------------------------------------------------- picking    }

    // ---- zoneTitleAndSec
    {
    SetOverlay ov;
    rowsFor(SEC_DISP, SetSnap(), &ov);
    int idx, aux;
    // close disc
    const float cx = setCloseX() / kSetHW, cy = setTitleY() / kSetHH;
    CHECK(setZoneAt(ov, cx, cy, &idx, &aux) == SZONE_CLOSE);
    // title band away from the disc eats the hit
    CHECK(setZoneAt(ov, 0.0f, 0.97f, &idx, &aux) == SZONE_BODY);
    // a sidebar row
    float sx, sy, hw, hh;
    setSecRect(2, &sx, &sy, &hw, &hh);
    CHECK(setZoneAt(ov, sx / kSetHW, sy / kSetHH, &idx, &aux) ==
           SZONE_SEC);
    CHECK(idx == 2);
    // the same slot on a desktop overlay resolves to its visible sec
    SetOverlay desk;
    desk.android = false;
    CHECK(setZoneAt(desk, sx / kSetHW, sy / kSetHH, &idx, &aux) ==
           SZONE_SEC);
    CHECK(idx == SEC_DISP);
    setSecRect(5, &sx, &sy, &hw, &hh);
    CHECK(setZoneAt(desk, sx / kSetHW, sy / kSetHH, &idx, &aux) ==
           SZONE_SEC);
    CHECK(idx == SEC_ABOUT);
    }

    // ---- zoneRows
    {
    SetOverlay ov;
    SetSnap snap;
    rowsFor(SEC_DISP, snap, &ov);
    int idx, aux;
    float rx, ry, hw, hh;
    setRowRect(ov, 0, &rx, &ry, &hw, &hh);
    // row 0 is the brightness slider: a hit mid-track is the slider zone
    float tl, tr;
    setSliderTrack(&tl, &tr);
    CHECK(setZoneAt(ov, ((tl + tr) * 0.5f) / kSetHW, ry / kSetHH,
                   &idx, &aux) == SZONE_SLIDER);
    CHECK(idx == 0);
    // left of the track is the row body
    CHECK(setZoneAt(ov, (rx - hw * 0.5f) / kSetHW, ry / kSetHH,
                   &idx, &aux) == SZONE_ROW);
    // the theme segmented row: a hit on cell 2
    float sx, sy, shw, shh;
    setSegRect(PK_THEME, 2, 0.0f, &sx, &sy, &shw, &shh);
    setRowRect(ov, 1, &rx, &ry, &hw, &hh);
    CHECK(setZoneAt(ov, sx / kSetHW, ry / kSetHH, &idx, &aux) ==
           SZONE_SEG);
    CHECK(idx == 1 && aux == 2);
    // off the card
    CHECK(setZoneAt(ov, 1.5f, 0.0f, &idx, &aux) == SZONE_NONE);
    }

    // ---- zoneScrollShifts
    {
    SetOverlay ov;
    SetSnap snap;
    snap.wifi.resize(20);
    rowsFor(SEC_WIFI, snap, &ov);
    int idx, aux;
    float rx, ry, hw, hh;
    // before scroll row 3 sits at the pane's y; after scroll a later row
    ov.scroll = 0.0f;
    setRowRect(ov, 3, &rx, &ry, &hw, &hh);
    const float u = rx / kSetHW, v = ry / kSetHH;
    CHECK(setZoneAt(ov, u, v, &idx, &aux) == SZONE_ROW && idx == 3);
    ov.scroll = setScrollMax(ov);
    CHECK(setZoneAt(ov, u, v, &idx, &aux) == SZONE_ROW);
    CHECK(idx > 3);                          // a deeper row slid under
    }

    // ---- zoneScanPill
    {
    SetOverlay ov;
    SetSnap snap;
    rowsFor(SEC_BT, snap, &ov);
    int idx, aux;
    float rx, ry, hw, hh;
    setRowRect(ov, 1, &rx, &ry, &hw, &hh);   // row 1 is the scan header
    // the pill's right edge lands in the scan zone
    CHECK(setZoneAt(ov, (rx + hw - 0.02f) / kSetHW, ry / kSetHH,
                   &idx, &aux) == SZONE_SCAN);
    CHECK(idx == 1);
    // so does the widest pill's left edge; left of it is the row body
    CHECK(setZoneAt(ov, (setScanPillL() + 0.005f) / kSetHW, ry / kSetHH,
                   &idx, &aux) == SZONE_SCAN);
    CHECK(setZoneAt(ov, (setScanPillL() - 0.005f) / kSetHW, ry / kSetHH,
                   &idx, &aux) == SZONE_ROW);
    }

    // ---- zoneModal
    {
    SetOverlay ov;
    SetSnap snap;
    SetWifi n;
    n.sec = WSEC_WPA; n.saved = -1;
    snap.wifi.push_back(n);
    rowsFor(SEC_WIFI, snap, &ov);
    ov.dlg = DLG_WIFI; ov.dlgArg = 0;
    int idx, aux;
    // a row hit lands on the modal instead
    float rx, ry, hw, hh;
    setRowRect(ov, 0, &rx, &ry, &hw, &hh);
    CHECK(setZoneAt(ov, rx / kSetHW, ry / kSetHH, &idx, &aux) ==
           SZONE_BODY);
    // a dialog button resolves
    float bx, by, bhw, bhh;
    setDlgBtnRect(DLG_WIFI, true, 2, &bx, &by, &bhw, &bhh);
    CHECK(setZoneAt(ov, bx / kSetHW, by / kSetHH, &idx, &aux) ==
           SZONE_BTN);
    CHECK(idx == 1);                         // 2 buttons: slots 1,2
    // a key resolves
    float kx, ky, khw, khh;
    setKeyRect(0, 0, &kx, &ky, &khw, &khh);
    CHECK(setZoneAt(ov, kx / kSetHW, ky / kSetHH, &idx, &aux) ==
           SZONE_KEY);
    CHECK(idx == 0);
    }

    // ---- pickRay
    {
    SetOverlay ov;
    rowsFor(SEC_DISP, SetSnap(), &ov);
    const float origin[3] = {0.0f, 0.0f, 0.0f};
    // aim straight at the card's centre on the anchor cylinder
    float c[3], r[3], up[3];
    setCenter(0.0f, 0.0f, origin, c, r, up);
    float d[3] = {c[0] - origin[0], c[1] - origin[1], c[2] - origin[2]};
    const float len = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
    d[0] /= len; d[1] /= len; d[2] /= len;
    SetHit pk = pickSetRay(ov, 0.0f, 0.0f, origin, origin, d);
    CHECK(pk.hit);
    CHECK(pk.zone != SZONE_NONE);
    // a ray off the card misses
    float d2[3] = {d[0], d[1] + 2.0f, d[2]};
    SetHit miss = pickSetRay(ov, 0.0f, 0.0f, origin, origin, d2);
    CHECK(!miss.hit);
    }
}
