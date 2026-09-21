#ifndef ATANKS_SCORE_H
#define ATANKS_SCORE_H 1
//
// Created by sed on 28.07.23.
//

#include "player.h"

/** @struct sScore
 * @brief Small struct to order the score board, used by the end-of-game-sorting, too.
 **/
struct sScore {
	int32_t     color  = BLACK;   ///< Player color.
	int32_t     diff   = 0;       ///< Kills minus deaths.
	int32_t     idx    = -1;      ///< Player index.
	int32_t     killed = 0;       ///< Deaths.
	int32_t     kills  = 0;       ///< Kills.
	char const* name   = nullptr; ///< Player name.
	sScore*     next   = nullptr; ///< Next score entry.
	sScore*     prev   = nullptr; ///< Previous score entry.
	int32_t     score  = 0;       ///< Score.

	/// Copy player stats into a score entry.
	sScore&     operator= ( CPlayer& rhs ) {
                color  = rhs.color;
                idx    = rhs.index;
                killed = rhs.killed;
                kills  = rhs.kills;
                name   = rhs.get_name();
                score  = rhs.score;
                diff   = kills - killed;
                return *this;
	}
};

// sort players. The returned scores point to an array that must be deleted.
sScore* sort_scores();


#endif // ATANKS_SCORE_H
