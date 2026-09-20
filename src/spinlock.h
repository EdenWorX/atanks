#ifndef ATANKS_SPINLOCK_H
#define ATANKS_SPINLOCK_H 1
//
// Created by sed on 28.07.23.
//

#include "globaltypes.h"

#include <thread>

/** @brief minimal spinlock class
 * It can do nothing but lock and unlock. No recursive locks.
 * But then it is a lot faster and leaner than mutexes and critical
 * sections ever can be. ;)
 **/
class CSpinLock {
public:
	/// Construct an unlocked spinlock.
	explicit CSpinLock();
	/// Destroy the spinlock.
	~CSpinLock();

	CSpinLock( CSpinLock const& )             = delete;
	CSpinLock& operator= ( CSpinLock const& ) = delete;

	/// Test whether the caller holds the lock.
	bool       has_lock();
	/// Acquire the lock (spin on contention).
	void       lock();
	/// Release the lock.
	void       unlock();

private:
	abool_t         is_destroyed{ false };
	aflag_t         lock_flag{ false };
	std::thread::id owner_id;
};

#endif // ATANKS_SPINLOCK_H
