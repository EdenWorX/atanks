#ifndef ATANKS_SRC_GAMELOOP_H_INCLUDED
#define ATANKS_SRC_GAMELOOP_H_INCLUDED

#include "player.h"

/// Helper Class to background level creation
class LevelCreator {
	abool_t in_progress[ 4 ]{ false };
	abool_t i_must_yield{true};
	abool_t i_shall_die { false };
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

// The massive game loop, rewritten here for
// all sorts of reasons.
void game();


#endif // ATANKS_SRC_GAMELOOP_H_INCLUDED
