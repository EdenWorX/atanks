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

#include "optiontypes.h"

// The menu enums double as stable indexes (savegames, option lists), so
// these tests pin their layout: first values, counts, and special codes.

TEST_GROUP(OptionTypes){};

TEST(OptionTypes, MenuClassLayout) {
    LONGS_EQUAL(0, MC_AREYOUSURE);
    LONGS_EQUAL(3, MC_MAIN);
    LONGS_EQUAL(12, MC_MENUCLASS_COUNT);
}

TEST(OptionTypes, TextClassLayout) {
    LONGS_EQUAL(0, TC_COLOUR);
    LONGS_EQUAL(18, TC_TEXTCLASS_COUNT);
    LONGS_EQUAL(19, TC_FREETEXT);
    LONGS_EQUAL(20, TC_NONE);
}

TEST(OptionTypes, EntryTypeLayout) {
    LONGS_EQUAL(0, ET_NONE);
    LONGS_EQUAL(8, ET_VALUE);
}

TEST(OptionTypes, ResetCodes) {
    LONGS_EQUAL(667, RO_BACK);
    LONGS_EQUAL(1337, RO_RESET);
}
