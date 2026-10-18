#pragma once

// xdg_positioner anchor/gravity math, kept protocol-header-free so the
// host tests compile it. The numeric values mirror the protocol enums:
// anchor/gravity 0 = none, 1..9 top/bottom/left/right + diagonals,
// center.
struct PopupRect { int x = 0, y = 0, w = 0, h = 0; };

void popupPos(const PopupRect& anchor, int w, int h,
              int anchorEdge, int gravity, int dx, int dy,
              int* ox, int* oy);
