#ifndef ATANKS_LEVELCREATOR_H
#define ATANKS_LEVELCREATOR_H 1
//
// Created by sed on 28.07.23.
//


#include "globaldata.h"
#include "globaltypes.h"

/// Class to background level creation.
///
/// Runs terrain and sky generation (perlin noise, see perlin.cpp) off the
/// main thread while the menu stays responsive.
class LevelCreator {
	abool_t   in_progress[ 4 ]{ false };
	abool_t   i_must_yield{ true };
	abool_t   i_shall_die{ false };
	CSpinLock fiLock;
	int32_t   fiVal = 0;
	void      add_fi();

public:
	/// Construct a level creator.
	explicit LevelCreator();
	/// Run background generation.
	void operator() ();
	/// Abort background generation.
	void die_now();
	/// Test for generation progress.
	bool has_progress();
	/// Print the generator state.
	void print_state() const;
	/// Stop yielding to the main thread.
	void work_alone();
	/// Note the current generation step.
	void working_on( int32_t what );
	/// Yield to the main thread.
	void yield();

	/* Status Getters */
	/// Test whether generation may proceed.
	[[nodiscard]] bool can_work() const;
	/// Test whether generation finished.
	[[nodiscard]] bool is_finished() const;
};


#endif // ATANKS_LEVELCREATOR_H
