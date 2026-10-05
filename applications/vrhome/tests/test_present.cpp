#include "test.h"
#include "../src/render/present.h"

void testPresent() {
    // first valid stamp counts as a present
    {
        long long last = -1;
        int n = 0;
        CHECK(presentTick(&last, &n, 13900000));
        CHECK(n == 1);
        CHECK(last == 13900000);
    }

    // the same stamp arriving again is not a new flip
    {
        long long last = -1;
        int n = 0;
        presentTick(&last, &n, 13900000);
        CHECK(!presentTick(&last, &n, 13900000));
        CHECK(n == 1);
    }

    // a newer stamp counts, and the cursor advances
    {
        long long last = -1;
        int n = 0;
        presentTick(&last, &n, 13900000);
        CHECK(presentTick(&last, &n, 27800000));
        CHECK(n == 2);
        CHECK(last == 27800000);
    }

    // non-positive and stale stamps never count
    {
        long long last = -1;
        int n = 0;
        CHECK(!presentTick(&last, &n, 0));
        CHECK(!presentTick(&last, &n, -50));
        presentTick(&last, &n, 1000);
        CHECK(!presentTick(&last, &n, 999));
        CHECK(!presentTick(&last, &n, 0));
        CHECK(n == 1);
        CHECK(last == 1000);
    }

    // a burst of submits inside one present only counts once: submit 30,
    // present 2 -> monado fps reports 2, not 30
    {
        long long last = -1;
        int n = 0;
        for (int i = 0; i < 30; ++i) presentTick(&last, &n, 13900000);
        for (int i = 0; i < 30; ++i) presentTick(&last, &n, 27800000);
        CHECK(n == 2);
    }
}
