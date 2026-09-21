#pragma once

#include <cstdio>
#include <cmath>

// tiny harness: CHECK records failures, main prints the tally
extern int gChecks, gFails;

#define CHECK(cond) do { ++gChecks; if (!(cond)) { ++gFails; \
    printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

#define CHECK_F(a, b, eps) do { ++gChecks; \
    const float _d = fabsf((a) - (b)); \
    if (!(_d <= (eps))) { ++gFails; \
        printf("FAIL %s:%d  |%s - %s| = %f > %f  (%f vs %f)\n", \
               __FILE__, __LINE__, #a, #b, _d, (float)(eps), \
               (float)(a), (float)(b)); } } while (0)
