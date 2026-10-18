#include "popuppos.h"

enum { E_NONE = 0, E_TOP, E_BOTTOM, E_LEFT, E_RIGHT,
       E_TOP_LEFT, E_TOP_RIGHT, E_BOTTOM_LEFT, E_BOTTOM_RIGHT,
       E_CENTER };

// the anchor point on the anchor rect, in parent surface coords
static void anchorPoint(const PopupRect& r, int edge, int* ax, int* ay) {
    switch (edge) {
    case E_TOP:          *ax = r.x + r.w / 2; *ay = r.y; break;
    case E_BOTTOM:       *ax = r.x + r.w / 2; *ay = r.y + r.h; break;
    case E_LEFT:         *ax = r.x; *ay = r.y + r.h / 2; break;
    case E_RIGHT:        *ax = r.x + r.w; *ay = r.y + r.h / 2; break;
    case E_TOP_LEFT:     *ax = r.x; *ay = r.y; break;
    case E_TOP_RIGHT:    *ax = r.x + r.w; *ay = r.y; break;
    case E_BOTTOM_LEFT:  *ax = r.x; *ay = r.y + r.h; break;
    case E_BOTTOM_RIGHT: *ax = r.x + r.w; *ay = r.y + r.h; break;
    case E_CENTER:       *ax = r.x + r.w / 2; *ay = r.y + r.h / 2; break;
    default:             *ax = r.x; *ay = r.y; break;
    }
}

void popupPos(const PopupRect& anchor, int w, int h,
              int anchorEdge, int gravity, int dx, int dy,
              int* ox, int* oy) {
    int ax, ay;
    anchorPoint(anchor, anchorEdge, &ax, &ay);
    int x = ax, y = ay;
    switch (gravity) {
    case E_TOP:          x = ax - w / 2; y = ay - h; break;
    case E_BOTTOM:       x = ax - w / 2; y = ay; break;
    case E_LEFT:         x = ax - w; y = ay - h / 2; break;
    case E_RIGHT:        x = ax; y = ay - h / 2; break;
    case E_TOP_LEFT:     x = ax - w; y = ay - h; break;
    case E_TOP_RIGHT:    x = ax; y = ay - h; break;
    case E_BOTTOM_LEFT:  x = ax - w; y = ay; break;
    case E_BOTTOM_RIGHT: x = ax; y = ay; break;
    case E_CENTER:       x = ax - w / 2; y = ay - h / 2; break;
    default:             x = ax; y = ay; break;
    }
    *ox = x + dx;
    *oy = y + dy;
}
