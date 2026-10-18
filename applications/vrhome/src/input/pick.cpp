#include "pick.h"

DashSel dashSel(bool sysModal, const SysMsgPick& mp, const NotifPick& np,
                const GridPick& gp, const ShelfPick& sp, const DockPick& dp,
                float kbdT) {
    if (sysModal || mp.hit) return DSEL_SYSMSG;
    if (np.stack && (!dp.bar || np.t <= dp.t) &&
        (kbdT < 0.0f || np.t <= kbdT)) return DSEL_NOTIF;
    if (gp.hit && (!dp.bar || gp.t <= dp.t) &&
        (!sp.hit || gp.t <= sp.t) &&
        (kbdT < 0.0f || gp.t <= kbdT)) return DSEL_GRID;
    if (sp.hit && (kbdT < 0.0f || sp.t <= kbdT)) return DSEL_SHELF;
    if (dp.bar && (kbdT < 0.0f || dp.t <= kbdT)) return DSEL_DOCK;
    if (kbdT >= 0.0f) return DSEL_KBD;
    return DSEL_PANEL;
}
