//
// Created by sed on 28.07.23.
//

#include "spinlock.h"

#include "main.h"

#include <cassert>


#ifndef USE_MUTEX_INSTEAD_OF_SPINLOCK

/// @brief Default ctor
CSpinLock::CSpinLock() {
	lock_flag.clear(); // Done this way, because VC++ can't do it normally.
	owner_id = std::thread::id();
}

/// @brief destructor - mark as destroyed, lock and go
CSpinLock::~CSpinLock() {
	bool need_lock = !hasLock();

	if ( need_lock ) {
		lock();
	}
	is_destroyed.store( true );
	if ( need_lock ) {
		unlock();
	}
}

/// @brief return true if this thread has an active lock
bool CSpinLock::hasLock() {
	// This works, because unlock() sets the owner_id to std::thread::id() which is a neutral and unused id.
	return ( std::this_thread::get_id() == owner_id );
}

/** @brief Get a lock
 * Warning: No recursive locking possible! Only lock once!
 **/
void CSpinLock::lock() {
	std::thread::id this_id = std::this_thread::get_id();
	assert( !hasLock() && "ERROR: Lock already owned!" );

	while ( lock_flag.test_and_set() && !is_destroyed.load( ATOMIC_READ ) ) {
		std::this_thread::yield();
	}
	if ( !is_destroyed.load( ATOMIC_READ ) ) {
		owner_id = this_id;
	}
}

/// @brief unlock if this thread owns the lock. Otherwise do nothing.
void CSpinLock::unlock() {
	assert( hasLock() && "ERROR: Lock *NOT* owned!" );

	if ( hasLock() ) {
		owner_id = std::thread::id();
		lock_flag.clear( std::memory_order_release );
	}
}

#endif // USE_MUTEX_INSTEAD_OF_SPINLOCK
