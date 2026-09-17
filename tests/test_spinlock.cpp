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

#include <thread>
#include <vector>

#include "spinlock.h"

// Note: under USE_MUTEX_INSTEAD_OF_SPINLOCK (thread-sanitizer builds)
// CSpinLock is aliased to std::mutex via globaldata.h, which has neither
// hasLock() nor the spinlock contract. These tests therefore cover the real
// CSpinLock only; release and debug builds always take this path.
#ifndef USE_MUTEX_INSTEAD_OF_SPINLOCK

TEST_GROUP(SpinLock){};

TEST(SpinLock, FreshLockIsFree) {
    CSpinLock guard;
    CHECK_FALSE(guard.hasLock());
}

TEST(SpinLock, LockUnlockCycle) {
    CSpinLock guard;
    guard.lock();
    CHECK_TRUE(guard.hasLock());
    guard.unlock();
    CHECK_FALSE(guard.hasLock());
}

TEST(SpinLock, ContendedCounter) {
    CSpinLock guard;
    int       counter = 0;
    auto      work    = [&]() {
        for (int i = 0; i < 2500; ++i) {
            guard.lock();
            ++counter;
            guard.unlock();
        }
    };
    std::vector< std::thread > threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(work);
    }
    for (auto& t : threads) {
        t.join();
    }
    LONGS_EQUAL(10000, counter);
}

#endif // USE_MUTEX_INSTEAD_OF_SPINLOCK
