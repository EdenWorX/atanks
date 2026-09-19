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

#include "box.h"

TEST_GROUP(Box){};

TEST(Box, DefaultIsZero) {
    sBox box;
    LONGS_EQUAL(0, box.x);
    LONGS_EQUAL(0, box.y);
    LONGS_EQUAL(0, box.w);
    LONGS_EQUAL(0, box.h);
}

TEST(Box, ConstructorAndSet) {
    sBox box(1, 2, 3, 4);
    LONGS_EQUAL(1, box.x);
    LONGS_EQUAL(2, box.y);
    LONGS_EQUAL(3, box.w);
    LONGS_EQUAL(4, box.h);
    box.set(5, 6, 7, 8);
    LONGS_EQUAL(5, box.x);
    LONGS_EQUAL(6, box.y);
    LONGS_EQUAL(7, box.w);
    LONGS_EQUAL(8, box.h);
}

TEST(Box, Equality) {
    sBox first(1, 2, 3, 4);
    sBox second(1, 2, 3, 4);
    CHECK(first == second);
    CHECK_FALSE(first != second);
    second.w = 9;
    CHECK(first != second);
    CHECK_FALSE(first == second);
}

TEST(Box, SelfEquality) {
    sBox box(1, 2, 3, 4);
    CHECK(box == box);
}
