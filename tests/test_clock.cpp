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

#include "clock.h"

// check_time_changed() is deliberately untested: it reports real second
// boundaries through thread-local state, so no deterministic expectation
// exists for it. Everything else here round-trips through reset/get.

TEST_GROUP(Clock){};

TEST(Clock, GameUsecRoundtrip) {
    game_us_reset();
    CHECK(game_us_get() >= 0);
}

TEST(Clock, MenuMsRoundtrip) {
    menu_ms_reset();
    CHECK(menu_ms_get() >= 0);
}

TEST(Clock, RepeatedReadsStayNonNegative) {
    game_us_reset();
    menu_ms_reset();
    for (int i = 0; i < 10; ++i) {
        CHECK(game_us_get() >= 0);
        CHECK(menu_ms_get() >= 0);
    }
}
