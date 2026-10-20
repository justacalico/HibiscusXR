#include "layout.h"

#include "../anim/anim.h"
#include "../common/config.h"
#include "../panels/layout.h"

#include <cmath>
#include <cstdio>

void gridCenter(float yaw, float pitch, const float origin[3],
                float c[3], float r[3], float up[3]) {
    // same anchor cylinder the windows and strip ride: the card hangs at
    // its own distance so it lands in front of the windows' plane
    ringPoint(yaw, pitch, kGridDist, kPanelY, origin, c, r, up);
}

void tickGridHover(std::vector<GridItem>& items, int hover, float dtMs) {
    for (int i = 0; i < (int)items.size(); ++i)
        items[i].hs = dampMs(items[i].hs,
                             i == hover ? kGridHoverScale : 1.0f,
                             dtMs, kHoverTauMs);
}

float gridLayout(std::vector<GridItem>& items) {
    const float cellW = (kGridHW * 2.0f - kGridSidePad * 2.0f) / kGridCols;
    const float x0 = -kGridHW + kGridSidePad + cellW * 0.5f;
    const float y0 = kGridHH - kGridHeadH - kGridCellH * 0.5f;
    int i = 0;
    for (auto& it : items) {
        it.x = x0 + (i % kGridCols) * cellW;
        it.y = y0 - (i / kGridCols) * kGridCellH;
        ++i;
    }
    const int rows = ((int)items.size() + kGridCols - 1) / kGridCols;
    return rows * kGridCellH;
}

void gridClipBand(float* lo, float* hi) {
    *lo = -kGridHH;
    *hi = kGridHH - kGridHeadH;
}

bool gridCellShown(float iy, float gs) {
    float lo, hi;
    gridClipBand(&lo, &hi);
    const float half = kGridCellH * 0.5f * gs;
    return iy + half >= lo * gs - 0.02f && iy - half <= hi * gs + 0.02f;
}

float gridScrollMax(int n) {
    const int rows = (n + kGridCols - 1) / kGridCols;
    const float content = rows * kGridCellH;
    float lo, hi;
    gridClipBand(&lo, &hi);
    const float over = content - (hi - lo);
    return over > 0.0f ? over : 0.0f;
}

float gridClampScroll(float scroll, int n) {
    const float mx = gridScrollMax(n);
    return scroll < 0.0f ? 0.0f : scroll > mx ? mx : scroll;
}

bool gridSlopPassed(float grabV, float v) {
    return fabsf(v - grabV) * kGridHH > kGridDragSlop;
}

float gridScrollBy(float scroll, float delta, int n) {
    return gridClampScroll(scroll + delta * kGridWheelStep, n);
}

int gridItemAt(int n, float u, float v, float scroll) {
    const float x = u * kGridHW, y = v * kGridHH;
    float lo, hi;
    gridClipBand(&lo, &hi);
    if (y > hi || y < lo) return -1;
    const float cellW = (kGridHW * 2.0f - kGridSidePad * 2.0f) / kGridCols;
    const int col = (int)floorf((x + kGridHW - kGridSidePad) / cellW);
    if (col < 0 || col >= kGridCols) return -1;
    const int row = (int)floorf((hi - y + scroll) / kGridCellH);
    const int idx = row * kGridCols + col;
    return idx < n ? idx : -1;
}

float gridCloseX() {
    return kGridHW - kGridSidePad - kGridCloseR;
}

// ------------------------------------------------------- groups + menus

bool groupHas(const LibGroup& g, const std::string& pkg) {
    for (const std::string& p : g.pkgs) if (p == pkg) return true;
    return false;
}

void groupToggle(LibGroup* g, const std::string& pkg) {
    for (size_t i = 0; i < g->pkgs.size(); ++i)
        if (g->pkgs[i] == pkg) {
            g->pkgs.erase(g->pkgs.begin() + i);
            return;
        }
    g->pkgs.push_back(pkg);
}

std::string groupNewName(const std::vector<LibGroup>& gs,
                         const char* fmt) {
    char cand[64];
    for (int n = 1;; ++n) {
        snprintf(cand, sizeof(cand), fmt ? fmt : "Group %d", n);
        bool taken = false;
        for (const LibGroup& g : gs)
            if (g.name == cand) { taken = true; break; }
        if (!taken) return cand;
    }
}

std::vector<GridItem> gridVisible(const std::vector<GridItem>& all,
                                  const std::vector<LibGroup>& gs,
                                  int sel) {
    if (sel < 0 || sel >= (int)gs.size()) return all;
    std::vector<GridItem> out;
    for (const GridItem& it : all)
        if (groupHas(gs[sel], it.pkg)) out.push_back(it);
    return out;
}

std::string groupEncode(const LibGroup& g) {
    std::string out = g.name;
    out += '\t';
    for (size_t i = 0; i < g.pkgs.size(); ++i) {
        if (i) out += ',';
        out += g.pkgs[i];
    }
    return out;
}

bool groupDecode(const std::string& line, LibGroup* out) {
    const size_t tab = line.find('\t');
    if (tab == std::string::npos || tab == 0) return false;
    out->name = line.substr(0, tab);
    out->pkgs.clear();
    size_t at = tab + 1;
    while (at <= line.size()) {
        const size_t comma = line.find(',', at);
        const std::string pkg = line.substr(at,
            comma == std::string::npos ? comma : comma - at);
        at = comma == std::string::npos ? line.size() + 1 : comma + 1;
        if (!pkg.empty()) out->pkgs.push_back(pkg);
    }
    return true;
}

static float clampM(float v, float lo, float hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

// a popup's half height from its row count: the rows plus the edge pad
static float popHH(int rows) {
    return rows * kGridMenuRowH * 0.5f + kGridMenuPad;
}

// which row of a popup a card-local point is on: rows stack downward from
// the rect's top edge
static int popRowAt(float cx, float cy, int rows, float x, float y) {
    const float hw = kGridMenuHW, hh = popHH(rows);
    if (x < cx - hw || x > cx + hw || y < cy - hh || y > cy + hh)
        return -1;
    const int r = (int)((cy + hh - kGridMenuPad - y) / kGridMenuRowH);
    return r < rows ? r : -1;
}

void gridGroupPill(float labelHW, float* x, float* y,
                   float* hw, float* hh) {
    *hw = labelHW + kGridPillPadX;
    if (*hw < kGridPillHW0) *hw = kGridPillHW0;
    *hh = kGridPillHH;
    // right edge parks left of the close disc
    *x = gridCloseX() - kGridCloseR - 0.020f - *hw;
    *y = kGridHH - kGridHeadH * 0.5f;
}

int gridDropRows(int nGroups) { return nGroups + 2; }

int gridDropRowKind(int row) {
    if (row <= 0) return GD_NEW;
    if (row == 1) return GD_ALL;
    return GD_GROUP;
}

void gridDropRect(float pillX, int rows, float* cx, float* cy,
                  float* hw, float* hh) {
    *hw = kGridMenuHW;
    *hh = popHH(rows);
    const float top = kGridHH - kGridHeadH - kGridPopGap;
    *cy = top - *hh;
    *cx = clampM(pillX, -kGridHW + *hw + kGridMenuPad,
                 kGridHW - *hw - kGridMenuPad);
}

int gridDropRowAt(float cx, float cy, int rows, float x, float y) {
    return popRowAt(cx, cy, rows, x, y);
}

int gridMenuRows(int nGroups) { return nGroups + 2; }

int gridMenuRowKind(int row, int nGroups) {
    if (row <= 0) return GM_PIN;
    if (row <= nGroups) return GM_GROUP;
    return GM_NEW;
}

void gridMenuRect(float ax, float ay, int rows, float* cx, float* cy,
                  float* hw, float* hh) {
    *hw = kGridMenuHW;
    *hh = popHH(rows);
    // the anchor is the menu's top-left corner, slid right a touch so the
    // dots it opened from stay peeking out
    *cx = clampM(ax + kGridPopGap + *hw, -kGridHW + *hw + kGridMenuPad,
                 kGridHW - *hw - kGridMenuPad);
    *cy = clampM(ay - *hh, -kGridHH + *hh + kGridMenuPad,
                 kGridHH - *hh - kGridMenuPad);
}

int gridMenuRowAt(float cx, float cy, int rows, float x, float y) {
    return popRowAt(cx, cy, rows, x, y);
}

void gridDotsPos(const GridItem& it, float scroll, float* x, float* y) {
    *x = it.x + kGridDotsOff;
    *y = it.y + scroll + kGridDotsOff;
}

bool gridDotsHit(const GridItem& it, float scroll, float x, float y) {
    float dx, dy;
    gridDotsPos(it, scroll, &dx, &dy);
    const float ox = x - dx, oy = y - dy;
    return ox * ox + oy * oy <= kGridDotsR * kGridDotsR;
}

int gridZoneAt(const GridOverlay& g, float u, float v, int* idx) {
    *idx = -1;
    const float x = u * kGridHW, y = v * kGridHH;
    const int ng = (int)g.groups.size();
    // an open context menu wins hits on its own rows first
    if (!g.menuPkg.empty()) {
        float cx, cy, hw, hh;
        gridMenuRect(g.menuX, g.menuY, gridMenuRows(ng), &cx, &cy,
                     &hw, &hh);
        const int row = gridMenuRowAt(cx, cy, gridMenuRows(ng), x, y);
        if (row >= 0) { *idx = row; return GZONE_MENU; }
    }
    // then the open group dropdown's rows
    if (g.grpOpen) {
        float px, py, phw, phh;
        gridGroupPill(g.pillHW, &px, &py, &phw, &phh);
        const int rows = gridDropRows(ng);
        float cx, cy, hw, hh;
        gridDropRect(px, rows, &cx, &cy, &hw, &hh);
        const int row = gridDropRowAt(cx, cy, rows, x, y);
        if (row >= 0) { *idx = row; return GZONE_GROW; }
    }
    // the title band: the close disc, then the group pill, else body
    if (y > kGridHH - kGridHeadH) {
        const float dx = x - gridCloseX(),
                    dy = y - (kGridHH - kGridHeadH * 0.5f);
        const float rr = kGridCloseR + 0.012f;
        if (dx * dx + dy * dy <= rr * rr) return GZONE_CLOSE;
        float px, py, phw, phh;
        gridGroupPill(g.pillHW, &px, &py, &phw, &phh);
        if (x > px - phw && x < px + phw &&
                y > py - phh && y < py + phh)
            return GZONE_GROUPSEL;
        return GZONE_BODY;
    }
    const int i = gridItemAt((int)g.items.size(), u, v, g.scroll);
    if (i >= 0) {
        *idx = i;
        if (gridDotsHit(g.items[i], g.scroll, x, y)) return GZONE_DOTS;
        return GZONE_ITEM;
    }
    return GZONE_BODY;
}

GridPick pickGridRay(const GridOverlay& g, float yaw, float pitch,
                     const float origin[3], const float o[3],
                     const float d[3]) {
    GridPick pk;
    if (g.items.empty() && g.all.empty()) return pk;
    float c[3], r[3], up[3];
    gridCenter(yaw, pitch, origin, c, r, up);
    if (!rayQuad(c, r, up, origin, o, d, kGridHW, kGridHH,
                 &pk.u, &pk.v, &pk.t)) return pk;
    if (fabsf(pk.u) > 1.0f || fabsf(pk.v) > 1.0f) return pk;
    pk.hit = true;
    pk.zone = gridZoneAt(g, pk.u, pk.v, &pk.idx);
    return pk;
}
