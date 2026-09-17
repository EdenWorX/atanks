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

#include "zbuffer.h"

TEST_GROUP(ZBuffer){};

TEST(ZBuffer, FreshBufferReadsFalse) {
    ZBuffer zb(8, 8);
    CHECK_FALSE(zb.test(0, 0));
    CHECK_FALSE(zb.test(3, 5));
    CHECK_FALSE(zb.test(7, 7));
}

TEST(ZBuffer, SetAndTest) {
    ZBuffer zb(8, 8);
    zb.set(3, 5);
    CHECK_TRUE(zb.test(3, 5));
    CHECK_FALSE(zb.test(3, 4));
    CHECK_FALSE(zb.test(4, 5));
    CHECK_FALSE(zb.test(0, 0));
}

TEST(ZBuffer, Corners) {
    ZBuffer zb(8, 8);
    zb.set(0, 0);
    zb.set(7, 7);
    CHECK_TRUE(zb.test(0, 0));
    CHECK_TRUE(zb.test(7, 7));
    CHECK_FALSE(zb.test(0, 7));
    CHECK_FALSE(zb.test(7, 0));
}

TEST(ZBuffer, OutOfRangeDoesNotCrash) {
    ZBuffer zb(8, 8);
    zb.set(-1, 0);
    zb.set(8, 0);
    zb.set(0, 8);
    zb.set(100, 100);
    CHECK_FALSE(zb.test(-1, 0));
    CHECK_FALSE(zb.test(8, 8));
    CHECK_FALSE(zb.test(0, 0));
}

TEST(ZBuffer, NonPowerOfTwoSize) {
    ZBuffer zb(10, 6);
    for (int32_t y = 0; y < 6; ++y) {
        for (int32_t x = 0; x < 10; ++x) {
            zb.set(x, y);
        }
    }
    for (int32_t y = 0; y < 6; ++y) {
        for (int32_t x = 0; x < 10; ++x) {
            CHECK_TRUE(zb.test(x, y));
        }
    }
}

TEST(ZBuffer, InstancesAreIndependent) {
    ZBuffer first(8, 8);
    ZBuffer second(8, 8);
    first.set(2, 2);
    CHECK_TRUE(first.test(2, 2));
    CHECK_FALSE(second.test(2, 2));
}
