#include "test.h"

#include "input/pick.h"

static const SysMsgPick noMsg{};
static const NotifPick noNotif{};
static const GridPick noGrid{};
static const QsPick noQuick{};
static const ShelfPick noShelf{};
static const DockPick noDock{};

void testPick() {
    // nothing on the ray: the window pick stands
    CHECK(dashSel(false, noMsg, noNotif, noGrid, noQuick, noShelf, noDock, -1.0f) ==
          DSEL_PANEL);

    // dock hit: wins with no window distance consulted - the dash overlay
    // draws over a window swung in front of it, so the strip must also
    // take the click
    {
        DockPick dp;
        dp.bar = true;
        dp.idx = 2;
        dp.t = 1.35f;
        CHECK(dashSel(false, noMsg, noNotif, noGrid, noQuick, noShelf, dp, -1.0f) ==
              DSEL_DOCK);
    }

    // the shelf pill hangs above the strip and outranks it whenever both
    // are hit; the grid card beats the dock only when it is actually nearer
    {
        DockPick dp;
        dp.bar = true;
        dp.t = 1.35f;
        ShelfPick sp;
        sp.hit = true;
        sp.t = 1.20f;
        CHECK(dashSel(false, noMsg, noNotif, noGrid, noQuick, sp, dp, -1.0f) ==
              DSEL_SHELF);
        sp.t = 1.50f;
        CHECK(dashSel(false, noMsg, noNotif, noGrid, noQuick, sp, dp, -1.0f) ==
              DSEL_SHELF);
        GridPick gp;
        gp.hit = true;
        gp.t = 1.30f;
        CHECK(dashSel(false, noMsg, noNotif, gp, noQuick, noShelf, dp, -1.0f) ==
              DSEL_GRID);
        gp.t = 1.60f;
        CHECK(dashSel(false, noMsg, noNotif, gp, noQuick, noShelf, dp, -1.0f) ==
              DSEL_DOCK);
    }

    // the grid also yields to a nearer shelf pill
    {
        GridPick gp;
        gp.hit = true;
        gp.t = 1.38f;
        ShelfPick sp;
        sp.hit = true;
        sp.t = 1.30f;
        CHECK(dashSel(false, noMsg, noNotif, gp, noQuick, sp, noDock, -1.0f) ==
              DSEL_SHELF);
    }

    // notification stack: above the dock unless the strip is nearer, and
    // outranks the grid card like the draw order does
    {
        NotifPick np;
        np.stack = true;
        np.t = 1.34f;
        DockPick dp;
        dp.bar = true;
        dp.t = 1.35f;
        CHECK(dashSel(false, noMsg, np, noGrid, noQuick, noShelf, dp, -1.0f) ==
              DSEL_NOTIF);
        np.t = 1.50f;
        CHECK(dashSel(false, noMsg, np, noGrid, noQuick, noShelf, dp, -1.0f) ==
              DSEL_DOCK);
        np.t = 1.34f;
        GridPick gp;
        gp.hit = true;
        gp.t = 1.10f;
        CHECK(dashSel(false, noMsg, np, gp, noQuick, noShelf, noDock, -1.0f) ==
              DSEL_NOTIF);
    }

    // the keyboard rides nearest: it beats every dash element it overlaps
    {
        DockPick dp;
        dp.bar = true;
        dp.t = 1.35f;
        CHECK(dashSel(false, noMsg, noNotif, noGrid, noQuick, noShelf, dp, 1.15f) ==
              DSEL_KBD);
        CHECK(dashSel(false, noMsg, noNotif, noGrid, noQuick, noShelf, noDock,
                      1.15f) == DSEL_KBD);
        // but a dash element in front of it still wins on real depth
        CHECK(dashSel(false, noMsg, noNotif, noGrid, noQuick, noShelf, dp, 1.50f) ==
              DSEL_DOCK);
        NotifPick np;
        np.stack = true;
        np.t = 1.10f;
        CHECK(dashSel(false, noMsg, np, noGrid, noQuick, noShelf, noDock, 1.15f) ==
              DSEL_NOTIF);
    }

    // a modal system card outranks everything, hit or not
    {
        DockPick dp;
        dp.bar = true;
        dp.t = 1.35f;
        SysMsgPick mp;
        mp.hit = true;
        mp.t = 2.0f;
        CHECK(dashSel(false, mp, noNotif, noGrid, noQuick, noShelf, dp, 1.0f) ==
              DSEL_SYSMSG);
        CHECK(dashSel(true, noMsg, noNotif, noGrid, noQuick, noShelf, dp, 1.0f) ==
              DSEL_SYSMSG);
    }
}
