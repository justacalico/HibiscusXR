#pragma once

#include "../dock/item.h"
#include "../grid/item.h"
#include "../notif/item.h"
#include "../quick/item.h"
#include "../sysmsg/item.h"

// which layer of the dash the aim ray selects. The dash draws as an
// overlay on top of the windows, so a hit on any dash element beats a
// window no matter how near the window got - what you see is what you
// click
enum DashSel {
    DSEL_PANEL = 0,   // nothing of the dash hit: the window pick stands
    DSEL_SYSMSG,
    DSEL_NOTIF,
    DSEL_QUICK,
    DSEL_GRID,
    DSEL_SHELF,
    DSEL_DOCK,
    DSEL_KBD,
};

// arbitration across one aim ray's picks: a modal system card first, then
// the dash chrome ordered among itself by real ray distance (t). kbdT is
// the ray distance to the keyboard quad, negative when it missed
DashSel dashSel(bool sysModal, const SysMsgPick& mp, const NotifPick& np,
                const GridPick& gp, const QsPick& qp, const ShelfPick& sp,
                const DockPick& dp, float kbdT);
