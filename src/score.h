#ifndef ATANKS_SCORE_H
#define ATANKS_SCORE_H 1
//
// Created by sed on 28.07.23.
//

#include "player.h"

/// @brief small struct to order the score board, used by the end-of-game-sorting, too
struct sScore {
	int32_t     color  = BLACK;
	int32_t     diff   = 0;
	int32_t     idx    = -1;
	int32_t     killed = 0;
	int32_t     kills  = 0;
	char const* name   = nullptr;
	sScore*     next   = nullptr;
	sScore*     prev   = nullptr;
	int32_t     score  = 0;

	sScore&     operator= ( PLAYER& rhs ) {
                color  = rhs.color;
                idx    = rhs.index;
                killed = rhs.killed;
                kills  = rhs.kills;
                name   = rhs.getName();
                score  = rhs.score;
                diff   = kills - killed;
                return *this;
	}
};

// sort players. The returned scores point to an array that must be deleted.
sScore* sort_scores();


#endif // ATANKS_SCORE_H
