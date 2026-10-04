#include "test.h"
#include "../src/hud/quiet.h"

void testQuiet() {
    // an untouched input is quiet once the grace period passes
    {
        HudQuietIn q;
        CHECK(!hudQuietBusy(q));
        CHECK(!hudQuiet(q, 500, 1000));
        CHECK(hudQuiet(q, 1000, 1000));
        CHECK(hudQuiet(q, 5000, 1000));
    }

    // every busy signal blocks the quiet path on its own
    {
        bool HudQuietIn::*fields[] = {
            &HudQuietIn::panels, &HudQuietIn::grid, &HudQuietIn::notifs,
            &HudQuietIn::sysmsgs, &HudQuietIn::toastOnly,
            &HudQuietIn::sysMsgOnly, &HudQuietIn::debugOnly,
            &HudQuietIn::kbd, &HudQuietIn::holdRing, &HudQuietIn::held,
            &HudQuietIn::presses, &HudQuietIn::inputPending,
            &HudQuietIn::aimMoved, &HudQuietIn::recentSummon,
        };
        for (auto f : fields) {
            HudQuietIn q;
            q.*f = true;
            CHECK(hudQuietBusy(q));
            CHECK(!hudQuiet(q, 60000, 1000));
        }
    }

    // busy first means the grace timer restarts: quiet only after grace
    {
        HudQuietIn q;
        CHECK(!hudQuiet(q, 999, 1000));
        CHECK(hudQuiet(q, 1001, 1000));
    }
}
