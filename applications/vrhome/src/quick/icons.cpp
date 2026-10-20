#include "icons.h"

#include "item.h"
#include "../hud/engine.h"
#include "../render/icons.h"

// the QsId -> Material Symbols map; the atlas cells are drawn by
// drawGlyph, this file only picks which glyph each tile shows

void quickIcon(HudEngine* e, const Mat4& vp, int id, const float ic[3],
               const float r[3], const float up[3], float s,
               const float col[4]) {
    int g = -1;
    switch (id) {
    case QS_WIFI:        g = GI_WIFI; break;
    case QS_ICON_ETH:    g = GI_LAN; break;
    case QS_BOUNDARY:    g = GI_FENCE; break;
    case QS_BLUETOOTH:   g = GI_BLUETOOTH; break;
    case QS_SEETHROUGH:  g = GI_VISIBILITY; break;
    case QS_MIC:         g = GI_MIC; break;
    case QS_RESET_VIEW:  g = GI_RESTART_ALT; break;
    case QS_NIGHT:       g = GI_BEDTIME; break;
    case QS_DND:         g = GI_DO_NOT_DISTURB_ON; break;
    case QS_AIRPLANE:    g = GI_FLIGHT; break;
    case QS_SAVER:       g = GI_BATTERY_SAVER; break;
    case QS_REPORT:      g = GI_REPORT; break;
    case QS_ABOUT:       g = GI_INFO; break;
    }
    if (g >= 0) drawGlyph(e, vp, g, ic, r, up, s, 0.008f, col);
}

void quickSliderIcon(HudEngine* e, const Mat4& vp, int slider,
                     const float ic[3], const float r[3],
                     const float up[3], float s, const float col[4]) {
    drawGlyph(e, vp, slider == QSLIDER_BRIGHTNESS ? GI_SUNNY : GI_VOLUME_UP,
              ic, r, up, s, 0.008f, col);
}

void quickGearIcon(HudEngine* e, const Mat4& vp, const float ic[3],
                   const float r[3], const float up[3], float s,
                   const float col[4]) {
    drawGlyph(e, vp, GI_SETTINGS, ic, r, up, s, 0.008f, col);
}
