#include "test.h"

#include "hud/status.h"

void testStatus() {
    // prop unset: the settings toggle owns the line
    CHECK(statusLineVisible(-1, false) == false);
    CHECK(statusLineVisible(-1, true) == true);

    // prop set: it wins over the toggle both ways
    CHECK(statusLineVisible(0, true) == false);
    CHECK(statusLineVisible(1, false) == true);
    CHECK(statusLineVisible(1, true) == true);
    CHECK(statusLineVisible(0, false) == false);
}
