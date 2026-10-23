#include "grid.h"

#include "dashapps.h"
#include "layout.h"
#include "../anim/anim.h"
#include "../hud/engine.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/palette.h"
#include "../dock/dock.h"
#include "../dock/layout.h"
#include "../quick/item.h"
#include "../render/icons.h"
#include "../render/shape.h"
#include "../set/set.h"
#include "../text/draw.h"

#include <GLES2/gl2.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

// uiStrings fallback: the library block's English before the first pull
// lands - same strings res/values ships
static const char* kLibFallback[UILIB_COUNT] = {
    "All apps", "New group", "Pin to dock", "Remove from dock", "Group %d",
};

static const char* libStr(HudEngine* e, int i) {
    return e->uiLibStr[i].empty() ? kLibFallback[i] : e->uiLibStr[i].c_str();
}

// a dash entry's label rides the card's own uiStrings slot; the
// registry's baked name covers a pull that hasn't landed yet
static const char* dashLibLabel(HudEngine* e, int panel) {
    const std::string* s = nullptr;
    switch (panel) {
    case DP_SETTINGS: s = &e->uiSet[UISET_TITLE]; break;
    case DP_QUICK:    s = &e->uiQs[UIQS_TITLE];   break;
    }
    if (s && !s->empty()) return s->c_str();
    return dashLibApps()[panel].name;
}

// the pill label's text size, metres per font px like cardText's
static const float kPillTs = 0.0014f;

static const char* gridGroupLabel(HudEngine* e) {
    if (e->grid.groupSel >= 0 &&
            e->grid.groupSel < (int)e->grid.groups.size())
        return e->grid.groups[e->grid.groupSel].name.c_str();
    return libStr(e, UILIB_ALL);
}

// rebuild the visible cells from `all` through the group filter, keeping
// each surviving cell's hover scale
static void gridApplyFilter(HudEngine* e) {
    std::vector<GridItem> vis =
        gridVisible(e->grid.all, e->grid.groups, e->grid.groupSel);
    for (auto& it : vis)
        for (auto& o : e->grid.items)
            if (o.pkg == it.pkg) { it.hs = o.hs; break; }
    e->grid.items.swap(vis);
    gridLayout(e->grid.items);
    e->grid.scroll = gridClampScroll(e->grid.scroll,
                                     (int)e->grid.items.size());
}

static void gridPushGroups(HudEngine* e) {
    if (e->backend) e->backend->setGroups(e->grid.groups);
    gridApplyFilter(e);
}

void syncGrid(HudEngine* e) {
    if (!e->backend) return;
    // popups can't outlive the card they hang off
    if (!e->grid.shown) {
        e->grid.menuPkg.clear();
        e->grid.grpOpen = false;
    }
    if (e->backend->takeGroups(&e->grid.groups)) {
        if (e->grid.groupSel >= (int)e->grid.groups.size())
            e->grid.groupSel = -1;
        gridApplyFilter(e);
    }
    const int v = e->backend->appsVersion();
    if (v != e->grid.appsVer) {
        e->grid.appsVer = v;
        e->backend->launcherApps(&e->grid.all);
        // the dash's own cards lead the list - baked in, so they cost no
        // package pull and no icon rebind when the grid opens
        dashLibMerge(&e->grid.all);
        for (auto& it : e->grid.all)
            if (it.dashPanel < 0) it.vr = iconFor(e, it.pkg).vr;
        // a rebuild can pull the floor out from under an open menu
        if (!e->grid.menuPkg.empty()) {
            bool alive = false;
            for (auto& it : e->grid.all)
                if (it.pkg == e->grid.menuPkg) { alive = true; break; }
            if (!alive) e->grid.menuPkg.clear();
        }
        gridApplyFilter(e);
    }
    e->grid.pillHW = measureText(e, gridGroupLabel(e), kPillTs) * 0.5f;
}

void gridActivate(HudEngine* e, int idx) {
    if (idx < 0 || idx >= (int)e->grid.items.size()) return;
    const GridItem& it = e->grid.items[idx];
    switch (it.dashPanel) {
    case DP_SETTINGS:
        setOpen(e, SEC_WIFI);
        return;
    case DP_QUICK:
        // the status pill's own toggle; the grid being up means the
        // quick card is down, so this can only open it
        dockSysActivate(e);
        return;
    }
    queueLaunch(it.pkg.c_str());
    e->grid.shown = false;
}

void gridMenuOpen(HudEngine* e, int idx) {
    if (idx < 0 || idx >= (int)e->grid.items.size()) return;
    const GridItem& it = e->grid.items[idx];
    e->grid.menuPkg = it.pkg;
    e->grid.grpOpen = false;
    gridDotsPos(it, e->grid.scroll, &e->grid.menuX, &e->grid.menuY);
}

void gridMenuActivate(HudEngine* e, int row) {
    const std::string pkg = e->grid.menuPkg;
    const int ng = (int)e->grid.groups.size();
    e->grid.menuPkg.clear();
    switch (gridMenuRowKind(row, ng)) {
    case GM_PIN:
        if (!pkg.empty()) dockTogglePin(e, pkg.c_str());
        return;
    case GM_GROUP: {
        const int gi = row - 1;
        if (gi < ng && !pkg.empty()) {
            groupToggle(&e->grid.groups[gi], pkg);
            gridPushGroups(e);
        }
        return;
    }
    default: {   // GM_NEW: a fresh group holding the menu's app
        if (pkg.empty()) return;
        LibGroup g;
        g.name = groupNewName(e->grid.groups, libStr(e, UILIB_GROUPFMT));
        g.pkgs.push_back(pkg);
        e->grid.groups.push_back(g);
        gridPushGroups(e);
        return;
    }
    }
}

void gridDropActivate(HudEngine* e, int row) {
    e->grid.grpOpen = false;
    switch (gridDropRowKind(row)) {
    case GD_NEW: {
        LibGroup g;
        g.name = groupNewName(e->grid.groups, libStr(e, UILIB_GROUPFMT));
        e->grid.groups.push_back(g);
        e->grid.groupSel = (int)e->grid.groups.size() - 1;
        gridPushGroups(e);
        return;
    }
    case GD_ALL:
        e->grid.groupSel = -1;
        gridApplyFilter(e);
        return;
    default: {
        const int gi = row - 2;
        if (gi < (int)e->grid.groups.size()) {
            e->grid.groupSel = gi;
            gridApplyFilter(e);
        }
        return;
    }
    }
}

// ------------------------------------------------------------- draw

// text on the card's plane at a card-local point, centred or left-
// anchored - the same shader juggling the quick card's cardText does
static void gridText(HudEngine* e, const Mat4& vp, const float c[3],
                     const float r[3], const float up[3],
                     float x, float y, const char* s, float ts,
                     float alpha, bool centred, bool dim) {
    if (!e->font.ok || !s || !*s) return;
    float gt, gb, yo = y;
    if (textBounds(e->font.set, s, ts, &gt, &gb)) yo = y - (gt + gb) * 0.5f;
    float xo = x;
    if (centred) xo = x - measureText(e, s, ts) * 0.5f;
    float o[3] = {c[0] + r[0]*xo + up[0]*yo,
                  c[1] + r[1]*xo + up[1]*yo,
                  c[2] + r[2]*xo + up[2]*yo};
    o[0] -= c[0] * 0.010f; o[1] -= c[1] * 0.010f; o[2] -= c[2] * 0.010f;
    const float* tc = dim ? kPalTextDim : kPalText;
    glUseProgram(e->textProg);
    glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                       1, GL_FALSE, vp.m);
    glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                tc[0], tc[1], tc[2]);
    glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e->font.tex);
    drawTextPanel(e, s, o, r, up, ts, 0.0f, alpha);
    glUseProgram(e->shapeProg);
}

// one row of a popup: a label, an optional leading glyph and a check mark
// trailing on rows that represent a live state
struct PopRow {
    const char* label;
    int glyph = -1;     // GI_* drawn at the row's left, -1: none
    bool marked = false;    // trailing check glyph
};

// a frosted popup card and its rows, anchored on a card-local rect;
// `zone`/`hover` light the row under the aim
static void drawPopup(HudEngine* e, const Mat4& vp, const float c[3],
                      const float r[3], const float up[3],
                      float cx, float cy, float hw, float hh,
                      const std::vector<PopRow>& rows, int zone,
                      float gs, float ga) {
    const float pc[3] = {c[0] + r[0]*cx*gs + up[0]*cy*gs,
                         c[1] + r[1]*cx*gs + up[1]*cy*gs,
                         c[2] + r[2]*cx*gs + up[2]*cy*gs};
    const float shc[3] = {pc[0] - up[0]*0.016f*gs,
                          pc[1] - up[1]*0.016f*gs,
                          pc[2] - up[2]*0.016f*gs};
    const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.34f * ga};
    shapeQuad(e, vp, shc, r, up, 0.009f, 0.0f, hw*gs + 0.03f,
              hh*gs + 0.03f, hw*gs, hh*gs, 0.030f, -1.0f, 0.04f, shCol);
    const float bg[4] = {kPalPanel[0], kPalPanel[1], kPalPanel[2],
                         0.94f * ga};
    shapeQuad(e, vp, pc, r, up, 0.010f, 0.0f, hw*gs, hh*gs, hw*gs, hh*gs,
              0.020f, 0.0f, 0.002f, bg, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
              0.10f);
    const float rim[4] = {kPalText[0], kPalText[1], kPalText[2],
                          0.16f * ga};
    shapeQuad(e, vp, pc, r, up, 0.011f, 0.0f, hw*gs, hh*gs, hw*gs, hh*gs,
              0.020f, 0.0014f, 0.0010f, rim);

    const float top = cy + hh - kGridMenuPad;
    for (int i = 0; i < (int)rows.size(); ++i) {
        const float ry = top - (i + 0.5f) * kGridMenuRowH;
        if (e->grid.zone == zone && e->grid.hover == i) {
            const float rc[3] = {c[0] + r[0]*cx*gs + up[0]*ry*gs,
                                 c[1] + r[1]*cx*gs + up[1]*ry*gs,
                                 c[2] + r[2]*cx*gs + up[2]*ry*gs};
            const float hcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                                   0.10f * ga};
            const float rhw = (hw - kGridMenuPad * 0.5f) * gs;
            shapeQuad(e, vp, rc, r, up, 0.012f, 0.0f, rhw,
                      kGridMenuRowH * 0.42f * gs, rhw,
                      kGridMenuRowH * 0.42f * gs,
                      kGridMenuRowH * 0.30f * gs, 0.0f, 0.0015f, hcol);
        }
        float lx = cx - hw + kGridMenuPad + 0.006f;
        if (rows[i].glyph >= 0) {
            const float gc[3] = {c[0] + r[0]*(lx + 0.010f)*gs +
                                     up[0]*ry*gs,
                                 c[1] + r[1]*(lx + 0.010f)*gs +
                                     up[1]*ry*gs,
                                 c[2] + r[2]*(lx + 0.010f)*gs +
                                     up[2]*ry*gs};
            const float icol[4] = {kPalText[0], kPalText[1], kPalText[2],
                                   0.85f * ga};
            drawGlyph(e, vp, rows[i].glyph, gc, r, up, 0.009f * gs,
                      0.014f, icol);
            lx += 0.026f;
        }
        gridText(e, vp, c, r, up, lx * gs, ry * gs, rows[i].label,
                 0.0013f * gs, ga, false, false);
        if (rows[i].marked) {
            const float mx = cx + hw - kGridMenuPad - 0.016f;
            const float mc[3] = {c[0] + r[0]*mx*gs + up[0]*ry*gs,
                                 c[1] + r[1]*mx*gs + up[1]*ry*gs,
                                 c[2] + r[2]*mx*gs + up[2]*ry*gs};
            const float mcol[4] = {kPalAccent[0], kPalAccent[1],
                                   kPalAccent[2], 0.95f * ga};
            drawGlyph(e, vp, GI_CHECK, mc, r, up, 0.009f * gs, 0.014f,
                      mcol);
        }
    }
}

void drawGrid(HudEngine* e, const Mat4& vp) {
    // openT chases `shown`, so the card is still drawn while it closes
    if (e->grid.openT <= 0.0f) return;
    const float gt = easeOutCubic(e->grid.openT);
    const float gs = kGridScale0 + (1.0f - kGridScale0) * gt;
    const float ga = gt;
    float c[3], r[3], up[3];
    gridCenter(e->dockYaw, ringPitchFor(e->dockPitch), e->dashDist,
               e->ringPos,
               c, r, up);
    const float hw = kGridHW * gs, hh = kGridHH * gs;

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(e->shapeProg);

    // shadow + card: frosted glass, lit from its top edge
    const float shc[3] = {c[0] - up[0] * 0.024f * gs,
                          c[1] - up[1] * 0.024f * gs,
                          c[2] - up[2] * 0.024f * gs};
    const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.34f * ga};
    shapeQuad(e, vp, shc, r, up, -0.03f, 0.0f, hw + 0.06f, hh + 0.06f,
              hw, hh, 0.06f, -1.0f, 0.06f, shCol);
    const float cardCol[4] = {kPalPanel[0], kPalPanel[1], kPalPanel[2],
                              0.66f * ga};
    shapeQuad(e, vp, c, r, up, 0.004f, 0.0f, hw, hh, hw, hh, 0.06f,
              0.0f, 0.003f, cardCol, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
              0.14f);
    const float rimCol[4] = {kPalText[0], kPalText[1], kPalText[2],
                             0.14f * ga};
    shapeQuad(e, vp, c, r, up, 0.005f, 0.0f, hw, hh, hw, hh, 0.06f,
              0.0016f, 0.0012f, rimCol, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
              0.30f);

    // title band: label left, the group pill and the close disc on the
    // right end
    const float hy = hh - kGridHeadH * 0.5f * gs;
    {
        const bool bhov = e->grid.zone == GZONE_CLOSE;
        const float bx = gridCloseX() * gs;
        const float bc[3] = {c[0] + r[0]*bx + up[0]*hy,
                             c[1] + r[1]*bx + up[1]*hy,
                             c[2] + r[2]*bx + up[2]*hy};
        const float* bgp = bhov ? kPalDanger : kPalText;
        const float bg[4] = {bgp[0], bgp[1], bgp[2],
                             (bhov ? 0.32f : 0.13f) * ga};
        shapeQuad(e, vp, bc, r, up, 0.006f, 0.0f, kGridCloseR * gs,
                  kGridCloseR * gs, kGridCloseR * gs, kGridCloseR * gs,
                  kGridCloseR * gs, 0.0f, 0.002f, bg);
        const float icol[4] = {kPalText[0], kPalText[1], kPalText[2],
                               0.92f * ga};
        drawGlyph(e, vp, GI_CLOSE, bc, r, up, kGridCloseR * 0.52f * gs,
                  0.008f, icol);
    }
    // the group pill: the library's filter - a press opens the dropdown
    {
        float px, py, phw, phh;
        gridGroupPill(e->grid.pillHW, &px, &py, &phw, &phh);
        const bool phov = e->grid.zone == GZONE_GROUPSEL || e->grid.grpOpen;
        const float pc[3] = {c[0] + r[0]*px*gs + up[0]*py*gs,
                             c[1] + r[1]*px*gs + up[1]*py*gs,
                             c[2] + r[2]*px*gs + up[2]*py*gs};
        const float bg[4] = {kPalText[0], kPalText[1], kPalText[2],
                             (phov ? 0.24f : 0.11f) * ga};
        shapeQuad(e, vp, pc, r, up, 0.006f, 0.0f, phw * gs, phh * gs,
                  phw * gs, phh * gs, phh * gs, 0.0f, 0.002f, bg);
        const float cw = 0.020f * gs;
        gridText(e, vp, c, r, up, (px - cw * 0.5f) * gs, py * gs,
                 gridGroupLabel(e), kPillTs * gs, ga, true, false);
        const float cc[3] = {c[0] + r[0]*(px + phw - cw * 0.75f)*gs +
                                 up[0]*py*gs,
                             c[1] + r[1]*(px + phw - cw * 0.75f)*gs +
                                 up[1]*py*gs,
                             c[2] + r[2]*(px + phw - cw * 0.75f)*gs +
                                 up[2]*py*gs};
        const float icol[4] = {kPalText[0], kPalText[1], kPalText[2],
                               0.80f * ga};
        drawGlyph(e, vp, GI_EXPAND_MORE, cc, r, up, 0.010f * gs, 0.008f,
                  icol);
    }
    if (e->font.ok) {
        const float ts = 0.0016f * gs;
        const char* title =
            e->uiLib.empty() ? "Library" : e->uiLib.c_str();
        float gt, gb, yo = hy;
        if (textBounds(e->font.set, title, ts, &gt, &gb))
            yo = hy - (gt + gb) * 0.5f;
        float to[3] = {c[0] + r[0]*(-hw + kGridSidePad * gs) + up[0]*yo,
                       c[1] + r[1]*(-hw + kGridSidePad * gs) + up[1]*yo,
                       c[2] + r[2]*(-hw + kGridSidePad * gs) + up[2]*yo};
        to[0] -= c[0] * 0.010f; to[1] -= c[1] * 0.010f;
        to[2] -= c[2] * 0.010f;
        glUseProgram(e->textProg);
        glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                           1, GL_FALSE, vp.m);
        glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                    kPalText[0], kPalText[1], kPalText[2]);
        glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, e->font.tex);
        drawTextPanel(e, title, to, r, up, ts, 0.0f, ga);
        glUseProgram(e->shapeProg);
    }

    // cells: clip to under the title band - the icon shader fades out at
    // the band edges while a row scrolls through
    float blo, bhi;
    gridClipBand(&blo, &bhi);
    blo *= gs; bhi *= gs;
    const float clipC = (blo + bhi) * 0.5f, clipH = (bhi - blo) * 0.5f;
    for (int i = 0; i < (int)e->grid.items.size(); ++i) {
        const GridItem& it = e->grid.items[i];
        const float iy = (it.y + e->grid.scroll) * gs;
        if (!gridCellShown(iy, gs)) continue;
        const float hp = hoverP(it.hs, kGridHoverScale);
        const float s = kGridIconHW * gs * it.hs;
        const float ic[3] = {c[0] + r[0]*it.x*gs + up[0]*iy,
                             c[1] + r[1]*it.x*gs + up[1]*iy,
                             c[2] + r[2]*it.x*gs + up[2]*iy};
        const DockIcon* icon = nullptr;
        if (it.dashPanel < 0) {
            auto f = e->dockIcons.find(it.pkg);
            if (f != e->dockIcons.end()) icon = &f->second;
        }

        if (hp > 0.02f) {
            const float hl[4] = {kPalText[0], kPalText[1], kPalText[2],
                                 0.10f * hp * ga};
            // hugs the icon: any bigger and the lift clips into the
            // label drawn under the cell
            const float hx = s + 0.008f;
            shapeQuad(e, vp, ic, r, up, 0.006f, 0.0f, hx, hx, hx, hx,
                      hx * kIconRad, 0.0f, 0.002f, hl,
                      -1.0f, 0.0f, iy, clipC, clipH);
        }
        if (it.dashPanel >= 0) {
            // baked tile: a glass square carrying the panel's atlas
            // glyph - no texture bind per cell
            const float tc[4] = {kPalText[0], kPalText[1], kPalText[2],
                                 (0.12f + 0.06f * hp) * ga};
            shapeQuad(e, vp, ic, r, up, 0.008f, 0.0f, s, s, s, s,
                      s * kIconRad, 0.0f, 0.002f, tc,
                      -1.0f, 0.0f, iy, clipC, clipH);
            // the glyph rides the text program, which has no clip hook:
            // like the letter tile it only draws while it fits the band
            if (iy + s <= clipC + clipH && iy - s >= clipC - clipH) {
                const float gcol[4] = {kPalText[0], kPalText[1],
                                       kPalText[2], 0.92f * ga};
                drawGlyph(e, vp, dashLibApps()[it.dashPanel].glyph,
                          ic, r, up, s * 0.58f, 0.014f, gcol);
            }
        } else if (icon && icon->tex) {
            drawIconTex(e, vp, ic, r, up, s, icon->tex, ga,
                        iy, clipC, clipH);
        } else {
            glUseProgram(e->shapeProg);
            drawLetterTile(e, vp, ic, r, up, s,
                           it.label.empty() ? it.pkg.c_str()
                                            : it.label.c_str(),
                           iy, clipC, clipH, ga);
            glUseProgram(e->shapeProg);
        }
        if (it.vr) {
            const float vc[4] = {kPalWarn[0], kPalWarn[1], kPalWarn[2],
                                 (0.65f + 0.30f * hp) * ga};
            shapeQuad(e, vp, ic, r, up, 0.009f, 0.0f, s + 0.006f,
                      s + 0.006f, s + 0.006f, s + 0.006f, s + 0.006f,
                      0.0018f, 0.0015f, vc, -1.0f, 0.0f, iy, clipC, clipH);
        }
        // the cell's menu affordance: a small dots glyph off the icon's
        // top-right, lit while the aim is on it - dash cells get none,
        // they can't pin or group
        if (it.dashPanel < 0) {
            float dx, dy;
            gridDotsPos(it, e->grid.scroll, &dx, &dy);
            dy *= gs; dx *= gs;
            if (dy + 0.020f < bhi && dy - 0.020f > blo) {
                const bool dhov = e->grid.zone == GZONE_DOTS &&
                                  e->grid.hover == i;
                const float dc[3] = {c[0] + r[0]*dx + up[0]*dy,
                                     c[1] + r[1]*dx + up[1]*dy,
                                     c[2] + r[2]*dx + up[2]*dy};
                const float icol[4] = {kPalText[0], kPalText[1],
                                       kPalText[2],
                                       (dhov ? 0.95f : 0.42f) * ga};
                drawGlyph(e, vp, GI_MORE_VERT, dc, r, up,
                          0.011f * gs, 0.008f, icol);
            }
        }
        // the label line sits under the icon; it only draws while it fits
        // the visible band whole, the icon fade carries the edge cases
        const float ly = iy - (kGridIconHW + 0.038f) * gs;
        const char* lab = it.dashPanel >= 0
            ? dashLibLabel(e, it.dashPanel) : it.label.c_str();
        if (*lab && e->font.ok &&
                ly + 0.018f < bhi && ly - 0.018f > blo) {
            const float ts = 0.0011f;
            const float lw = measureText(e, lab, ts) * 0.5f;
            const float maxW = (kGridHW * 2.0f - kGridSidePad * 2.0f) /
                               kGridCols * 0.48f;
            const float lw2 = lw > maxW ? maxW : lw;
            float lo3[3] = {c[0] + r[0]*(it.x*gs - lw2) + up[0]*ly,
                            c[1] + r[1]*(it.x*gs - lw2) + up[1]*ly,
                            c[2] + r[2]*(it.x*gs - lw2) + up[2]*ly};
            lo3[0] -= c[0] * 0.010f; lo3[1] -= c[1] * 0.010f;
            lo3[2] -= c[2] * 0.010f;
            glUseProgram(e->textProg);
            glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                               1, GL_FALSE, vp.m);
            glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                        kPalText[0], kPalText[1], kPalText[2]);
            glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, e->font.tex);
            drawTextPanel(e, lab, lo3, r, up,
                          lw > maxW ? ts * maxW / lw : ts, 0.0f, ga);
            glUseProgram(e->shapeProg);
        }
    }

    // popups layer over the cells: the group dropdown hangs off the title
    // pill, the context menu off its cell's dots
    if (e->grid.grpOpen) {
        float px, py, phw, phh;
        gridGroupPill(e->grid.pillHW, &px, &py, &phw, &phh);
        const int ng = (int)e->grid.groups.size();
        const int rows = gridDropRows(ng);
        float cx, cy, dhw, dhh;
        gridDropRect(px, rows, &cx, &cy, &dhw, &dhh);
        std::vector<PopRow> pr(rows);
        pr[0].label = libStr(e, UILIB_NEW);
        pr[0].glyph = GI_ADD;
        pr[1].label = libStr(e, UILIB_ALL);
        pr[1].marked = e->grid.groupSel < 0;
        for (int i = 0; i < ng; ++i) {
            pr[i + 2].label = e->grid.groups[i].name.c_str();
            pr[i + 2].marked = e->grid.groupSel == i;
        }
        drawPopup(e, vp, c, r, up, cx, cy, dhw, dhh, pr, GZONE_GROW,
                  gs, ga);
    }
    if (!e->grid.menuPkg.empty()) {
        const int ng = (int)e->grid.groups.size();
        const int rows = gridMenuRows(ng);
        float cx, cy, mhw, mhh;
        gridMenuRect(e->grid.menuX, e->grid.menuY, rows,
                     &cx, &cy, &mhw, &mhh);
        std::vector<PopRow> pr(rows);
        const bool pinned =
            std::find(e->dockPins.begin(), e->dockPins.end(),
                      e->grid.menuPkg) != e->dockPins.end();
        pr[0].label = libStr(e, pinned ? UILIB_UNPIN : UILIB_PIN);
        for (int i = 0; i < ng; ++i) {
            pr[i + 1].label = e->grid.groups[i].name.c_str();
            pr[i + 1].marked =
                groupHas(e->grid.groups[i], e->grid.menuPkg);
        }
        pr[rows - 1].label = libStr(e, UILIB_NEW);
        pr[rows - 1].glyph = GI_ADD;
        drawPopup(e, vp, c, r, up, cx, cy, mhw, mhh, pr, GZONE_MENU,
                  gs, ga);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
