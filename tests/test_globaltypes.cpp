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

#include "globaltypes.h"

// The enum helpers below drive savegame parsing and language menus, so
// these tests pin their rotation and wraparound semantics.

TEST_GROUP(GlobalTypes){};

TEST(GlobalTypes, DataStageFullRotation) {
    eDataStage stage = DS_NAME;
    ++stage;
    CHECK(stage == DS_DESC);
    ++stage;
    CHECK(stage == DS_DATA);
    ++stage;
    CHECK(stage == DS_NAME);
}

TEST(GlobalTypes, LanguageLayout) {
    LONGS_EQUAL(0, EL_ENGLISH);
    LONGS_EQUAL(8, EL_LANGUAGE_COUNT);
    LONGS_EQUAL(7, EL_ITALIAN);
}

TEST(GlobalTypes, IncrementWrapsAround) {
    eLanguages lang = EL_ITALIAN;
    ++lang;
    LONGS_EQUAL(EL_ENGLISH, lang);
}

TEST(GlobalTypes, DecrementWrapsAround) {
    eLanguages lang = EL_ENGLISH;
    --lang;
    LONGS_EQUAL(EL_ITALIAN, lang);
}

TEST(GlobalTypes, PostIncrementReturnsOldValue) {
    eLanguages lang = EL_ENGLISH;
    eLanguages old  = lang++;
    LONGS_EQUAL(EL_ENGLISH, old);
    LONGS_EQUAL(EL_PORTUGUESE, lang);
}

TEST(GlobalTypes, AddAssignComputesModulo) {
    eLanguages lang = EL_ENGLISH;
    lang += 8;
    LONGS_EQUAL(EL_ENGLISH, lang);
    lang += 10;
    LONGS_EQUAL(EL_FRENCH, lang);
}

TEST(GlobalTypes, ControlCodes) {
    LONGS_EQUAL(0, CONTROL_NONE);
    LONGS_EQUAL(101, CONTROL_FIRE);
    LONGS_EQUAL(202, CONTROL_QUIT);
}
