#include "test.h"

#include "quick/layout.h"
#include "dock/item.h"
#include "common/config.h"
#include "math/head.h"

#include <cmath>

static const float o0[3] = {0.0f, 0.0f, 0.0f};

void testQuick() {
    // the catalog keeps the old panel's order: four large radio tiles
    // then the small grid; the inert tiles stay marked unimplemented
    {
        CHECK(quickSpec(QS_WIFI).large && quickSpec(QS_WIFI).implemented);
        CHECK(quickSpec(QS_BOUNDARY).large &&
              !quickSpec(QS_BOUNDARY).implemented);
        CHECK(quickSpec(QS_SEETHROUGH).large &&
              !quickSpec(QS_SEETHROUGH).implemented);
        CHECK(!quickSpec(QS_MIC).large && quickSpec(QS_MIC).implemented);
        CHECK(quickSpec(QS_RESET_VIEW).implemented &&
              !quickSpec(QS_RESET_VIEW).toggle);
        CHECK(!quickSpec(QS_REPORT).implemented);
        CHECK(quickSpec(QS_ABOUT).implemented &&
              !quickSpec(QS_ABOUT).toggle);
    }

    // tile rects: the large row spans the card evenly, small tiles grid
    // under it, everything inside the card's inner padding
    {
        float x, y, hw, hh;
        quickTileRect(QS_WIFI, &x, &y, &hw, &hh);
        CHECK(hw > 0.0f && hh > 0.0f);
        CHECK(x - hw >= -kQuickHW + kQuickPad - 1e-4f);
        CHECK(y + hh <= kQuickHH - kQuickHeadH + 1e-4f);
        quickTileRect(QS_SEETHROUGH, &x, &y, &hw, &hh);
        CHECK(x + hw <= kQuickHW - kQuickPad + 1e-4f);
        float lx0;
        quickTileRect(QS_WIFI, &lx0, &y, &hw, &hh);
        CHECK(lx0 < x);
        quickTileRect(QS_ABOUT, &x, &y, &hw, &hh);
        CHECK(y - hh >= -kQuickHH + 1e-4f);
        CHECK(y - hh < -0.2f);
        float y0, y1;
        quickTileRect(QS_MIC, &x, &y0, &hw, &hh);
        quickTileRect(QS_MIC + kQuickCols, &x, &y1, &hw, &hh);
        CHECK_F(y1, y0 - kQuickCellH, 1e-6f);
    }

    // tile text: the label sits under the icon and the value keeps a gap
    // under it - at the drawn text sizes the two ink boxes need ~0.021m
    // between centres before they stop touching; both stay in the tile
    {
        float ly, sy;
        quickTileTextY(true, &ly, &sy);
        CHECK(ly < 0.0f && sy < ly);
        CHECK(sy > -1.0f);
        float x, y, hw, hh;
        quickTileRect(QS_WIFI, &x, &y, &hw, &hh);
        CHECK((ly - sy) * hh >= 0.024f);
        CHECK(y + sy * hh - 0.010f > y - hh);
        quickTileTextY(false, &ly, &sy);
        CHECK(ly < 0.0f && ly > -1.0f);
    }

    // zones: a tile centre picks the tile, the slider bands resolve their
    // row, the title row answers close/gear, the gaps are body
    {
        int idx = -1, sl = -1;
        float x, y, hw, hh;
        quickTileRect(QS_BLUETOOTH, &x, &y, &hw, &hh);
        CHECK(quickZoneAt(x / kQuickHW, y / kQuickHH, &idx, &sl) ==
              QZONE_TILE);
        CHECK(idx == QS_BLUETOOTH);
        const float sy = quickSliderY(QSLIDER_VOLUME);
        CHECK(quickZoneAt(0.0f, sy / kQuickHH, &idx, &sl) == QZONE_SLIDER);
        CHECK(sl == QSLIDER_VOLUME);
        const float sy0 = quickSliderY(QSLIDER_BRIGHTNESS);
        CHECK(quickZoneAt(0.0f, sy0 / kQuickHH, &idx, &sl) == QZONE_SLIDER);
        CHECK(sl == QSLIDER_BRIGHTNESS);
        const float ty = quickTitleY();
        CHECK(quickZoneAt(quickCloseX() / kQuickHW, ty / kQuickHH,
                          &idx, &sl) == QZONE_CLOSE);
        CHECK(quickZoneAt(quickGearX() / kQuickHW, ty / kQuickHH,
                          &idx, &sl) == QZONE_GEAR);
        CHECK(quickZoneAt(0.0f, ty / kQuickHH, &idx, &sl) == QZONE_BODY);
        CHECK(quickZoneAt(0.0f, -0.99f, &idx, &sl) == QZONE_BODY);
    }

    // slider value: the track ends clamp to 0 and 1
    {
        float tl, tr;
        quickSliderTrack(&tl, &tr);
        CHECK(tl < tr);
        CHECK_F(quickSliderValue(tl / kQuickHW), 0.0f, 1e-6f);
        CHECK_F(quickSliderValue(tr / kQuickHW), 1.0f, 1e-6f);
        CHECK_F(quickSliderValue((tl + tr) * 0.5f / kQuickHW), 0.5f, 1e-4f);
        CHECK_F(quickSliderValue(-1.0f), 0.0f, 1e-6f);
        CHECK_F(quickSliderValue(1.0f), 1.0f, 1e-6f);
    }

    // activate policy: inert tiles resolve to nothing, toggles flip,
    // reset view is native, about opens settings
    {
        QsStatus st;
        CHECK(quickActivateOp(st, QS_BOUNDARY) == QOP_NONE);
        CHECK(quickActivateOp(st, QS_SEETHROUGH) == QOP_NONE);
        CHECK(quickActivateOp(st, QS_REPORT) == QOP_NONE);
        CHECK(quickActivateOp(st, QS_WIFI) == QOP_TOGGLE);
        CHECK(quickActivateOp(st, QS_MIC) == QOP_TOGGLE);
        CHECK(quickActivateOp(st, QS_RESET_VIEW) == QOP_RECENTER);
        CHECK(quickActivateOp(st, QS_ABOUT) == QOP_SETTINGS);
        CHECK(quickActivateOp(st, -1) == QOP_NONE);
        CHECK(quickActivateOp(st, QS_COUNT) == QOP_NONE);
    }

    // toggle bits and subtitles: wifi carries the ssid when one's known,
    // off state reads off, an unknown ssid reads not-connected
    {
        QsStatus st;
        CHECK(!quickToggleOn(st, QS_WIFI));
        CHECK(quickSubtitle(st, QS_WIFI).kind == QSUB_OFF);
        st.toggles |= 1u << QS_WIFI;
        CHECK(quickToggleOn(st, QS_WIFI));
        CHECK(quickSubtitle(st, QS_WIFI).kind == QSUB_NOTCONN);
        st.ssid = "Cafe";
        QsSub s = quickSubtitle(st, QS_WIFI);
        CHECK(s.kind == QSUB_CUSTOM && s.text == st.ssid.c_str());
        st.toggles |= 1u << QS_MIC;
        CHECK(quickSubtitle(st, QS_MIC).kind == QSUB_ON);
        st.toggles |= 1u << QS_BLUETOOTH;
        st.btName = "Pods";
        s = quickSubtitle(st, QS_BLUETOOTH);
        CHECK(s.kind == QSUB_CUSTOM && s.text == st.btName.c_str());
    }

    // the wifi tile swaps its fan for the plug while a cable is patched;
    // every other tile keeps its own glyph
    {
        QsStatus st;
        CHECK(quickTileIcon(st, QS_WIFI) == QS_WIFI);
        st.net = DNET_WIFI;
        CHECK(quickTileIcon(st, QS_WIFI) == QS_WIFI);
        st.net = DNET_ETH;
        CHECK(quickTileIcon(st, QS_WIFI) == QS_ICON_ETH);
        CHECK(quickTileIcon(st, QS_MIC) == QS_MIC);
    }

    // the wired state: the tile reads "Ethernet", lit, untappable, with
    // no ssid to carry it falls back to the on word
    {
        QsStatus st;
        CHECK(!quickTileOn(st, QS_WIFI));
        CHECK(quickTileLabel(st, QS_WIFI) == UIQS_LABEL0 + QS_WIFI);
        st.net = DNET_ETH;
        CHECK(quickTileOn(st, QS_WIFI));
        CHECK(!quickTileOn(st, QS_MIC));
        CHECK(quickTileLabel(st, QS_WIFI) == UIQS_ETH);
        CHECK(quickTileLabel(st, QS_MIC) == UIQS_LABEL0 + QS_MIC);
        CHECK(quickSubtitle(st, QS_WIFI).kind == QSUB_ON);
        CHECK(quickActivateOp(st, QS_WIFI) == QOP_NONE);
        CHECK(quickActivateOp(st, QS_MIC) == QOP_TOGGLE);
    }

    // the card sits on the dash's anchor cylinder at the grid's distance:
    // a ray at its centre hits, off-card rays miss, a tile ray picks it
    {
        float c[3], r[3], up[3];
        quickCenter(0.0f, 0.0f, kPanelDist, o0, c, r, up);
        CHECK_F(c[2], -kQuickDist, 1e-6f);
        CHECK(kQuickDist < kPanelDist && kQuickDist > kDockDist);
        float d[3] = {c[0], c[1], c[2]};
        const float dl = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        d[0] /= dl; d[1] /= dl; d[2] /= dl;
        QsPick pk = pickQuickRay(0.0f, 0.0f, kPanelDist, o0, o0, d);
        // dead centre lands in the large row's tile gap: card body
        CHECK(pk.hit && pk.zone == QZONE_BODY);
        float tx, ty, thw, thh;
        quickTileRect(QS_WIFI, &tx, &ty, &thw, &thh);
        float d2[3] = {c[0] + r[0]*tx + up[0]*ty,
                       c[1] + r[1]*tx + up[1]*ty,
                       c[2] + r[2]*tx + up[2]*ty};
        const float dl2 = sqrtf(d2[0]*d2[0] + d2[1]*d2[1] + d2[2]*d2[2]);
        d2[0] /= dl2; d2[1] /= dl2; d2[2] /= dl2;
        pk = pickQuickRay(0.0f, 0.0f, kPanelDist, o0, o0, d2);
        CHECK(pk.hit && pk.zone == QZONE_TILE && pk.idx == QS_WIFI);
        float d3[3] = {c[0] + r[0] * (kQuickHW + 0.4f), c[1],
                       c[2] + r[2] * (kQuickHW + 0.4f)};
        const float dl3 = sqrtf(d3[0]*d3[0] + d3[1]*d3[1] + d3[2]*d3[2]);
        d3[0] /= dl3; d3[1] /= dl3; d3[2] /= dl3;
        pk = pickQuickRay(0.0f, 0.0f, kPanelDist, o0, o0, d3);
        CHECK(!pk.hit);
    }
}
