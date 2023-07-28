//
// Created by sed on 28.07.23.
//

#include "score.h"

/** @brief sort players by scores.
 *
 * The return value is the pointer to the allocated array, users
 * must use its prev() pointer to find the head entry.
 *
 * @return a pointer to the scores array. This must be deleted.
 **/
sScore* sort_scores() {
	auto*   scores     = new sScore[ env.numGamePlayers ];
	sScore* score_head = scores;
	sScore* score_tail = scores;
	sScore* curr       = nullptr;

	for ( int32_t z = 0; z < env.numGamePlayers; z++ ) {
		curr            = score_head;
		scores[ z ]     = *( env.players[ z ] );
		scores[ z ].idx = z; // The game index is needed.

		// Walk to find a lower score:
		while ( curr && ( curr->score > scores[ z ].score ) ) {
			curr = curr->next;
		}

		// Walk to find a lower diff:
		while ( curr && ( curr->score == scores[ z ].score ) && ( curr->diff > scores[ z ].diff ) ) {
			curr = curr->next;
		}

		// Walk to find a lower kills value:
		while ( curr && ( curr->score == scores[ z ].score ) && ( curr->diff == scores[ z ].diff )
		        && ( curr->kills > scores[ z ].kills ) ) {
			curr = curr->next;
		}

		// Walk to find a higher killed value:
		while ( curr && ( curr->score == scores[ z ].score ) && ( curr->diff == scores[ z ].diff )
		        && ( curr->kills == scores[ z ].kills ) && ( curr->killed < scores[ z ].killed ) ) {
			curr = curr->next;
		}

		// Walk to find a higher name value:
		while ( curr && ( curr->score == scores[ z ].score ) && ( curr->diff == scores[ z ].diff )
		        && ( curr->kills == scores[ z ].kills ) && ( curr->killed == scores[ z ].killed )
		        && ( strcmp( curr->name, scores[ z ].name ) < 0 ) ) {
			curr = curr->next;
		}

		// If there is a curr, sort the new score before it.
		if ( curr && ( curr != &scores[ z ] ) ) {
			scores[ z ].prev = curr->prev;
			scores[ z ].next = curr;
			if ( scores[ z ].prev ) {
				scores[ z ].prev->next = &scores[ z ];
			}
			curr->prev = &scores[ z ];
			if ( score_head == curr ) {
				score_head = &scores[ z ];
			}
		}

		// Otherwise, this is the new tail:
		else if ( score_tail != &scores[ z ] ) {
			scores[ z ].prev = score_tail;
			score_tail->next = &scores[ z ];
			score_tail       = &scores[ z ];
		}
	} // End of sorting scores

	return scores;
}
