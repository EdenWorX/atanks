#ifndef ATANKS_LEVELCREATOR_H
#define ATANKS_LEVELCREATOR_H 1
//
// Created by sed on 28.07.23.
//


#include "globaldata.h"
#include "globaltypes.h"

/// Class to background level creation
class LevelCreator {
	abool_t   in_progress[ 4 ]{ false };
	abool_t   i_must_yield{ true };
	abool_t   i_shall_die{ false };
	CSpinLock fiLock;
	int32_t   fiVal = 0;
	void      add_fi();

public:
	explicit LevelCreator();
	void operator() ();
	void die_now();
	bool has_progress();
	void print_state() const;
	void work_alone();
	void working_on( int32_t what );
	void yield();

	/* Status Getters */
	[[nodiscard]] bool can_work() const;
	[[nodiscard]] bool is_finished() const;
};


#endif // ATANKS_LEVELCREATOR_H
