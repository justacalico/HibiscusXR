#include "layout.h"

#include "../common/config.h"
#include "../panels/layout.h"

#include <cmath>

// the catalog in display order: the large radio row, then the small grid.
// implemented=false marks the tiles whose platform side doesn't exist yet
// - they draw greyed and take no presses, same rule the flutter panel used
static const QsSpec kSpecs[QS_COUNT] = {
    {QS_WIFI,        true,  true,  true},
    {QS_BOUNDARY,    true,  false, true},
    {QS_BLUETOOTH,   true,  true,  true},
    {QS_SEETHROUGH,  true,  false, true},
    {QS_MIC,         false, true,  true},
    {QS_RESET_VIEW,  false, true,  false},
    {QS_NIGHT,       false, true,  true},
    {QS_DND,         false, true,  true},
    {QS_AIRPLANE,    false, true,  true},
    {QS_SAVER,       false, true,  true},
    {QS_CENTER,      false, true,  true},
    {QS_REPORT,      false, false, false},
    {QS_ABOUT,       false, true,  false},
};

const QsSpec& quickSpec(int i) {
    return kSpecs[i];
}

void quickCenter(float yaw, float pitch, const float origin[3],
                 float c[3], float r[3], float up[3]) {
    // same anchor cylinder the windows and the app grid ride: the card
    // hangs in front of the window slots, just behind the dock strip
    ringPoint(yaw, pitch, kQuickDist, kPanelY, origin, c, r, up);
}

// vertical chain under the title band: slider rows, the large row, then
// the small grid rows, each separated by a fixed gap
float quickSliderY(int i) {
    return kQuickHH - kQuickHeadH - 0.014f -
           kQuickSliderH * 0.5f - i * (kQuickSliderH + kQuickSliderGap);
}

static float quickLargeY() {
    return quickSliderY(QSLIDER_COUNT - 1) - kQuickSliderH * 0.5f -
           0.016f - kQuickLargeH * 0.5f;
}

static float quickSmallY(int row) {
    return quickLargeY() - kQuickLargeH * 0.5f - 0.016f -
           kQuickCellH * 0.5f - row * kQuickCellH;
}

void quickTileRect(int i, float* x, float* y, float* hw, float* hh) {
    const QsSpec& s = quickSpec(i);
    const float inner = kQuickHW * 2.0f - kQuickPad * 2.0f;
    if (s.large) {
        const float cellW = inner / kQuickLargeCols;
        *x = -kQuickHW + kQuickPad + cellW * (i + 0.5f);
        *y = quickLargeY();
        *hw = cellW * 0.5f - 0.008f;
        *hh = kQuickLargeH * 0.5f - 0.006f;
    } else {
        const int si = i - kQuickLargeCols;
        const float cellW = inner / kQuickCols;
        *x = -kQuickHW + kQuickPad + cellW * (si % kQuickCols + 0.5f);
        *y = quickSmallY(si / kQuickCols);
        *hw = cellW * 0.5f - 0.008f;
        *hh = kQuickCellH * 0.5f - 0.007f;
    }
}

void quickSliderTrack(float* l, float* r) {
    // the icon owns the left end of the row; the track fills the rest out
    // to the card's inner edge
    *l = -kQuickHW + kQuickPad + 0.058f;
    *r = kQuickHW - kQuickPad;
}

float quickSliderValue(float u) {
    float l, r;
    quickSliderTrack(&l, &r);
    const float x = u * kQuickHW;
    const float v = (x - l) / (r - l);
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

float quickCloseX() {
    return kQuickHW - kQuickPad - kQuickCloseR;
}

float quickGearX() {
    return quickCloseX() - kQuickCloseR * 2.0f - 0.016f - kQuickGearR;
}

float quickTitleY() {
    return kQuickHH - kQuickHeadH * 0.5f;
}

bool quickToggleOn(const QsStatus& st, int id) {
    return (st.toggles & (1u << id)) != 0;
}

QsSub quickSubtitle(const QsStatus& st, int id) {
    QsSub s;
    if (!quickToggleOn(st, id)) { s.kind = QSUB_OFF; return s; }
    if (id == QS_WIFI && !st.ssid.empty()) {
        s.kind = QSUB_CUSTOM; s.text = st.ssid.c_str();
    } else if (id == QS_BLUETOOTH && !st.btName.empty()) {
        s.kind = QSUB_CUSTOM; s.text = st.btName.c_str();
    } else if (id == QS_WIFI || id == QS_BLUETOOTH) {
        s.kind = QSUB_NOTCONN;
    } else {
        s.kind = QSUB_ON;
    }
    return s;
}

QsOp quickActivateOp(int id) {
    if (id < 0 || id >= QS_COUNT) return QOP_NONE;
    const QsSpec& s = kSpecs[id];
    if (!s.implemented) return QOP_NONE;
    if (s.toggle) return QOP_TOGGLE;
    if (id == QS_RESET_VIEW) return QOP_RECENTER;
    return QOP_SETTINGS;   // QS_ABOUT
}

int quickZoneAt(float u, float v, int* idx, int* slider) {
    *idx = -1;
    *slider = -1;
    if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) return QZONE_NONE;
    const float x = u * kQuickHW, y = v * kQuickHH;
    if (y > kQuickHH - kQuickHeadH) {
        const float ty = quickTitleY();
        const float cdx = x - quickCloseX(), cdy = y - ty;
        const float cr = kQuickCloseR + 0.012f;
        if (cdx * cdx + cdy * cdy <= cr * cr) return QZONE_CLOSE;
        const float gdx = x - quickGearX(), gdy = y - ty;
        const float gr = kQuickGearR + 0.012f;
        if (gdx * gdx + gdy * gdy <= gr * gr) return QZONE_GEAR;
        return QZONE_BODY;
    }
    for (int i = 0; i < QSLIDER_COUNT; ++i) {
        const float sy = quickSliderY(i);
        if (fabsf(y - sy) <= kQuickSliderH * 0.5f + 0.006f) {
            *slider = i;
            return QZONE_SLIDER;
        }
    }
    for (int i = 0; i < QS_COUNT; ++i) {
        float tx, ty, hw, hh;
        quickTileRect(i, &tx, &ty, &hw, &hh);
        if (fabsf(x - tx) <= hw + 0.004f && fabsf(y - ty) <= hh + 0.004f) {
            *idx = i;
            return QZONE_TILE;
        }
    }
    return QZONE_BODY;
}

QsPick pickQuickRay(float yaw, float pitch, const float origin[3],
                    const float o[3], const float d[3]) {
    QsPick pk;
    float c[3], r[3], up[3];
    quickCenter(yaw, pitch, origin, c, r, up);
    if (!rayQuad(c, r, up, origin, o, d, kQuickHW, kQuickHH,
                 &pk.u, &pk.v, &pk.t)) return pk;
    if (fabsf(pk.u) > 1.0f || fabsf(pk.v) > 1.0f) return pk;
    pk.hit = true;
    pk.zone = quickZoneAt(pk.u, pk.v, &pk.idx, &pk.slider);
    return pk;
}
