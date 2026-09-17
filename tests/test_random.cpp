/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2026  EdenWorX
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 */

#include "CppUTest/TestHarness.h"

#include "random.h"

TEST_GROUP(Random){};

TEST(Random, GetRandIsNonNegative) {
    for (int i = 0; i < 1000; ++i) {
        CHECK(get_rand() >= 0);
    }
}

TEST(Random, GetRandVaries) {
    int32_t const first = get_rand();
    bool          varied = false;
    for (int i = 0; i < 100; ++i) {
        if (get_rand() != first) {
            varied = true;
            break;
        }
    }
    CHECK_TRUE(varied);
}

TEST(Random, CentralRandStaysInRange) {
    double const bounds[] = { 1.0, 100.0 };
    for (double u : bounds) {
        for (int i = 0; i < 500; ++i) {
            double const r = central_rand(u);
            CHECK(r >= 0.0);
            CHECK(r <= u);
        }
    }
}

TEST(Random, CentralRandIsNotConstant) {
    double mn = 1.0;
    double mx = 0.0;
    for (int i = 0; i < 200; ++i) {
        double const r = central_rand(1.0);
        if (r < mn) {
            mn = r;
        }
        if (r > mx) {
            mx = r;
        }
    }
    CHECK(mx > mn);
}

TEST(Random, AiMacrosStayInRange) {
    int ai_level = 7;
    for (int i = 0; i < 200; ++i) {
        int const v = RAND_AI_0P;
        CHECK(v >= 0);
        CHECK(v < ai_level);
    }
}
