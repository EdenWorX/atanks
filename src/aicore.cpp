#include "aicore.h"

#include "beam.h"
#include "explosion.h"
#include "missile.h"
#include "player.h"
#include "random.h"
#include "tank.h"

#include <cassert>

/** @struct sItemListEntry
 * @brief doubly linked list element to organize the AIs item preferences.
 **/
struct sItemListEntry {
	int32_t         amount     = 0;     //!< Number of items in stock.
	bool            escape     = false; //!< Set to true if "getting away" points are awarded.
	bool            kamikaze   = false; //!< Set to true if self destruct points are awarded.
	sItemListEntry* next       = nullptr;
	int32_t         preference = 0;     //!< Shortcut to the players preferences.
	sItemListEntry* prev       = nullptr;
	int32_t         score      = 0;     //!< How likely the AI uses this item.
	bool            selectable = false; //!< Some are not selectable, like parachutes.
	int32_t         type       = 0;     //!< The (enum) EItemType of the item

	explicit sItemListEntry( sItemListEntry* prev_ );
	~sItemListEntry();

	[[nodiscard, maybe_unused]] char const* get_name() const { return item[type].get_name(); }
};

/** @struct sOppMemEntry
 * @brief doubly linked list element to organize the AIs opponent memory.
 **/
struct sOppMemEntry {
	bool          alive       = true;    //!< False if the tank is destroyed.
	int32_t       attempts    = 0;       //!< How often tried to hit this round.
	int32_t       buried_l    = 0;       //!< Buried level to the left.
	int32_t       buried_r    = 0;       //!< Buried level to the right.
	double        diff_life   = 0.;      //!< Difference to bots life value: (this - opp).
	double        distance    = 0.;      //!< Shortcut to the absolute distance between both tanks.
	int32_t       dmg_done    = 0;       //!< damage done in simulation to calculate hit score.
	sOpponent*    entry       = nullptr; //!< The AIs sOpponent memory (see players.h).
	bool          hasRepulse  = false;   //!< Whether or not the opponent has a repulsor shield up.
	sOppMemEntry* next        = nullptr;
	bool          is_buried   = false;   //!< Whether buried_l+buried_r is greater than BURIED_LEVEL.
	bool          onSameTeam  = false;   //!< True if on the same team as the player.
	double        opLife      = 0.;      //!< Full opponents life, which is tank->sh + tank->l.
	double        opX         = 0;       //!< X-coordinate of the opponents tank.
	double        opY         = 0;       //!< Y-coordinate of the opponents tank.
	sOppMemEntry* prev        = nullptr;
	bool          revengeDone = false;   //!< Wether the score has already taken revenge into account.
	int32_t       score       = 0;       //!< How likely the AI attacks this opponent.
	double        team_mod    = 1.;      //!< Multiplier for the score according to which teams both belong to.

	explicit sOppMemEntry( sOppMemEntry* prev_ );
	~sOppMemEntry();

	[[nodiscard, maybe_unused]] char const* get_name() const { return entry->opponent->get_name(); }
};

/** @struct sWeapListEntry
 * @brief doubly linked list element to organize the AIs weapon preferences
 **/
struct sWeapListEntry {
	int32_t         amount      = 0;     //!< Number of weapons in stock.
	bool            blast_out   = false; //!< Set to true if blasting out points are awarded.
	int32_t         delay       = 0;     //!< Used to track delayed weapons.
	double          dmg_cluster = 0.;    //!< Cluster full damage.
	double          dmg_single  = 0.;    //!< Single shot damage.
	double          dmg_spread  = 0.;    //!< Spread full damage.
	bool            kamikaze    = false; //!< Set to true if self destruct points are awarded.
	sWeapListEntry* next        = nullptr;
	int32_t         preference  = 0;     //!< Shortcut to the players preferences.
	sWeapListEntry* prev        = nullptr;
	int32_t         radius      = 0;     //!< Blast radius of the weapon.
	int32_t         score       = 0;     //!< How likely the AI uses this weapon.
	int32_t         spread      = 1;     //!< Checked weapon spread value. (See CAICore::get_memory())
	int32_t         subMunCount = 0;     //!< Number of sub munition "bomblets"
	int32_t         subMunType  = -1;    //!< Clusters and such have sub munition.
	int32_t         type        = 0;     //!< The (enum) EWeaponType of the weapon.

	explicit sWeapListEntry( sWeapListEntry* prev_ );
	~sWeapListEntry();

	[[nodiscard, maybe_unused]] char const* get_name() const { return weapon[type].get_name(); }
};

/// @brief Template swapper, the types just need prev/next pointers
template< typename t_t > static void swap_entries( t_t* lhs, t_t* rhs ) {
	if ( lhs && rhs && ( lhs != rhs ) ) {
		// backup neighbourhood (and use as short cuts ;-) )
		t_t* l_next = lhs->next;
		t_t* l_prev = lhs->prev;
		t_t* r_next = rhs->next;
		t_t* r_prev = rhs->prev;

		// Insert rhs into lhs location
		if ( l_next && ( l_next != rhs ) ) {
			l_next->prev = rhs;
		}
		if ( l_prev && ( l_prev != rhs ) ) {
			l_prev->next = rhs;
		}

		// Insert lhs into rhs location
		if ( r_next && ( r_next != lhs ) ) {
			r_next->prev = lhs;
		}
		if ( r_prev && ( r_prev != lhs ) ) {
			r_prev->next = lhs;
		}

		// Move rhs to lhs location
		rhs->next = l_next == rhs ? lhs : l_next;
		rhs->prev = l_prev == rhs ? lhs : l_prev;

		// Move lhs to (former) rhs location
		lhs->next = r_next == lhs ? rhs : r_next;
		lhs->prev = r_prev == lhs ? rhs : r_prev;
	}
}

/// @brief Template sorter, the types need prev, next and score.
/// Sorting is done by score in descending order. If *head is sorted
/// down the list, it is set to the new first element.
template< typename t_t > static void sort_entries( t_t** head ) {
	if ( !head || !( *head ) ) {
		return;
	}

	bool sorted = false;

	while ( !sorted ) {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		sorted    = true;

		t_t* curr = *head;
		t_t* next = curr->next;

		while ( next ) {
			if ( next->score > curr->score ) {
				sorted = false;
				swap_entries( curr, next );
				if ( *head == curr ) {
					*head = next;
				}
			} else {
				curr = next;
			}
			next = curr->next;
		} // end of having next
	} // end of not sorted

#if defined( ATANKS_DEBUG_AIMING ) || defined( ATANKS_DEBUG_EMOTIONS )
	DEBUG_LOG_AI( "Memory Sorting", "Sorting results:", 0 )
	t_t*    curr = *head;
	int32_t nr   = 1;
	while ( curr ) {
		if ( curr->score > -10'000 ) {
			DEBUG_LOG_AI( "Memory Sorting", "% 3d: Score %5d (%s)", nr++, curr->score, curr->get_name() )
			curr = curr->next;
		} else {
			curr = nullptr;
		}
	}
#endif // ATANKS_DEBUG_AIMING || ATANKS_DEBUG_EMOTIONS
}

#if defined( ATANKS_DEBUG_AIMING ) || defined( ATANKS_DEBUG_EMOTIONS )
/// @brief Get a string describing the given AI @a level
[[nodiscard]] static char const* getLevelName( int32_t level ) {
	switch ( level ) {
		case 0:
			return "HUMAN (!!!)";
			break;
		case 1:
			return "Useless";
			break;
		case 2:
			return "Guesser";
			break;
		case 3:
			return "Range Finder";
			break;
		case 4:
			return "Targetter";
			break;
		case 5:
			return "Deadly";
			break;
		case 6:
			return "Deadly + 1";
			break;
		default:
			break;
	}
	return "OUT OF RANGE (!!!)";
}
#endif // Need AI Debug helper


/// @brief CAICore default constructor
CAICore::CAICore() {
	// As the opponent counts, and both weapons and items
	// list sizes are fixed, memory is reserved here.

	/// 1) Items
	for ( int32_t i = 0; allow_work && ( i < ITEMS ); ++i ) {
		try {
			item_curr = new itentry_t( item_last );
			if ( !item_head ) {
				item_head = item_curr;
			}
			item_last = item_curr;
		} catch ( std::bad_alloc& e ) {
			cerr << "Unable to reserve " << sizeof( itentry_t );
			cerr << " bytes for AI item chain: " << e.what() << endl;

			destroy();
			allow_work = false;
		}
	}

	/// 2) Opponents
	for ( int32_t i = 0; allow_work && ( i < env.num_game_players ); ++i ) {
		// Look for highest AI type
		if ( ( env.players[i]->type <= DEADLY_PLAYER ) && ( env.players[i]->type > best_type ) ) {
			best_type = env.players[i]->type;
		}

		// Create memory chain
		try {
			mem_curr = new opentry_t( mem_last );
			if ( !mem_head ) {
				mem_head = mem_curr;
			}
			mem_last = mem_curr;
		} catch ( std::bad_alloc& e ) {
			cerr << "Unable to reserve " << sizeof( opentry_t );
			cerr << " bytes for AI memory chain: " << e.what() << endl;

			destroy();
			allow_work = false;
		}
	}

	/// 3) Weapons
	for ( int32_t i = 0; allow_work && ( i < WEAPONS ); ++i ) {
		try {
			weap_curr = new weentry_t( weap_last );
			if ( !weap_head ) {
				weap_head = weap_curr;
			}
			weap_last = weap_curr;
		} catch ( std::bad_alloc& e ) {
			cerr << "Unable to reserve " << sizeof( weentry_t );
			cerr << " bytes for AI weapon chain: " << e.what() << endl;

			destroy();
			allow_work = false;
		}
	}

	// Stop if no work can be done
	is_stopped = !allow_work;

	DEBUG_LOG_AI( "CAICore", "Instance created", 0 )
}

/// @brief CAICore destructor
CAICore::~CAICore() {
	if ( is_working ) {
		if ( !is_stopped ) {
			this->stop();
		}
		while ( is_working ) {
			std::this_thread::yield();
		}
	}

	// Clean up memory chains:
	this->destroy();

	DEBUG_LOG_AI( "CAICore", "Instance destroyed", 0 )
}

/// @brief return the currently active player or nullptr if not working
CPlayer* CAICore::active_player() const {
	if ( is_working ) {
		return player;
	}
	return nullptr;
}

/** @brief aim the current selection
 * @param[in] combo_attempt the current attempt number of the combo
 * @param[in] combo_tries the total number of attempts for the combo
 * @param[in] may_move if set to true, the AI might try to move the tank into a better position.
 * @return true if the aiming resulted in a usable hit.
 **/
bool CAICore::aim( int32_t combo_attempt, int32_t combo_tries, bool may_move ) {
	pl_stage = PS_AIM;

	DEBUG_LOG_AIM( player->get_name(), "Starting to aim %s at %s", weapon[weap_idx].get_name(), mem_curr->entry->opponent->get_name() )

	int32_t rng_attempt = 0;
	bool    is_last     = ( combo_attempt == combo_tries ) && need_success;

	// reset current values as there can be no guarantee that the
	// last selected combination works for the current weapon/opponent
	// selection.
	sanitize_curr();
	hill_detected = false;
	// Note: curr_overshoot is reset to MAX_OVERSHOOT in calc_attack() but
	// might have an actual traced value from calc_boxed(), so do not reset
	// it here again.


	// Reset aiming round memory
	best_score      = NEUTRAL_ROUND_SCORE;
	best_angle      = angle;
	best_power      = power;
	best_prime_hit  = false;
	best_overshoot  = MAX_OVERSHOOT;
	last_ang_mod    = 0;
	last_overshoot  = MAX_OVERSHOOT;
	last_pow_mod    = 0;
	last_reverted   = false;
	last_score      = 0;
	last_was_better = false;
	reached_x       = static_cast< int32_t >( std::lround( x ) );
	reached_y       = static_cast< int32_t >( std::lround( y ) );


	// loop until finished, forced off or ending unsuccessfully
	while ( !is_stopped && ( ++rng_attempt <= findRngAttempts ) ) {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		DEBUG_LOG_AIM(
			player->get_name(),
			"[%d/%d] Starting with: Angle % 3d, Power % 4d",
			rng_attempt,
			findRngAttempts,
			GET_DISP_ANGLE( curr_angle ),
			curr_power
		)

		int32_t hit_score    = 0;
		int32_t has_crashed  = 0;
		int32_t has_finished = 0;

		// Modifications for this round:
		int32_t ang_mod = 1 + RAND_AI_1P;     // [ 1;  7]
		int32_t pow_mod = ( 10 + RAND_AI_1P ) // [10; 17]
		                * curr_power / 100;   // [10;340]

		// Lower ang_mod and pow_mod if the last overshoot isn't that high
		int32_t abs_last_overshoot = std::abs( last_overshoot );
		int32_t max_ang_mod        = abs_last_overshoot / ( ai_level * 20 ) + 1;
		int32_t max_pow_mod        = abs_last_overshoot / ( ai_level * 5 ) + 5;

		if ( ang_mod > max_ang_mod ) {
			DEBUG_LOG_AIM( player->get_name(), " => ang_mod too high (%d / %d) ; reducing...", ang_mod, max_ang_mod )
			ang_mod = max_ang_mod;
		}

		// [max_]pow_mod must be powers of five
		max_pow_mod += max_pow_mod % 5;
		pow_mod     += pow_mod % 5;

		if ( pow_mod > max_pow_mod ) {
			DEBUG_LOG_AIM( player->get_name(), " => pow_mod too high (%d / %d) ; reducing...", pow_mod, max_pow_mod )
			pow_mod = max_pow_mod;
		}


		// See where we are going:
		trace_weapon( has_crashed, has_finished );
		hit_score = calc_hit_score( is_last && need_success );


		// See whether this shot actually got nearer to the opponent.
		// (Note: Otherwise a better score just means less collateral damage!)
		bool is_nearer = std::abs( last_overshoot ) >= std::abs( curr_overshoot );


		DEBUG_LOG_AIM(
			player->get_name(),
			"[%d/%d] => Score %d, Overshoot %d (%s [last: %d])",
			rng_attempt,
			findRngAttempts,
			hit_score,
			curr_overshoot,
			is_nearer ? "Nearer" : "Farther",
			last_overshoot
		)


		// Note down a new best score:
		bool new_best_score = ( hit_score > best_score );
		if ( ( new_best_score && curr_prime_hit ) || ( !best_prime_hit && ( new_best_score || curr_prime_hit ) ) ) {

			sanitize_curr();

			// Note: Better score with prime hit, prime hit for the first time,
			//       or better score with prime never hit.
			DEBUG_LOG_AIM(
				player->get_name(),
				"[%d/%d] => New best score %d [%d best] using %d at %d°",
				rng_attempt,
				findRngAttempts,
				hit_score,
				best_score,
				curr_power,
				curr_angle
			)

			best_angle     = curr_angle;
			best_overshoot = curr_overshoot;
			best_power     = curr_power;
			best_prime_hit = curr_prime_hit;
			best_score     = hit_score;

			// The last modifications seem to have brought something
			ang_mod = ( last_ang_mod + ( SIGN( last_ang_mod ) * ang_mod ) ) / 2;
			pow_mod = ( last_pow_mod + pow_mod ) / 2 * SIGN( best_overshoot );
		}

		// Otherwise a movement might be in order
		else if (
			can_move                                      // No failed or finished moving done, yet
		        && may_move                                   // Half of the oppAttempts are off
		        && ( rng_attempt >= ( findRngAttempts / 2 ) ) // Half the aiming, too
		        && ( buried < BURIED_LEVEL )                  // Not buried
		        && ( !best_prime_hit || ( best_score <= 0 ) ) // nothing achieved here
		        && need_success                               // nothing achieved otherwise so far
		        && move_tank()                                /* movement done */
		) {
			// Reclaim this rng_attempt
			--rng_attempt;
			// And continue, we need to retrace the weapon
			continue;
		}


		// The outcome has to be checked:
		if ( allow_work && !is_stopped ) {

			/* The following situations can occur:
			 *
			 * A) The shot (or part of them) have not been finished.
			 *    In this case power must be reduced and the angle
			 *    has to be moved in a more neutral position if it
			 *    is too steep or flat.
			 * B) With steel or wrap wall the shot might crash into
			 *    a wrap ceiling or the wall and ceiling made of steel.
			 *    Generally this can only be fixed by lowering power and
			 *    getting away from 45° angles.
			 * C) The hit is nearer than the last one.
			 *    This is good. If it even has a positive hit_score,
			 *    it can be noted down as a new best hit.
			 * D) The hit is farther away. If the score is better,
			 *    the direction of modification seems correct.
			 *    Otherwise it might be better to revert those changes.
			 */

			// --- Situation A) The shot was not finished. ---
			//-------------------------------------------------
			if ( !has_finished // Pure situation A must be taken care of
			     || ( ( has_finished < weap_curr->spread ) && ( ( has_finished - RAND_AI_0P ) < 0 ) ) ) {

				DEBUG_LOG_AIM(
					player->get_name(),
					"[%d/%d] %d / %d finished, trying to correct",
					rng_attempt,
					findRngAttempts,
					has_finished,
					weap_curr->spread
				)

				fix_unfinished( ang_mod, pow_mod );
				last_reverted   = ( SIGN( last_ang_mod ) != SIGN( ang_mod ) );
				last_was_better = false;
			} // end of having lost the shot prediction


			// --- Situation B) The shot(s) crashed       ---
			//------------------------------------------------
			else if (
				( has_crashed == weap_curr->spread )
			        || ( ( has_crashed > 0 ) && ( ( has_crashed + RAND_AI_0P ) >= weap_curr->spread ) )
			) {

				DEBUG_LOG_AIM(
					player->get_name(),
					"[%d/%d] %d / %d crashed, trying to correct",
					rng_attempt,
					findRngAttempts,
					has_crashed,
					weap_curr->spread
				)

				fix_crashed( ang_mod, pow_mod );

				// If there is a positive hit_score, halve it for every
				// shot that crashed:
				if ( hit_score > 0 ) {
					for ( int32_t i = 0; i < has_crashed; ++i ) {
						if ( RAND_AI_0P ) {
							hit_score /= 2;
						}
					}
				}

				last_reverted   = ( SIGN( last_ang_mod ) != SIGN( ang_mod ) );
				last_was_better = false;
			}


			// --- Situation C) The shot is nearer to the target. ---
			//--------------------------------------------------------
			else if ( is_nearer ) {
				// Note: if this is a new best score, some adaptation has
				//       already been made above.
				if ( hit_score < last_score ) {
					DEBUG_LOG_AIM(
						player->get_name(),
						"[%d/%d] => Nearer but not a better score %d [%d best]",
						rng_attempt,
						findRngAttempts,
						hit_score,
						best_score
					)

					// Just modify the new angle mod to mimic the last
					// with new values
					ang_mod         = std::abs( ang_mod ) * SIGN( last_ang_mod );

					last_was_better = true;
				} else {
					last_was_better = false;
				}

				last_reverted = ( SIGN( last_ang_mod ) != SIGN( ang_mod ) );

				// pow_mod must have the opposite sign of the overshoot:
				pow_mod = std::abs( pow_mod ) * SIGN( curr_overshoot ) * -1;

				DEBUG_LOG_AIM(
					player->get_name(),
					"[%d/%d] New angle mod %d, new power mod %d",
					rng_attempt,
					findRngAttempts,
					ang_mod,
					pow_mod
				)
			} // end of having a nearer hit


			// --- Situation D) The shot hit farther away than the best. ---
			//---------------------------------------------------------------
			else {
				DEBUG_LOG_AIM(
					player->get_name(),
					"[%d/%d] Farther impact (%d curr, %d best)"
					" [score %d]",
					rng_attempt,
					findRngAttempts,
					curr_overshoot,
					best_overshoot,
					hit_score
				)

				fix_overshoot( ang_mod, pow_mod, hit_score );

				last_reverted = SIGN( ang_mod ) != SIGN( last_ang_mod );

				DEBUG_LOG_AIM(
					player->get_name(),
					"[%d/%d] New angle mod %d, new power mod %d",
					rng_attempt,
					findRngAttempts,
					ang_mod,
					pow_mod
				)
			} // end of situation C


			// Try to fix 180° shots if no positive score was achieved:
			if ( ( 180 == curr_angle ) && !ang_mod && ( hit_score < 1 ) ) {
				ang_mod = SIGN( mem_curr->opX - x ) * ( RAND_AI_1P + 1 ) * -1.;
				DEBUG_LOG_AIM( player->get_name(), "Vertical shot detected, new angle mod %d", ang_mod )
			}

			// Otherwise check if we actually reach a non-steel wall if the
			// shot was flipped.
			else if (
				has_flipped && ( std::abs( curr_overshoot ) > std::abs( mem_curr->opX - x ) )
			        && ( SIGN( reached_x - x ) != SIGN( mem_curr->opX - x ) )
			) {
				DEBUG_LOG_AIM(
					player->get_name(),
					"Flip shot failed, flipping back from %d° to %d°",
					GET_DISP_ANGLE( curr_angle ),
					GET_DISP_ANGLE( FLIP_ANGLE( curr_angle ) )
				)
				curr_angle  = FLIP_ANGLE( curr_angle );
				has_flipped = false;
			}

			// Power modification can be modified by a difference between
			// the overshoot and the actual modification according to
			// AI settings:
			// ----------------------------------------------------------
			double power_diff =
				static_cast< double >( std::abs( curr_overshoot ) - std::abs( pow_mod ) )
			        / static_cast< double >( std::abs( ang_mod ) ? std::abs( ang_mod ) : 1 );

			if ( ( curr_overshoot < MAX_OVERSHOOT ) && ( power_diff > std::abs( pow_mod ) ) && ( hit_score < 1 ) ) {
				DEBUG_LOG_AIM(
					player->get_name(),
					"Too low power mod difference %d"
					" (overshoot %d, pow_mod %d)",
					ROUND( power_diff ),
					curr_overshoot,
					ROUND( pow_mod )
				)

				pow_mod += ROUND( power_diff * focus_rate / 2. * SIGNd( pow_mod ) );

				if ( std::abs( pow_mod ) > max_pow_mod ) {
					pow_mod = max_pow_mod * SIGN( pow_mod );
				}

				DEBUG_LOG_AIM(
					player->get_name(),
					std::abs( pow_mod ) == max_pow_mod ? "pow_mod %d at maximum!" : "Hopefully fixed power mod: %d",
					pow_mod
				)
			}


			// Make sure both modifications applied end in a sane results:
			// -----------------------------------------------------------

			// Test angle to the right
			if ( ( curr_angle + ang_mod ) < 90 ) {
				if ( curr_angle > 90 ) {
					// Just sanitize
					ang_mod = 90 - curr_angle;
				} else {
					// Pull up to try again from a very different view
					ang_mod = ROUND( 60. * focus_rate ) // + [10;60]
					        - ( RAND_AI_0P * 5 );       // - [ 5;25]
				}
			} // end of sanitizing angle right

			// Test angle to the left
			else if ( ( curr_angle + ang_mod ) > 270 ) {
				if ( curr_angle < 270 ) {
					// Just sanitize
					ang_mod = 270 - curr_angle;
				} else {
					// Pull up to try again from a very different view
					ang_mod = ROUND( -60. * focus_rate ) // - [10;60]
					        + ( RAND_AI_0P * 5 );        // + [ 0;25]
				}
			} // end of sanitizing angle left


			// Test bottom power range
			if ( ( curr_power + pow_mod ) < MIN_POWER ) {
				if ( curr_power > MIN_POWER ) {
					// Just sanitize
					pow_mod = MIN_POWER - curr_power;
				} else {
					// Give more power to go somewhere else
					pow_mod = ROUND( 900. * focus_rate ) // + [150;900]
					        - ( RAND_AI_0P * 50 );       // - [  0;250]
				}
			}

			// Test upper power range
			if ( ( curr_power + pow_mod ) > MAX_POWER ) {
				if ( curr_power < MAX_POWER ) {
					// Just sanitize
					pow_mod = MAX_POWER - curr_power;
				} else {
					// Give more power to go somewhere else
					pow_mod = ROUND( -900. * focus_rate ) // - [150;900]
					        + ( RAND_AI_0P * 50 );        // + [  0;250]
				}
			}


			// Apply mods:
			curr_angle += ang_mod;
			curr_power += pow_mod;


			// Save current score, mods and overshot:
			last_ang_mod   = ang_mod;
			last_overshoot = curr_overshoot;
			last_pow_mod   = pow_mod;
			last_score     = hit_score;
		} // end of allow_work and not is_stopped


	} // end of aiming loop

	DEBUG_LOG_AIM(
		player->get_name(),
		"Final score with angle %d, power %d : %d => %s%s",
		GET_DISP_ANGLE( best_angle ),
		best_power,
		best_score,
		( best_score > 0 ) || ( is_last && need_success ) ? "Success!" : "Failure!",
		is_last && need_success ? " (is_last forced!)" : ""
	)

	// If this was the last try, and it did not reach the target having
	// a negative best score, assume that the path is blocked.
	// However, if a best setup is already known, revert to that.
	if ( ( weap_idx < WEAPONS ) && is_last && need_success && ( best_setup_score < 0 ) && ( best_round_score < 0 ) && ( best_score < 0 )
	     && ( best_overshoot < 0 )                           // too short
	     && ( ( ( reached_y > BOXED_TOP )                    // Not a ceiling crash,
	            && ( -best_overshoot > weap_curr->radius ) ) // but can't hit
	          || hill_detected ) /* if a hill was detected, it must be removed */ ) {
		bool free_tank = std::abs( reached_x - x ) < weapon[RIOT_BLAST].radius;

		if ( use_freeing_tool( free_tank, is_last ) ) {
			need_aim      = false;
			is_blocked    = true;
			hill_detected = true;

			if ( free_tank ) {
				calc_unbury( is_last );
			} else {
				// Write back best values
				curr_angle = best_angle;
				curr_power = best_power;

				// If this is a shot that got too short and a riot bomb
				// is chosen, flatten the angle to hit the mountain in between
				flatten_curr_ang();

				// Now set the results:
				sanitize_curr();
				angle = curr_angle;
				power = curr_power;
				DEBUG_LOG_AIM(
					player->get_name(),
					"Obstacle detected, trying to clear path using %s",
					weap_idx < WEAPONS ? weapon[weap_idx].get_name() : item[weap_idx - WEAPONS].get_name()
				)
			}
			return true;
		}
	}

	// Write back best values if this is a success:
	if ( best_score > best_round_score ) {
		best_round_score = best_score;
		curr_angle       = best_angle;
		curr_power       = best_power;
		sanitize_curr();
	}


	return ( ( best_round_score > 0 ) || ( is_last && need_success ) );
}

/// @brief Allow CAICore to create CFloatText instances
void CAICore::allow_text() {
	text_allowed.store( true, ATOMIC_WRITE );
}

/** @brief calculate basic attack values or set up the last ones
 * @param[in] attempt If this equals findTgtAttempts, this method is forced to
 *            not check too harshly, so it always returns true.
 * @param[in] tries number of attempts this bot has.
 * @return true if the method came up with something sane.
 **/
bool CAICore::calc_attack( int32_t attempt, int32_t tries ) {
	bool is_last   = ( ( attempt == tries ) && need_success );
	pl_stage       = PS_CALCULATE;
	has_flipped    = false;
	is_blocked     = false;
	need_aim       = false;
	curr_overshoot = MAX_OVERSHOOT;
	offset_x       = 0;
	offset_y       = 0;

	// If an item is chosen over a weapon, nothing is to be done
	if ( item_curr && !weap_curr ) {
		// If an item is selected, write back the currently used
		// angle and power, nothing is to be changed now.
		curr_angle = angle;
		curr_power = power;
		return true;
	}

	assert( weap_curr && "ERROR: weap_curr is nullptr in calc_attack() with no item chosen!" );

	// If the currently chosen opponent is the one attacked
	// in the last round, simply copy back the old attack
	// values and be done
	if ( last_opp && ( last_opp->opponent != player ) // don't repeat self destruct attempts
	     && ( last_opp == mem_curr->entry ) && ( last_weap == weap_idx ) ) {
		curr_angle = last_ang;
		curr_power = last_pow;

		if ( weap_idx < WEAPONS ) {
			need_aim = true;
		}

		return true;
	}

	/* Now that all is set up, we have to add some error to the bots' calculation.
	 * But we do not want to have to manipulate each and every calculation, so we simply move the target a bit.
	 */
	int32_t x_drift = env.screen_width / 40; // Limit drift to 5% screenwidth (Although the USELESS_PLAYER can get over it.)
	int32_t x_dir   = get_rand() % 2 ? -1 : 1;
	double  x_off   = error_multiplier * x_dir * ( ( get_rand() % x_drift ) + x_drift ); // [2.5;5.0]% screenwidth

	if ( ( ROUND( mem_curr->opX + x_off ) <= 0 ) || ( ROUND( mem_curr->opX + x_off ) >= env.screen_width ) ) {
		x_off *= -1.;
	}

	mem_curr->opX      += x_off;
	mem_curr->opY       = global.surface[ROUND( mem_curr->opX )];
	mem_curr->distance  = FABSDISTANCE2( x, y, mem_curr->opX, mem_curr->opY );


	/* If the current target is different or there was no last target,
	 * a basic set of values must be generated.
	 *
	 * Outline:
	 * --------
	 * There are five possible scenarios:
	 * a) The tank is not buried (enough) and a laser is chosen:
	 *    -> a direct angle will do, make sure power is sane.
	 * b) The tank is buried and an appropriate tool is chosen:
	 *    -> fire tool at the most filled side or, if the difference is less
	 *       than the AI level, in the direction of the chosen opponent.
	 * c) Kamikaze
	 *    -> indicated by setting mem_curr to the own entry
	 *    -> if shaped weapon is chosen, fire 45° and power 150 to the side
	 *       where the terrain height is nearest to this tanks bottom.
	 *    -> if napalm is chosen, fire against the wind with power 100 - 300
	 *    -> otherwise fire 180° and power 200 + spread modification.
	 * d) Fire in non-boxed mode
	 *    -> normal calculation
	 * e) Fire in boxed mode
	 *    -> extended power-control after normal calculation
	 *    -> if the target can't be reached while staying below the ceiling,
	 *       check for an obstacle that can be removed and do so if found.
	 */

	DEBUG_LOG_AIM( player->get_name(), "[%d / %d] Starting to aim at %s", attempt, tries, mem_curr->entry->opponent->get_name() )
	DEBUG_LOG_AIM(
		player->get_name(),
		"Aim from %d/%d to %d/%d [drift %d, distance %d/%d]",
		ROUND( x ),
		ROUND( y ),
		ROUND( mem_curr->opX ),
		ROUND( mem_curr->opY ),
		ROUND( x_off ),
		ROUND( mem_curr->opX - x ),
		ROUND( mem_curr->opY - y )
	)

	/* Case a) The tank is not buried (enough) and a laser is chosen
	 * ===================================================================
	 */
	if ( ( buried < BURIED_LEVEL ) && ( SML_LAZER <= weap_idx ) && ( LRG_LAZER >= weap_idx ) ) {
		return calc_laser( is_last );
	}


	/* Case b) The tank is buried and an appropriate tool is chosen
	 * ===================================================================
	 * (This means that it must be checked whether this is an appropriate
	 *  tool or not. Here the method might fail if it isn't suitable.)
	 */
	if ( buried >= BURIED_LEVEL ) {
		return calc_unbury( is_last );
	}


	/* Case c) Kamikaze
	 * ===================================================================
	 */
	if ( mem_curr->entry->opponent == player ) {
		return calc_kamikaze( is_last );
	}


	/* Case d) Fire in non-boxed mode
	 * ===================================================================
	 * This is always done, the boxed mode variant below simply checks the
	 * values and tries to adapt.
	 * The flipping is only allowed if the same opponent is tried again.
	 */
	bool result = calc_standard( is_last, ( 0 == ( ++mem_curr->attempts % 2 ) ) );


	/* Case e) Fire in boxed mode
	 * ===================================================================
	 *    -> extended power-control after normal calculation
	 *    -> if the target can't be reached while staying below the ceiling,
	 *       check for an obstacle that can be removed and do so if found.
	 */
	if ( result && env.is_boxed && !is_blocked && need_aim && ( weap_idx < WEAPONS ) ) {
		result = calc_boxed( is_last );
	}


	return result;
}

/** @brief Case e) Fire in boxed mode
 *
 * Note: calc_attack() has to make sure this method is only called if it is
 * appropriate. No further checks are made within this method.
 *
 * @param[in] is_last If this is set to true, the method is forced to succeed.
 * @return true if sane values could be found.
 **/
bool CAICore::calc_boxed( bool is_last ) {
	// Return at once if the bot "forgets" that there is a ceiling:
	if ( !is_last && RAND_AI_1N ) {
		// With this even the useless bot has only a ~33% chance to forget...
		return true;
	}

	bool   crashed   = true; // Assume the shot crashed in the ceiling
	bool   finished  = false;
	auto   local_x   = ROUND( x );
	auto   local_y   = ROUND( y );
	double end_xv    = 0.;
	double end_yv    = 0.;
	bool   can_mod_a = true;
	bool   can_mod_p = true;
	bool   top_wrap  = false; // Whether the shot wrapped through a wrap ceiling
	bool   can_dig   = ( weap_idx >= BURROWER ) && ( weap_idx <= PENETRATOR );

	// Cycle until the ceiling isn't hit any more.
	while ( allow_work && !is_stopped && crashed && ( can_mod_a || can_mod_p )
	        && trace_shot( curr_angle, 0, finished, top_wrap, local_x, local_y, end_xv, end_yv ) && finished ) {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		crashed = false;

		if ( ( local_y <= BOXED_TOP )  // crashed on top
		                               // wrapped to bottom with dirt above is a ceiling crash, too.
		     || ( top_wrap && !can_dig // (Unless a penetrator trick shot is tried)
		          && ( local_y > global.surface[local_x].load() ) )
		     // Steel wall have additional wall crashes
		     || ( ( WALL_STEEL == env.current_wall_type ) && ( ( local_x <= 2 ) || ( local_x >= ( env.screen_width - 3 ) ) ) ) ) {

			crashed = true;

			// Either reduce angle (-1), or power (1), or both (0)
			int32_t mod_mode = RAND_AI_1P ? ( get_rand() % 2 ? -1 : 1 ) : 0;

			// Apply mods but do not reduce into nothingness
			if ( ( mod_mode < 1 ) && can_mod_a ) {
				if ( ( curr_angle > 90 ) && ( curr_angle < 270 ) ) {
					curr_angle += curr_angle < 180 ? -1 : 1;
				} else {
					can_mod_a = false;
				}
			}
			if ( ( mod_mode > -1 ) && can_mod_p ) {
				if ( curr_power > MIN_POWER ) {
					curr_power -= 5;
				} else {
					can_mod_p = false;
				}
			}

			DEBUG_LOG_AIM(
				player->get_name(),
				"Ceiling crash! Reducing %s [%d°/%d]",
				1 == mod_mode   ? "power          "
				: 0 == mod_mode ? "angle and power"
				                : "angle          ",
				GET_DISP_ANGLE( curr_angle ),
				curr_power
			)
		}
	} // end of crashed loop

	// If the last (not crashed) shot is finished but doesn't get to
	// the target, it is blocked. But this is only considered if
	// a) This is the last attempt and
	// b) This map has a wrap or steel ceiling and
	// c) There was no positive setup score already
	if ( finished && !crashed
	     && ( best_setup_score <= 0 )
	     // But do not bail out on first try!
	     && ( best_setup_score > NEUTRAL_ROUND_SCORE ) && ( weap_idx < WEAPONS ) && ( curr_overshoot < 0 ) // too short
	     && is_last && ( ( WALL_STEEL == env.current_wall_type ) || ( WALL_WRAP == env.current_wall_type ) )
	     && ( -curr_overshoot > weap_curr->radius )                                                        // Can't hit
	     && ( -curr_overshoot > ( mem_curr->distance / 3 * 2 ) ) ) {
		// Note: With big weapons and near opponents, the radius might
		// be larger than two thirds the distance, hence two checks.
		bool free_tank = FABSDISTANCE2( x, y, local_x, local_y ) < weapon[RIOT_CHARGE].radius;

		if ( use_freeing_tool( free_tank, is_last ) ) {
			need_aim   = false;
			is_blocked = true;
			if ( free_tank ) {
				calc_unbury( is_last );
			} else {
				// If a riot bomb is chosen, flatten the angle:
				flatten_curr_ang();
				sanitize_curr();
				angle = curr_angle;
				power = curr_power;
				DEBUG_LOG_AIM(
					player->get_name(),
					"Obstacle detected, trying to clear path using %s",
					weap_idx < WEAPONS ? weapon[weap_idx].get_name() : item[weap_idx - WEAPONS].get_name()
				)
			}
			return true;
		}

		// This did not work. (use_freeing_tool always
		// succeeds if is_last is true)
		return false;
	}

	return ( is_last || !crashed );
}

/** @brief calculate a hit score off the dmg_done values in the opponent memory
 *
 * This method cycles through the opponents memory, and sums up
 * the damage done with curr_weap to a total score according
 * to a) how much damage over the opponents health (aka overkill)
 * has been done and b) on which team they are compared to us.
 *
 * If the primary target was not hit, the score is ensured to be
 * negative, as collateral damage is discouraged. However, this is
 * only done if @a is_last is false, as collateral damage with a
 * total positive score is better than nothing on the very last attempt.
 *
 * @param[in] is_last if set to true then any score is accepted.
 * @return The accumulated score.
 **/
int32_t CAICore::calc_hit_score( bool is_last ) const {
	int32_t    hit_score    = 0;
	opentry_t* opp          = mem_head;
	bool       can_overkill = true;
	ETeamTypes target_team  = mem_curr ? mem_curr->entry->opponent->team : TEAM_NEUTRAL;
	bool       tgt_team_hit = false;
	auto       weap_type    = static_cast< EWeaponType >( weap_curr->type );


	// Dirt weapons and the reducer can not overkill
	if ( ( ( DIRT_BALL <= weap_type ) && ( SUP_DIRT_BALL >= weap_type ) ) || ( REDUCER == weap_type ) ) {
		can_overkill = false;
	}


	while ( opp ) {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		if ( opp->dmg_done > 0 ) {
			int32_t overkill  = 0;
			bool    is_killed = false;
			bool    shock_hit = ( is_shocked && ( opp->entry == shocker ) );
			double  self_mod  = opp->entry->opponent == player ? ( player->self_preservation + 1. ) * ai_level : 1.;
			// Note: team_mod is negative if same team, so self_mod must
			//       be positive, or self hits generate huge scores!

			// 1: Determine whether the opponent was killed.
			if ( can_overkill && ( opp->dmg_done >= opp->opLife ) ) {
				overkill      = ROUND( opp->dmg_done - opp->opLife );
				opp->dmg_done = ROUND( opp->opLife );
				is_killed     = true;
			}

			DEBUG_LOG_AIM(
				player->get_name(),
				"Total damage against %10s: %d (%s, %d overkill)",
				opp->entry->opponent->get_name(),
				opp->dmg_done,
				is_killed ? "KILLED" : "not killed",
				overkill
			)


			// 2: add the simple damage to the score:
			hit_score += ROUND(
				static_cast< double >( opp->dmg_done ) * opp->team_mod * self_mod * ( shock_hit ? ai_over_mod : 1. )
			);


			// 3: Raise the score a bit if it is collateral damage on
			//    non-neutral team members of our target, but not our team.
			if ( ( TEAM_NEUTRAL != target_team ) && ( player->team != target_team )
			     && ( opp->entry->opponent->team == target_team ) ) {
				hit_score    *= ROUND( 1. + ( ( player->defensive + 2.5 ) / 10. ) );
				tgt_team_hit  = true;
			}


			// 4: If the opponent is killed, add a bonus to the score depending
			//    on team hit, self hit and whether the bot needs money or not.
			if ( is_killed ) {
				double kill_bonus = opp->dmg_done;

				if ( need_money ) {
					kill_bonus *= ai_type_mod;
				}

				// add some more for killing the shocker:
				if ( shock_hit ) {
					kill_bonus += kill_bonus / ai_over_mod;
				}

				hit_score += ROUND( kill_bonus * opp->team_mod * self_mod );
			}


			// 5: Check for overkill and dock points for it.
			if ( overkill > 0 ) {
				double over_score = overkill;

				// It is bad if the bot needs money, as it wastes expensive ammo
				if ( need_money ) {
					over_score *= ai_type_mod + .5;
				}

				// On the other hand, if the bot hit the shocker, the overkill
				// isn't considered that bad, though.
				if ( shock_hit ) {
					over_score /= 3. - ai_over_mod;
				}

				// Generate a generally negative score:
				over_score = std::abs( over_score ) * -1.;

				// The more aggressive, the less the reduction will be,
				// but only if it is neithe rus nor our team that got hit
				if ( !opp->onSameTeam ) {
					over_score /= ( player->defensive - 2. ) * -1.;
				}

				// add a fraction of the overkill score
				hit_score += ROUND( over_score / ( 10. - ai_level_d ) );
			} // end of overkill score
		} // end of having damage done
		opp = opp->next;
	} // end of looping opponents

	// If the primary target was not hit and this is not the last
	// attempt, make hit_score to be negative, unless the enemy
	// team is decimated. In the latter case the score is simply
	// reduced according to whether the bot needs money or not.
	if ( !curr_prime_hit && !is_last && ( hit_score > 0. ) ) {
		if ( tgt_team_hit ) {
			hit_score /= ROUND( ( ai_type_mod + ai_level_d ) / ( player->defensive + ( need_money ? 2.5 : 4.0 ) ) );
		} else {
			hit_score = -1 * std::abs( hit_score );
		}
	}

	return hit_score;
}

/** @brief calculate a score according to where the shot hit and
 * collateral damage done to friend and foe.
 *
 * Damage is recorded in the opponent memory dmg_done value.
 *
 * @param[in] hit_x x coordinate where the current selection hit.
 * @param[in] hit_y y coordinate where the current selection hit.
 * @param[in] weap_rad Calculated radius of the weapon.
 * @param[in] dmg Calculated damage of the weapon.
 * @param[in] weap_type Type of the weapon.
 * @return The resulting score
 **/
void CAICore::calc_hit_damage( int32_t hit_x, int32_t hit_y, double weap_rad, double dmg, EWeaponType weap_type ) {
	if ( ( nullptr == weap_curr ) // no weapon no score
	     || ( 0 == weap_rad ) ) { // no radius, no hit
		return;
	}

	// If this has no damage it is either a dirt ball, a reducer
	// or a riot weapon.
	// As dirt balls and reducers must be evaluated, they get a fake
	// damage of their radius so a score can be generated.
	if ( 0 == dmg ) {
		if ( ( ( DIRT_BALL <= weap_type ) && ( SUP_DIRT_BALL >= weap_type ) ) || ( REDUCER == weap_type ) ) {
			dmg = weap_rad;
		} else {
			return;
		}
	}

	// Napalm blobs have a much higher full damage output
	// than listed, as they do damage over time:
	if ( NAPALM_JELLY == weap_type ) {
		dmg *= static_cast< double >( EXPLOSIONFRAMES * weapon[NAPALM_JELLY].etime ) / ai_over_mod;
	}

	// Now the score can be calculated
	opentry_t* opp = mem_head;
	DEBUG_LOG_AIM( player->get_name(), "Checking impact at %d x %d", hit_x, hit_y )

	while ( opp ) {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		if ( CTank* oppTank = opp->entry->opponent->tank ) {
			// Now calculate the score if in range
			double part_dmg = 0.;

			// For dirt balls, only whether the tank is in x-range counts
			// as dirt falls down
			if ( ( DIRT_BALL <= weap_type ) && ( SUP_DIRT_BALL >= weap_type ) && ( oppTank->x > ( hit_x - weap_rad ) )
			     && ( oppTank->x < ( hit_x + weap_rad ) ) ) {
				part_dmg = dmg;
			} else {
				// All others need a normal check
				part_dmg = get_hit_damage( oppTank, weap_type, hit_x, hit_y );
			}

			if ( part_dmg > 0. ) {

				// REDUCER must be set, it is only checked as valid, yet:
				if ( REDUCER == weap_type ) {
					part_dmg = opp->entry->opponent->damage_multiplier * 25.;
				}

				// Set hit damage according to primary or collateral damage
				if ( opp == mem_curr ) {
					curr_prime_hit  = true;
					opp->dmg_done  += ROUND( part_dmg );
					DEBUG_LOG_AIM(
						player->get_name(),
						"%10s in blast range : primary damage   : %4d, total %d",
						opp->entry->opponent->get_name(),
						ROUND( part_dmg ),
						opp->dmg_done
					)
				} else {
					opp->dmg_done += ROUND( part_dmg );
					DEBUG_LOG_AIM(
						player->get_name(),
						"%10s in blast range : collateral damage: %4d, total %d",
						opp->entry->opponent->get_name(),
						ROUND( part_dmg ),
						opp->dmg_done
					)
				}

			} // end of having damage delivered
		} // end of having a tank to consider

		opp = opp->next;
	}
}

/** @brief Case c) Kamikaze
 *
 * Note: calc_attack() has to make sure this method is only called if it is
 * appropriate. No further checks are made within this method.
 *
 * @param[in] is_last If this is set to true, the method is forced to succeed.
 * @return true if sane values could be found.
 **/
bool CAICore::calc_kamikaze( bool is_last ) {
	DEBUG_LOG_AIM( player->get_name(), "I have decided to go bye bye!", 0 )

	// Is the selection sane?
	if ( weap_curr->kamikaze ) {

		bool is_good = true;

		// Only weapons need any angle/power adaptation
		if ( weap_curr ) {
			if ( ( SHAPED_CHARGE <= weap_idx ) && ( CUTTER >= weap_idx ) ) {

				// For this it is necessary to look at the terrain.
				// This does not make sense if there isn't a flat area
				// at either side of the tank with an even height.
				int32_t bottom  = tank->get_bottom();
				int32_t max_rad = weapon[weap_idx].radius / 20;

				// With a power of 150, the weapon can be hurled ~45 pixels.
				// So check that place plus some pixels around according
				// to ai_level.
				int32_t to_go   = 1 + ai_level + ( RAND_AI_1P );
				int32_t dist    = 45 - ( to_go / 2 );
				int32_t diff_l  = 0;
				int32_t diff_r  = 0;
				auto    round_x = ROUND( x );
				int32_t xl      = round_x - dist;
				int32_t xr      = round_x + dist;

				for ( int32_t i = 0; i < to_go; ++i ) {
					--xl;
					++xr;

					// If any x is in/ beyond a non-wrap wall, screw it:
					// a - check left side
					if ( xl < 1 ) {
						if ( WALL_WRAP == env.current_wall_type ) {
							xl = env.screen_width - 1 - ( 1 - std::abs( xl ) );
						} else {
							diff_l += env.screen_height;
						}
					} else {
						diff_l += std::abs( static_cast< int32_t >( global.surface[xl].load() - bottom ) );
					}

					// b - check right side
					if ( xr > ( env.screen_width - 2 ) ) {
						if ( WALL_WRAP == env.current_wall_type ) {
							xr = 1 + ( env.screen_width - 1 - xr );
						} else {
							diff_r += env.screen_height;
						}
					} else {
						diff_r += std::abs( static_cast< int32_t >( global.surface[xr].load() - bottom ) );
					}
				} // end of looping distance to_go

				// The average distance is taken. This ignores sudden peaks
				// and gaps that might stop/swallow the projectile, but
				// that should be okay.
				diff_l        /= to_go;
				diff_r        /= to_go;

				int32_t a_mod  = get_rand() % ( 10 - ai_level ) * ( get_rand() % 2 ? -1 : 1 );
				int32_t p_mod  = get_rand() % ( 20 - ( 2 * ai_level ) ) * ( get_rand() % 2 ? -1 : 1 );
				if ( ( diff_l < diff_r ) && ( diff_l < max_rad ) ) {
					curr_angle = 225 + a_mod;
				} else if ( diff_r < max_rad ) {
					curr_angle = 135 + a_mod;
				} else {
					is_good = false; // May need emergency plan below
				}

				// Set power and write back values
				if ( is_good ) {
					curr_power = 150 + p_mod;
				}

				DEBUG_LOG_AIM(
					player->get_name(),
					"Firing %s at %d° with power %d%s",
					weapon[weap_idx].get_name(),
					GET_DISP_ANGLE( curr_angle ),
					curr_power,
					is_good ? "!" : " will not work! Need a plan!"
				)

			} else if ( ( SML_NAPALM <= weap_idx ) && ( LRG_NAPALM >= weap_idx ) ) {

				// Adapt according to the wind:
				auto    wind     = ROUND( global.wind );
				int32_t wind_mod = 10 + ( std::abs( wind ) * ( 1 + RAND_AI_0P ) );

				if ( wind > 0 ) {
					curr_angle = 225;
				} else if ( wind < 0 ) {
					curr_angle = 135;
				} else {
					curr_angle = 180;
				}

				curr_power = 100 + wind_mod;

				DEBUG_LOG_AIM(
					player->get_name(),
					"Firing %s at %d° with power %d (wind %d, wind_mod %d)",
					weapon[weap_idx].get_name(),
					GET_DISP_ANGLE( curr_angle ),
					curr_power,
					wind,
					wind_mod
				)

			} else {
				int32_t spread_mod = 50 + ( 10 * ( 1 + RAND_AI_0P ) );
				int32_t spread     = weapon[weap_idx].spread; // shortcut
				curr_angle         = 180;
				curr_power         = 200 + ( ( spread * spread_mod ) / ( 2 - ( spread % 2 ) ) );

				DEBUG_LOG_AIM(
					player->get_name(),
					"Firing %s at %d° with power %d",
					weapon[weap_idx].get_name(),
					GET_DISP_ANGLE( curr_angle ),
					curr_power
				)
			}
		}

		if ( is_good ) {
			sanitize_curr();
			angle = curr_angle;
			power = curr_power;
			return true;
		}
	} // end of sane item/weapon selection

	// No, this selection does not make sense.
	// However, if this is a last try, it must work somehow.
	if ( is_last
	     && ( use_item( ITEM_FATAL_FURY ) || use_item( ITEM_DYING_WRATH ) || use_item( ITEM_VENGEANCE ) || use_weapon( DTH_HEAD )
	          || use_weapon( NUKE ) || use_weapon( SML_NUKE ) || use_weapon( LRG_MIS ) || use_weapon( MED_MIS )
	          || use_weapon( SML_MIS ) ) ) {
		// Note: The trick is, that the first working selection
		// results in the if-statement to end, and SML_MIS always
		// works.
		curr_angle = 180;
		curr_power = 250;
		angle      = curr_angle;
		power      = curr_power;

		DEBUG_LOG_AIM(
			player->get_name(),
			"Emergency plan: Firing %s at %d° with power %d",
			weapon[weap_idx].get_name(),
			GET_DISP_ANGLE( curr_angle ),
			curr_power
		)

		return true;
	}

	// This can not work, and does not need to be forced.
	return false;
}

/** @brief Case a) The tank is not buried (enough) and a laser is chosen.
 *
 * Note: calc_attack() has to make sure this method is only called if it is
 * appropriate. No further checks are made within this method.
 *
 * @return True if the opponent can be hit, false if the shot is blocked or
 *         pumped into a wall or the ceiling.
 **/
bool CAICore::calc_laser( bool is_last ) {
	int32_t old_angle = curr_angle;
	int32_t old_power = curr_power;
	double  drift     = static_cast< double >( maxAiLevel + 2 - RAND_AI_0P ) * error_multiplier * ( get_rand() % 2 ? -1. : 1. );

	curr_power        = tank->p;
	curr_angle        = GET_SAFE_ANGLE( mem_curr->opX - x, mem_curr->opY - y, drift );

	// Power doesn't matter, but the values must be sane nonetheless:
	sanitize_curr();

	// Let's see where the laser ends:
	double start_x = 0;
	double start_y = 0;
	player->tank->get_guntop( curr_angle, start_x, start_y );

	CBeam   mind_beam( player, start_x, start_y, curr_angle, weap_curr->type, BT_MIND_SHOT );

	int32_t end_x = 0;
	int32_t end_y = 0;
	mind_beam.get_end_point( end_x, end_y );

	// Generate a score for this
	curr_prime_hit = false;
	// Note: calc_hit_damage() sets curr_prime_hit to true if we hit our target.

	// reset virtual damage on opponents.
	opentry_t* opp = mem_head;
	while ( opp ) {
		opp->dmg_done = 0;
		opp           = opp->next;
	}

	calc_hit_damage( end_x, end_y, weapon[weap_curr->type].radius, weap_curr->dmg_single, static_cast< EWeaponType >( weap_curr->type ) );
	int32_t hit_score = calc_hit_score( is_last && need_success );

	// If the target is behind a dirt wall, break up this attempt
	bool crashed = false;
	if ( !tank->shoot_clearance( curr_angle, mem_curr->distance, crashed ) || crashed ) {

		// ...unless this is a forced success ...
		if ( is_last ) {
			// at least reduce the score.
			// hit_score = hit_score;
		} else {
			curr_angle = old_angle;
			curr_power = old_power;
			need_aim   = true; // for the next try or the closure
			DEBUG_LOG_AI( player->get_name(), "Cancelling laser shot: %s", crashed ? "Wrong angle" : "Not enough clearance" )
			return false;
		}
	}

	// If a positive score was achieved, we are set
	if ( ( hit_score > 0 ) || is_last ) {
		// Write back values
		sanitize_curr();
		angle = curr_angle;
		power = curr_power;
		if ( ( hit_score > best_round_score ) || is_last ) {
			best_round_score = std::abs( hit_score );
		}

		DEBUG_LOG_AIM(
			player->get_name(),
			"Firing %s at %d° with power %d",
			weapon[weap_curr->type].get_name(),
			GET_DISP_ANGLE( curr_angle ),
			curr_power
		)

		return true;
	}

	curr_angle = old_angle;
	curr_power = old_power;
	need_aim   = true; // for the next try or the closure
	DEBUG_LOG_AI( player->get_name(), "Cancelling laser shot: %d score too low", hit_score )

	return false;
}

/** @brief calculate x and y offsets for weapons that need it.
 *
 * These offsets are stored in offset_x and offset_y, as they are needed
 * in multiple places.
 *
 * If the needed offset is off the screen, or makes no sense, the method
 * returns false. But if @a is_last is set to true, insane offsets are tried
 * to be fixed. The idea is, that the bot tries nevertheless out of pure
 * desperation.
 *
 * @param[in] is_last If set to true, the method never fails
 * @return true if sane offsets were found.
 **/
bool CAICore::calc_offset( bool is_last ) {
	bool result = true;

	offset_x    = 0;
	offset_y    = 0;

	/* Weapon type 1: Napalm bombs
	 * There are two situations to consider, the normal shot using wind
	 * and the under-run trick. The latter is evil and will almost always
	 * destroy any tank even with a small napalm bomb. Just place the
	 * bomb with tail wind under a tank. No shield can protect the tank
	 * and most jellies will burn into the hull. So this option must be
	 * limited.
	 * Otherwise the wind is what has to be taken into account plus the
	 * surroundings. If the wind side is above the opponent, more distance
	 * is needed than when the area is below the target tank.
	 */
	if ( ( weap_idx >= SML_NAPALM ) && ( weap_idx <= LRG_NAPALM ) ) {
		offset_x = ROUND( global.wind * ( ai_level + RAND_AI_1P ) * ( -1. - focus_rate ) );

		// The farther away the opponent is, the more power is needed to
		// bring the package to the target. More impact power means a higher
		// initial velocity of the blobs, so the offset must be tweaked a bit.
		offset_x      *= ROUND( 1. + ( mem_curr->distance / static_cast< double >( env.screen_width ) * focus_rate ) );


		int32_t pos_x  = ROUND( mem_curr->opX ) + offset_x;


		// If the resulting x position is not on the screen,
		// the calculation already failed.
		if ( pos_x < 2 ) {
			if ( is_last ) {
				// But this must be taken...
				pos_x    = 2;
				offset_x = pos_x - ROUND( mem_curr->opX );
			} else {
				result = false;
			}
		} else if ( pos_x > ( env.screen_width - 2 ) ) {
			if ( is_last ) {
				// The same...
				pos_x    = env.screen_width - 2;
				offset_x = ROUND( mem_curr->opX ) - pos_x;
			} else {
				result = false;
			}
		}

		// If the result is true, the area around pos_x must be checked
		// to determine the y offset and whether to adapt offset_x
		// any further or not.
		if ( result ) {
			bool    found = false;
			int32_t pos_y = global.surface[pos_x].load();

			if ( pos_y < ( mem_curr->opY - std::abs( offset_x ) + ai_level ) ) {
				// This means more distance is needed. So we search for
				// the next valid x position that goes down again or that
				// doubles the x distance, whatever comes earlier.
				int32_t mov_x = SIGN( global.wind ) * -1;
				int32_t max_x = pos_x + offset_x;

				if ( max_x < 2 ) {
					max_x = 2;
				}
				if ( max_x > ( env.screen_width - 2 ) ) {
					max_x = ( env.screen_width - 2 );
				}

				for ( ; !found && ( pos_x != max_x ); pos_x += mov_x ) {
					if ( global.surface[pos_x].load() > pos_y ) {
						pos_x -= mov_x; // One step back
						found  = true;
					}
				}
			} else if ( pos_y > ( mem_curr->opY + std::abs( offset_x ) - ai_level ) ) {
				// Here check the area towards the enemy. But going nearer
				// than half the distance means an under-run, which is only
				// allowed after an additional check.
				int32_t mov_x = SIGN( global.wind );
				int32_t max_x = pos_x - ( offset_x / ( RAND_AI_0P ? 1 : 2 ) );
				// Note: Yes, the RAND_AI_0P is the mentioned additional check. ;)
				int32_t max_y = mem_curr->entry->opponent->tank->get_bottom();

				for ( ; !found && ( pos_x != max_x ); pos_x += mov_x ) {
					if ( global.surface[pos_x].load() <= ( max_y - ai_level ) ) {
						found = true;
					}
				}
			}

			// adapt offset_x and offset_y now:
			offset_x = ROUND( pos_x - mem_curr->opX );
			offset_y = ROUND( global.surface[pos_x].load() - mem_curr->opY );
		}
	} // end of handling napalm

	/* Weapon type 2: Shaped charges.
	 * These are easy. The shaped charge have an y radius of 1/20 of the
	 * x radius. Within this y radius of the centre no damage is done, so
	 * the area from this radius + 1 to + (ai_level * 2) is checked on
	 * either sides whether there is an y position that allows to actually
	 * catch the enemy in the blast. If not, this try is a failure.
	 * However, there is another trick shot here: Place a big shaped charge
	 * like the cutter at the right height behind a hill and blast the
	 * opponents tank through it.
	 */
	else if ( ( weap_idx >= SHAPED_CHARGE ) && ( weap_idx <= CUTTER ) ) {
		int32_t rad_y  = weapon[weap_idx].radius / 20;
		int32_t dist_x = rad_y + RAND_AI_1P;
		int32_t max_dist =
			RAND_AI_0P ? ( dist_x * 2 ) + RAND_AI_1P      // normal shot
			           : weapon[weap_idx].radius * 2 / 3; // trick shot
		auto    seek_y  = ROUND( ( mem_curr->opY + mem_curr->entry->opponent->tank->get_bottom() ) / 2. );
		auto    left_x  = ROUND( mem_curr->opX - dist_x );
		auto    right_x = ROUND( mem_curr->opX + dist_x );
		int32_t left_y  = left_x > 2 ? std::abs( global.surface[left_x].load() ) : 0;
		int32_t right_y = right_x < ( env.screen_width - 2 ) ? std::abs( global.surface[right_x].load() ) : 0;
		bool    go_left = ( mem_curr->opX > x ); // Which side to prefer
		bool    found_l = false;
		bool    found_r = false;

		for ( ; !found_l && !found_r && ( dist_x < max_dist ); ++dist_x ) {
			left_x  = ROUND( mem_curr->opX - dist_x );
			right_x = ROUND( mem_curr->opX + dist_x );
			left_y  = left_x > 2 ? std::abs( global.surface[left_x].load() ) : 0;
			right_y = right_x < ( env.screen_width - 2 ) ? std::abs( global.surface[right_x].load() ) : 0;

			if ( std::abs( left_y - seek_y ) <= rad_y ) {
				found_l = true;
			}
			if ( std::abs( right_y - seek_y ) <= rad_y ) {
				found_r = true;
			}
		}

		// If both are valid, use what is preferred
		if ( found_l && found_r ) {
			if ( go_left ) {
				found_r = false;
			} else {
				found_l = false;
			}
		}

		// if none is found but this is the last_try, take the simple
		// distance to the preferred side
		if ( !found_l && !found_r ) {
			if ( is_last ) {
				if ( ( go_left && ( ( mem_curr->opX - rad_y - 1 ) > 1 ) )
				     || ( ( mem_curr->opX + rad_y + 1 ) > ( env.screen_width - 2 ) ) ) {
					found_l = true;
					left_x  = ROUND( mem_curr->opX - rad_y - 1 );
					left_y  = global.surface[left_x].load();
				} else {
					found_r = true;
					right_x = ROUND( mem_curr->opX + rad_y + 1 );
					right_y = global.surface[right_x].load();
				}
			} else {
				result = false;
			}
		}

		// If something is found, set the real offsets
		if ( found_l ) {
			offset_x = ROUND( left_x - mem_curr->opX );
			offset_y = ROUND( left_y - mem_curr->opY );
		} else if ( found_r ) {
			offset_x = ROUND( right_x - mem_curr->opX );
			offset_y = ROUND( right_y - mem_curr->opY );
		}
	} // end of handling shaped charges

	/* Weapon type 3: Driller
	 * The driller must be placed above an enemy tank. This means it is
	 * only useful if the tank is buried, or the tank shall be sunk into
	 * the surface. However, there might be the possibility of an under
	 * shot if the tank is placed on a mountain side.
	 */
	else if ( DRILLER == weap_idx ) {
		int32_t rad_x    = weapon[weap_idx].radius / 20;
		auto    pos_x    = ROUND( mem_curr->opX );
		int32_t pos_y    = global.surface[pos_x].load();
		int32_t max_dist = rad_x * 2 / 3;
		auto    min_y    = ROUND( mem_curr->opY - rad_x );
		int32_t max_y    = mem_curr->entry->opponent->tank->get_bottom() + rad_x;

		// If the direct coordinates are already in order, do not search
		// further. Otherwise, try to shift left and right.
		if ( ( pos_y > min_y ) && ( pos_y < max_y ) ) {
			bool found_l = false;
			bool found_r = false;
			for ( int32_t off_x = 1; !found_l && !found_r && ( off_x < max_dist ); ++off_x ) {

				int32_t left_x  = pos_x - off_x;
				int32_t right_x = pos_x + off_x;
				auto    left_y  = ROUND( left_x > 1 ? global.surface[left_x].load() : mem_curr->opY );
				auto right_y = ROUND( right_x < ( env.screen_width - 1 ) ? global.surface[right_x].load() : mem_curr->opY );
				if ( ( left_y < min_y ) || ( left_y > max_y ) ) {
					found_l = true;
					pos_x   = left_x;
					pos_y   = left_y;
				} else if ( ( right_y < min_y ) || ( right_y > max_y ) ) {
					found_r = true;
					pos_x   = right_x;
					pos_y   = right_y;
				}
			}

			// If this did not succeed, but it is the last_shot or
			// the AI chooses to bury its opponent, use the opponents
			// coordinates.
			if ( !found_l && !found_r ) {
				if ( is_last || ( RAND_AI_0N ) ) {
					pos_x = ROUND( mem_curr->opX );
					pos_y = ROUND( mem_curr->opY );
				} else {
					result = false;
				}
			}
		} // end of searching a position to use

		// Set offsets if all is well
		if ( result ) {
			offset_x = ROUND( pos_x - mem_curr->opX );
			offset_y = ROUND( pos_y - mem_curr->opY );
		}
	} // end of handling drillers

	return result;
}

/** @brief Case d) Fire in non-boxed mode
 *
 * Note: calc_attack() has to make sure this method is only called if it is
 * appropriate. No further checks are made within this method.
 *
 * @param[in] is_last If this is set to true, the method is forced to succeed.
 * @param[in] allow_flip_shot If set to true, the bot is allowed to shoot in
 * the opposite direction. On steel walls, this parameter is ignored.
 * @return true if sane values could be found.
 **/
bool CAICore::calc_standard( bool is_last, bool allow_flip_shot ) {
	bool result = calc_offset( is_last );
	need_aim    = true;


	// --- 1) Get a basic raw angle firing directly ---
	// ------------------------------------------------
	bool   wrapped = false;
	double opX     = mem_curr->opX + offset_x;
	double opY     = mem_curr->opY + offset_y;
	// just some shortcuts
	double  dist_x   = opX - x;
	double  dist_y   = opY - y;
	int32_t scrWidth = env.screen_width;

	// Do not start horizontally, this might happen quite often.
	// If the opponent is above, limit the angle to somewhere between
	// 60° and 75°. If it is below, limit angle between 20° and 35° and
	// limit the angle between 40° and 55° if ~equal.
	int32_t new_angle = GET_SAFE_ANGLE( dist_x, dist_y, 0 );
	auto    ang_limit = ROUND( focus_rate * static_cast< double >( get_rand() % 16 ) );

	if ( dist_y < -100 ) {       /* above */
		ang_limit += 60;
	} else if ( dist_y > 100 ) { /* below */
		ang_limit += 20;
	} else {                     /* equal */
		ang_limit += 40;
	}

	// Apply limit:
	if ( new_angle < ( 90 + ang_limit ) ) {
		new_angle = 90 + ang_limit;
	}
	if ( new_angle > ( 270 - ang_limit ) ) {
		new_angle = 270 - ang_limit;
	}


	// --- 2) Modify the beginning angle according to focus_rate ---
	// --- Keeping this more variable gives a larger range of   ---
	// --- starting points to go forth from.                    ---
	// ------------------------------------------------------------
	double angle_mod = ( get_rand() % 13 ) * focus_rate // useless: 0-2, deadly+1: 0-12
	                 * ( ( get_rand() % 2 ) ? -1. : 1. );
	while ( ( std::abs( angle_mod ) > 0. )
	        && ( ( ( new_angle > 180 ) && ( ( ( new_angle + angle_mod ) < 190 ) || ( ( new_angle + angle_mod ) > 260 ) ) )
	             || ( ( new_angle < 180 ) && ( ( ( new_angle + angle_mod ) > 170 ) || ( ( new_angle + angle_mod ) < 100 ) ) ) ) ) {
		angle_mod /= 2.;
	}
	new_angle += ROUND( angle_mod );


	// --- 3) If this is a wrap wall, check whether shooting ---
	// ---    through the wall is actually shorter.          ---
	// --- A note on allow_flip_shot: If shooting wrapped is ---
	// --- shorter, the AI will chose it more often the      ---
	// --- higher the AI level. (80% for a deadly bot)       ---
	// --- The flipping is then a possibility to shoot non-  ---
	// --- wrapped again.                                    ---
	// ---------------------------------------------------------
	if ( ( WALL_WRAP == env.current_wall_type ) && RAND_AI_0P ) {
		auto wrapDist = ROUND( opX > x ? x + scrWidth - 3 - opX : ( scrWidth - x - 3 + opX ) * -1 );

		if ( std::abs( wrapDist ) < std::abs( dist_x ) ) {
			wrapped     = true;
			has_flipped = true;
			dist_x      = wrapDist;
			new_angle   = FLIP_ANGLE( new_angle );

			DEBUG_LOG_AIM( player->get_name(), "Flipping through wrap wall at %d°", GET_DISP_ANGLE( new_angle ) )
		}
	}


	// --- 4) Switch sides if possible and allowed ---
	// -----------------------------------------------
	if ( ( WALL_STEEL != env.current_wall_type ) && allow_flip_shot && ( get_rand() % ( ( ai_level + 3 ) / 2 ) ) ) {
		new_angle = FLIP_ANGLE( new_angle );

		// The result of this flip is different for each wall type
		if ( WALL_RUBBER == env.current_wall_type ) {
			dist_x += opX > x ? ( x - 1. ) + ( ( x - 1. ) / BOUNCE_CHANGE )
			                  : ( scrWidth - opX - 2. ) + ( ( scrWidth - opX - 2. ) / BOUNCE_CHANGE );
		} else if ( WALL_SPRING == env.current_wall_type ) {
			dist_x += opX > x ? ( x - 1. ) + ( ( x - 1. ) / SPRING_CHANGE )
			                  : ( scrWidth - opX - 2. ) + ( ( scrWidth - opX - 2. ) / SPRING_CHANGE );
		} else if ( WALL_WRAP == env.current_wall_type ) {
			if ( wrapped ) {
				// Shoot directly again
				dist_x = opX - x;
			} else {
				dist_x = opX > x ? x + scrWidth - 3 - opX : ( scrWidth - x - 3 + opX ) * -1;
			}
		}

		DEBUG_LOG_AIM( player->get_name(), "Flipping %s wall at %d°", wrapped ? "back from" : "towards", GET_DISP_ANGLE( new_angle ) )

		// wrap / unwrap
		wrapped     = !wrapped;
		has_flipped = wrapped;
	}


	// --- 5) Adjust angle giving shooting clearance    ---
	// --- Here the clearance is either needed to reach ---
	// --- the next wall or half the distance to the    ---
	// --- selected opponent.                           ---
	// ----------------------------------------------------
	double  clearance = std::abs( dist_x / 2. );
	int32_t old_angle = new_angle;
	int32_t max_drift = ( ai_level + 1 ) / 2; // [1;3]
	bool    crashed   = false;

	if ( wrapped ) {
		// wrapped shots need clearance to the wall away from the opponent:
		clearance = opX > x ? x - 2 : env.screen_width - x - 2;
	}

	while ( ( new_angle < ( 180 - max_drift ) ) && !tank->shoot_clearance( new_angle, clearance, crashed ) && !crashed ) {
		++new_angle;
	}
	while ( ( new_angle > ( 180 + max_drift ) ) && !tank->shoot_clearance( new_angle, clearance, crashed ) && !crashed ) {
		--new_angle;
	}


	// --- 6) Revert to half the distance between both angles ---
	// ---    if no full clearance is possible.               ---
	// --- An attempt to remove possible obstacles might be   ---
	// --- triggered here.                                    ---
	// ----------------------------------------------------------
	if ( ( new_angle >= ( 180 - max_drift ) ) && ( new_angle <= ( 180 + max_drift ) ) ) {
		new_angle = ( new_angle + old_angle ) / 2;

		// If this is the last chance, try to clear the obstacle.
		// However, if there is already a setup with a positive
		// score, revert to that.
		if ( is_last && ( best_setup_score <= 0 ) ) {
			/* Range is from Useless and pain resistant (0.1) to
			 * (Deadly + 1) and very pain sensitive: [max rand value]
			 * Useless    : 0.1 * 1 * 5 =  0.5 [ 20]
			 * Deadly + 1 : 3.0 * 6 * 5 = 90.0 [120]
			 */
			is_blocked = true;
			result     = use_freeing_tool( false, is_last );
			curr_angle = new_angle;

			// Try not to bomb the ceiling:
			if ( ( curr_angle >= 150 ) && ( curr_angle <= 210 ) ) {
				flatten_curr_ang();

				// Write back curr_ang, or it gets overwritten below.
				new_angle = curr_angle;
			}

			DEBUG_LOG_AIM(
				player->get_name(),
				"Obstacle detected, trying to clear path using %s",
				weap_idx < WEAPONS ? weapon[weap_idx].get_name() : item[weap_idx - WEAPONS].get_name()
			)
		} else {
			// This did not work out
			result = false;
		}
	} // end of being blocked


	// --- 7) Find necessary power                ---
	// --- This is just an estimation on possible ---
	// ---  "air time" of the projectile          ---
	// ----------------------------------------------
	if ( result ) {
		// As there is nothing that can fail now, the new
		// angle is the one to go with:
		curr_angle     = new_angle;

		double slope_x = env.slope[curr_angle][0];
		double slope_y = env.slope[curr_angle][1];
		double rawTime = slope_x != 0. ? dist_x / slope_x : dist_x / 0.00000001; // 180° should be impossible though.

		// lower target, less power.
		// If the target is above, the projectile hits earlier than
		// on lower targets, where the projectile has to fall down there.
		double airTime = std::abs( rawTime ) + ( dist_y * slope_y * env.fall_vector * 2.0 );

		// Less airTime doesn't necessarily mean less power
		// Horizontal firing means more power needed even though
		// air time is minimised.
		curr_power = ROUNDu( std::sqrt( airTime * env.fall_vector ) * static_cast< double >( env.frames_per_second ) );

		// Power modification according to the bots focus rate
		// This helps to have slightly different starting powers to
		// begin aiming with.
		curr_power += ROUND( focus_rate * static_cast< double >( get_rand() % 51 ) * ( get_rand() % 2 ? -1. : 1. ) );
		// With a focus rate of [0.166;1] this results in a modification
		// between [-8.3;8.3] and [-50;50].

		// Be sure power is in the valid range:
		if ( curr_power > MAX_POWER ) {
			curr_power = MAX_POWER;
		}
		if ( curr_power < MIN_POWER ) {
			curr_power = MIN_POWER;
		}

		// Power is only available in a stepping of five
		curr_power -= curr_power % 5;

		DEBUG_LOG_AIM(
			player->get_name(),
			"Firing %s at %d° with power %d",
			weapon[weap_idx].get_name(),
			GET_DISP_ANGLE( curr_angle ),
			curr_power
		)
	} // end of having a result

	// Write back values and stop aiming if this is a blocked path freeing attempt
	if ( result && is_blocked ) {
		sanitize_curr();
		angle    = curr_angle;
		power    = curr_power;
		need_aim = false;
	}


	return result;
}

/** @brief Case b) The tank is buried and an appropriate tool is chosen
 *
 * Note: This means that it must be checked whether this is an appropriate
 * tool or not. Here the method might fail if it isn't suitable.
 *
 * @param[in] is_last If this is set to true, the method is forced to succeed.
 * @return true if sane values could be found.
 **/
bool CAICore::calc_unbury( bool is_last ) {
	DEBUG_LOG_AIM( player->get_name(), "I am buried! (%d >= %d)", buried, BURIED_LEVEL )

	// Suitable tool?
	if ( ( ( RIOT_BOMB <= weap_curr->type )         // Clear freeing tool
	       && ( RIOT_BLAST >= weap_curr->type ) )   // that clears dirt
	     || ( ( ( SHAPED_CHARGE > weap_curr->type ) // shaped charges can not
	            || ( CUTTER < weap_curr->type ) )   // be used, and neither can
	          && ( DRILLER != weap_curr->type )     // the driller, to self
	          && weap_curr->kamikaze ) ) {          // destruct while buried

		// To not blast away an obstacle towards a wall with no
		// enemies behind it, count how many enemies are on each
		// side, first:
		opentry_t* op       = mem_head;
		int32_t    op_left  = 0;
		int32_t    op_right = 0;
		while ( op ) {
			if ( op->alive && !op->onSameTeam ) {
				if ( op->opX < tank->x ) {
					op_left++;
				} else {
					op_right++;
				}
			}
			op = op->next;
		}

		// Determine starting values according to which side
		// is buried stronger, and where the opponents are:
		bool go_left = true;

		// It is better to go right instead of left if:
		// a) more enemies are on the right
		// b) the count is equal but the right side is more buried or
		// c) the current favourite target is on the right and the left
		//    is not that much more buried. (depends on AI level)
		if ( ( op_right > op_left )                                                             // a)
		     || ( ( op_right == op_left ) && ( buried_r > buried_l ) )                          // b)
		     || ( ( mem_curr->opX > x ) && ( std::abs( buried_r - buried_l ) < ai_level ) ) ) { // c)

			go_left = false;
		}

		// find a good starting angle where the obstacle begins:
		double dist    = ai_level * ( player->defensive + 3. ) * 2;
		bool   crashed = false;
		curr_angle     = 180;

		if ( go_left ) {
			while ( ( curr_angle < 250 ) && ( tank->shoot_clearance( curr_angle, dist, crashed ) || !crashed ) ) {
				++curr_angle;
			}
		} else {
			while ( ( curr_angle > 110 ) && ( tank->shoot_clearance( curr_angle, dist, crashed ) || !crashed ) ) {
				--curr_angle;
			}
		}

		// add a variant to the angle:
		curr_angle += ( ( get_rand() % 21 ) - 10 ) / ai_level;

		// If riot charges are used, 45° is the lower border.
		if ( ( RIOT_BOMB <= weap_curr->type ) && ( RIOT_BLAST >= weap_curr->type ) ) {
			if ( curr_angle < 135 ) {
				curr_angle = 135;
			}
			if ( curr_angle > 225 ) {
				curr_angle = 225;
			}
		}

		// Be sure current values are sane:
		sanitize_curr();

		angle      = curr_angle;
		power      = curr_power;
		need_aim   = false; // Already done here!
		is_blocked = true;

		DEBUG_LOG_AIM(
			player->get_name(),
			"Freeing myself using %s at %d° with power %d",
			weapon[weap_idx].get_name(),
			GET_DISP_ANGLE( angle ),
			power
		)

		return true;
	} // end of having selected an appropriate tool.

	// The only non-self-destruct way to use a weapon for freeing
	// one self is the shaped charge:
	if ( ( ( SHAPED_CHARGE <= weap_curr->type ) && ( CUTTER >= weap_curr->type ) ) || ( DRILLER == weap_curr->type ) ) {
		curr_angle = 180;
		curr_power = 10 + RAND_AI_0P;

		sanitize_curr();

		angle      = curr_angle;
		power      = curr_power;
		need_aim   = false; // Already done here!
		is_blocked = true;

		DEBUG_LOG_AIM(
			player->get_name(),
			"Freeing myself using %s at %d° with power %d",
			weapon[weap_idx].get_name(),
			GET_DISP_ANGLE( angle ),
			power
		)

		return true;
	}

	// emergency values if this is our last try:
	if ( is_last ) {
		if ( !use_freeing_tool( true, true ) ) {
			use_weapon( SML_MIS );
		}
		curr_angle = buried_l > buried_r ? 200 : 100 + ( get_rand() % 61 );
		curr_power = 500 + ( get_rand() % 501 );

		sanitize_curr();

		angle      = curr_angle;
		power      = curr_power;
		need_aim   = false; // Already done here!
		is_blocked = true;

		DEBUG_LOG_AIM(
			player->get_name(),
			"(last!) Freeing myself using %s at %d° with power %d",
			weapon[weap_idx].get_name(),
			GET_DISP_ANGLE( angle ),
			power
		)

		return true;
	}

	// Otherwise this has failed

	DEBUG_LOG_AIM(
		player->get_name(),
		"Nothing suitable selected (%s)",
		item_curr   ? item[weap_idx - WEAPONS].get_name()
		: weap_curr ? weapon[weap_idx].get_name()
		            : "NOTHING"
	)

	return false;
}

/// @return false if the initialization of this instance failed
bool CAICore::can_work() const {
	return allow_work;
}

/** @brief check the currently set item list and update its organization
 *
 * This checks every item compared to the currently selected
 * target and sets a score on usability. The list is then
 * sorted by score in descending order.
 **/
void CAICore::check_item_mem() {
	item_curr = item_head;

	DEBUG_LOG_AIM( player->get_name(), "Starting to check item memory", 0 )

	while ( item_curr ) {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		// Update score
		update_item_score( item_curr );

		// Advance
		item_curr = item_curr->next;
	}

	// Eventually sort the list
	sort_entries( &item_head );
}

/** @brief check the currently set memory and update its organization
 *
 * This looks into each entry whether there is new damage for
 * this turn, and updates the score.
 * After the score updates, the list is reordered, so the entry
 * with the highest score becomes mem_head, and the list ends with
 * the lowest scored entry.
 **/
void CAICore::check_opp_mem() {
	mem_curr = mem_head;

	DEBUG_LOG_AIM( player->get_name(), "Starting to check opponent memory", 0 )

	while ( mem_curr ) {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		// Update score
		update_opp_score( mem_curr );

		// Advance
		mem_curr = mem_curr->next;
	}

	// Do we have a new revengee?
	if ( revengee && ( player->revenge != revengee->opponent ) ) {
		player->revenge = revengee->opponent;
	}

	// Not a single one?
	if ( nullptr == revengee ) {
		player->revenge = nullptr;
	}

	// Eventually sort the list
	sort_entries( &mem_head );
}

/** @brief check the currently set weapon list and update its organization
 *
 * This checks every weapon compared to the currently selected
 * target and sets a score on usability. The list is then
 * sorted by score in descending order.
 **/
void CAICore::check_weap_mem() {
	weap_curr = weap_head;

	DEBUG_LOG_AIM( player->get_name(), "Starting to check weapon memory", 0 )

	while ( weap_curr ) {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		// Update score
		update_weap_score( weap_curr );

		// Advance
		weap_curr = weap_curr->next;
	}

	// Eventually sort the list
	sort_entries( &weap_head );
}

/// @brief destroy all memory chains
void CAICore::destroy() {
	while ( item_head ) {
		item_curr = item_head;
		item_head = item_curr->next;
		delete item_curr;
	}
	item_curr = nullptr;
	item_last = nullptr;

	while ( mem_head ) {
		mem_curr = mem_head;
		mem_head = mem_curr->next;
		delete mem_curr;
	}
	mem_curr = nullptr;
	mem_last = nullptr;

	while ( weap_head ) {
		weap_curr = weap_head;
		weap_head = weap_curr->next;
		delete weap_curr;
	}
	weap_curr = nullptr;
	weap_last = nullptr;
}

/// @brief Forbid CAICore to create CFloatText instances
void CAICore::forbid_text() {
	text_allowed.store( false );
}

/** @brief get the set players memory and check it
 *
 * This method fetches all sOpponent entries from the handled player,
 * and fills the item and weapon chains with the stock count and weapon
 * preferences.
 *
 * The scores are not calculated, and the list is not sorted.
 * Therefore check_opp_mem() must be called first when the thread
 * starts in operator().
 *
 * @return true if the memory could be copied
 **/
bool CAICore::get_memory() {
	assert( player && "ERROR: get_memory() reached with nullptr player?" );
	assert( tank && "ERROR: get_memory() reached with nullptr tank?" );
	assert( !tank->destroy && "ERROR: get_memory() reached with destroyed tank?" );


	/// === 1) Copy item information ===

	int32_t idx  = 0;
	int32_t pref = 0;

	item_curr    = item_head;
	item_last    = nullptr;

	assert( item_head && "ERROR: get_memory() called without item memory set up!" );

	if ( !item_head ) {
		return false;
	}

	do {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		if ( -1 < ( pref = player->get_item_pref( idx ) ) ) {
			item_curr->amount     = player->ni[idx];
			item_curr->preference = pref;
			item_curr->selectable = item[idx].selectable != 0;
			item_curr->type       = idx;

			// The kamikaze value is only pre-set to true for vengeance
			// items, all other must be determined if the bot really
			// chooses to self-destruct.
			if ( ( item_curr->type >= ITEM_VENGEANCE ) && ( item_curr->type <= ITEM_FATAL_FURY ) ) {
				item_curr->kamikaze = true;
			} else {
				item_curr->kamikaze = false;
			}

			// Advance current
			item_curr = item_curr->next;
		}
	} while ( ( ++idx < ITEMS ) && item_curr );


	/// === 2) Copy opponents information ===

	idx             = 0;
	int32_t    bcnt = 0;          // Bastion count
	int32_t    rcnt = 0;          // Rogue count;
	double     dail = ai_level_d; // [d]ouble [ai]_[l]evel
	sOpponent* opp  = nullptr;

	mem_curr        = mem_head;
	mem_last        = nullptr;

	assert( mem_head && "ERROR: get_memory() called without opponents memory set up!" );

	if ( !mem_head ) {
		return false;
	}

	do {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		mem_curr->alive = false; // must be confirmed

		if (( opp = player->get_opp_mem( idx ) )) {
			CTank* oppTank        = nullptr;

			mem_curr->attempts    = 0;
			mem_curr->entry       = opp;
			mem_curr->revengeDone = false;

			if ( opp->opponent && opp->opponent->tank && !opp->opponent->tank->destroy ) {
				oppTank = opp->opponent->tank;
			}

			// The other values depend on whether an active tank was found:
			if ( oppTank ) {
				mem_curr->is_buried  = oppTank->how_buried( &mem_curr->buried_l, &mem_curr->buried_r ) > BURIED_LEVEL;
				mem_curr->hasRepulse = oppTank->has_repulsor_activated();
				mem_curr->opLife     = oppTank->l + oppTank->sh;
				mem_curr->opX        = oppTank->x;
				mem_curr->opY        = oppTank->y;
				mem_curr->diff_life  = curr_life - mem_curr->opLife;
				mem_curr->distance   = FABSDISTANCE2( x, y, mem_curr->opX, mem_curr->opY );

				if ( oppTank->l > 0 ) {
					mem_curr->alive = true;
				}

				// Is this the last one? then set as default best choice:
				if ( last_opp == opp ) {
					best_setup_mem = mem_curr;
				}
			} else {
				// Reset some values if there is no tank
				mem_curr->opLife    = 0.;
				mem_curr->diff_life = curr_life;
				mem_curr->distance  = env.screen_width * env.screen_height;
			}

			// Some calculations can be cut short if this is ourselves:
			if ( opp->opponent == player ) {
				mem_curr->onSameTeam = true;
				mem_curr->team_mod =
					( ( ( player->pain_sensitivity + 1. )      // [1;4]
				            * ( player->self_preservation + 1. ) ) // [1;4]
				          + 2. )                                   // [ 3  ; 18]
				        / -1.;                                     // [-1.5; -9]
			} else {
				mem_curr->onSameTeam = ( ( TEAM_NEUTRAL != player->team ) && ( opp->opponent->team == player->team ) );

				// team_mod is a multiplier reflecting the general behaviour
				// against the own and other teams.
				if ( TEAM_BASTION == player->team ) {
					// Bastion go strongly for Rogue and protect their team
					if ( mem_curr->onSameTeam ) {
						mem_curr->team_mod = ( 2. + dail ) / -2.; // [-1.5; -4.]
					} else if ( TEAM_ROGUE == opp->opponent->team ) {
						mem_curr->team_mod = 2. * ai_level;       // [2;12]
					} else {
						mem_curr->team_mod = ai_level;            // [1; 6]
					}
				} else if ( TEAM_ROGUE == player->team ) {
					// Rogue go for everyone, slightly favouring Bastion and do
					// not care that much hitting their own team members.
					if ( mem_curr->onSameTeam ) {
						mem_curr->team_mod = ( 2. + dail ) / -3.; // [-1; -2.66]
					} else if ( TEAM_BASTION == opp->opponent->team ) {
						mem_curr->team_mod = 1.25 * ai_level;     // [1.25;7.5]
					} else {
						mem_curr->team_mod = ai_level;            // [1   ;6  ]
					}
				} else {
					// Neutrals go slightly more for the teams, and less for
					// other neutrals. This is supposed to reflect the fact
					// that Bastion and Rogue have friends with them helping them
					// out. Neutrals are all alone and considered less dangerous.
					if ( TEAM_NEUTRAL == opp->opponent->team ) {
						mem_curr->team_mod = 1. + ( dail / 2. ); // => [1.5;4.]
					} else {
						mem_curr->team_mod = .5 + dail;          // => [1.5;6.5]
						if ( TEAM_BASTION == opp->opponent->team ) {
							++bcnt;
						} else {
							++rcnt;
						}
					}
				} // end of team_mod determination
			} // end of opponent handling

			// Advance current
			mem_curr = mem_curr->next;
		}
		++idx;
	} while ( opp && mem_curr );

	// If this is a neutral player, it has counted bastion and rogue. This is
	// done to raise the team_mod whenever any of these teams sport more
	// than one remaining tank.
	if ( ( TEAM_NEUTRAL == player->team ) && ( ( bcnt > 1 ) || ( rcnt > 1 ) ) ) {
		double j_mod = dail / 10. * static_cast< double >( bcnt - 1 );
		double s_mod = dail / 10. * static_cast< double >( rcnt - 1 );
		mem_curr     = mem_head;

		while ( mem_curr ) {

			if ( ( TEAM_BASTION == mem_curr->entry->opponent->team ) && ( bcnt > 1 ) ) {
				mem_curr->team_mod += j_mod;
			} else if ( ( TEAM_ROGUE == mem_curr->entry->opponent->team ) && ( rcnt > 1 ) ) {
				mem_curr->team_mod += s_mod;
			}

			mem_curr = mem_curr->next;
		}
	}

	/// === 3) Copy weapon information ===

	double dmgMod = player->damage_multiplier;
	idx           = 0;
	pref          = 0;

	weap_curr     = weap_head;
	weap_last     = nullptr;

	assert( weap_head && "ERROR: get_memory() called without weapon memory set up!" );

	if ( !weap_head ) {
		return false;
	}

	// Reset blast values, they have to be found anew:
	blast_min = 0.;
	blast_med = 0.;
	blast_big = 0.;
	blast_max = 0.;

	do {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		if ( -1 < ( pref = player->get_weap_pref( idx ) ) ) {
			int32_t subMun = weapon[idx].submunition; // short-cut
			double  damage = weapon[idx].damage * dmgMod * weapon[idx].get_delay_div();

			// === Dirt weapons have a "damage" based on their radius ===
			if ( ( DIRT_BALL <= idx ) && ( SMALL_DIRT_SPREAD >= idx ) ) {
				damage = weapon[idx].radius * ( 1.5 + player->defensive );
			}

			weap_curr->amount = player->nm[idx];

			// Chain missiles and such have a delay of the same count they shoot
			// missiles. However, weapons without a delay get a value of 1 here,
			// so looping in trace_weapon() is much simpler.
			weap_curr->delay = weapon[idx].delay > 0 ? weapon[idx].delay : 1;

			// To be able to track weapons that trigger other sub munitions,
			// their configuration must be remembered, too:
			weap_curr->subMunCount = weapon[idx].numSubmunitions;
			weap_curr->subMunType  = subMun;
			// Non-spread weapons have their spread value set to 1. To catch
			// future cases where this might be different, the value is
			// stored and sanitized here:
			weap_curr->spread = weapon[idx].spread > 0 ? weapon[idx].spread : 1;
			weap_curr->dmg_cluster =
				weapon[idx].numSubmunitions > 0
					? dmgMod * static_cast< double >( weapon[subMun].damage )
			                          * static_cast< double >( weapon[idx].numSubmunitions )
					: 0.;
			weap_curr->dmg_single = damage;
			weap_curr->dmg_spread = damage * static_cast< double >( weap_curr->spread );
			weap_curr->preference = pref;
			weap_curr->radius     = weapon[idx].radius;
			weap_curr->type       = idx;

			// Save blast value for opponents score calculation
			double ds = weap_curr->dmg_single;
			if ( SML_MIS == idx ) {
				if ( blast_min < ds ) {
					blast_min = ds;
				}
				if ( blast_med < ds ) {
					blast_med = ds;
				}
				if ( blast_big < ds ) {
					blast_big = ds;
				}
				if ( blast_max < ds ) {
					blast_max = ds;
				}
			} else if ( ( ( MED_MIS == idx ) || ( LRG_MIS == idx ) ) && ( weap_curr->amount > 0 ) ) {
				if ( blast_med < ds ) {
					blast_med = ds;
				}
				if ( blast_big < ds ) {
					blast_big = ds;
				}
				if ( blast_max < ds ) {
					blast_max = ds;
				}
			} else if ( ( ( SML_NUKE == idx ) || ( NUKE == idx ) ) && ( weap_curr->amount > 0 ) ) {
				if ( blast_big < ds ) {
					blast_big = ds;
				}
				if ( blast_max < ds ) {
					blast_max = ds;
				}
			} else if ( (( DTH_HEAD == idx )) && ( weap_curr->amount > 0 ) && ( blast_max < ds ) ) {
				blast_max = ds;
			}

			// If this is the last weapon used, store as default best choice
			if ( last_weap == idx ) {
				best_setup_weap = weap_curr;
			}

			// Advance current
			weap_curr = weap_curr->next;
		}
	} while ( ( ++idx < WEAPONS ) && weap_curr );

	return true;
}

/// @brief flatten curr_ang if a riot bom is chosen to clear the path
/// Note: No checks about being blocked, call it when appropriate. Only
///       the weapon is checked against riot bombs.
void CAICore::flatten_curr_ang() {
	if ( ( RIOT_BOMB <= weap_curr->type ) && ( HVY_RIOT_BOMB >= weap_curr->type ) ) {
		int32_t div = 1 + ( ( RAND_AI_1P + 2 ) / 2 ); // [2;4]
		// Minimum : 1 + ( (0 + 2) / 2 ) = 1 + ( 2 / 2 ) = 1 + 1 = 2
		// Useless:  1 + ( (1 + 2) / 2 ) = 1 + ( 3 / 2 ) = 1 + 1 = 2
		// Deadly+1: 1 + ( (5 + 2) / 2 ) = 1 + ( 7 / 2 ) = 1 + 3 = 4
		if ( curr_angle > 180 ) {
			curr_angle += ( 270 - curr_angle ) / div;
		} else {
			curr_angle -= ( curr_angle - 90 ) / div;
		}
	}
}

/// @brief adapt @a ang_mod and @a pow_mod when a shot using them crashed.
void CAICore::fix_crashed( int32_t& ang_mod, int32_t& pow_mod ) const {
	// Unless a hill was detected, the angle mod must not be greater than 1
	if ( !hill_detected && ( ang_mod > 1 ) ) {
		ang_mod = 1;
	}

	// Use a unified angle or there has to be an if/else for the same code
	int32_t fix_ang = curr_angle > 180 ? FLIP_ANGLE( curr_angle ) : curr_angle;

	// The following rules must be applied:
	// 1) If boxed mode is on, the angle must not be higher than 150°
	//    or it has to be reduced more.
	// 2) Otherwise lower the angle further away from 45° (aka 135° here) if
	//    it is already low.
	//    If it isn't, it is raised anyway.
	// 3) If none of the above apply, assume the angle to be in order if it
	//    is between 130 and 140, which is 45° +/- 5°.
	if ( env.is_boxed && ( fix_ang > 150 ) ) { // 1)
		ang_mod *= -1 * RAND_AI_1P;
		// Do not overdo it:
		while ( std::abs( ang_mod ) > ( ai_level + 2 ) ) {
			ang_mod /= 2;
		}
	} else if ( ( fix_ang > 100 ) && ( fix_ang <= 135 ) ) { // 2)
		ang_mod *= -1;
	} else if ( ( fix_ang > 130 ) && ( fix_ang < 140 ) ) {  // 3)
		ang_mod = 0;                                    // None needed
	}

	// now flip ang_mod if the angle was flipped:
	ang_mod *= curr_angle > 180 ? -1 : 1;

	// The power must be reduced if it is greater than the x distance.
	auto pow_diff = ROUND( curr_power - std::abs( mem_curr->opX - x ) );
	if ( pow_diff > 0 ) {
		pow_mod = std::abs( pow_mod ) * -1;
		// And strengthen the power reduction more if
		// the power is more than 50% over the distance
		if ( pow_diff >= ( std::abs( mem_curr->opX - x ) / 2 ) ) {
			pow_mod *= 2 * RAND_AI_1P;
		}
	}
}

/// @brief Try to adapt @a ang_mod and @a pow_mod according to the current
/// overshoot and where the best hit landed.
void CAICore::fix_overshoot( int32_t& ang_mod, int32_t& pow_mod, int32_t hit_score ) {
	// Here are some more possible (sub) situations to consider:
	// 1) The current score is at least better than the last.
	//    This can happen if the shot does no longer hit team mates.
	//    The important situation is, if the overshoot is very small
	//    and a new best score is achieved. The bigger the weapon, the
	//    higher the probability that this might be the case.
	// 2) Both the current and the last overshoot were negative, the
	//    angle was optimized towards 45°/135° and the power was raised.
	//    Having a worse overshoot then can happen if the gun was
	//    lowered and the shot crashes into the side of a hill or
	//    mountain.
	//    The angle must then be brought towards 180° more than the
	//    last angle modification brought it away from it.
	//    However, the angle has to be flatter if we are in a boxed
	//    environment, to not go up and into the ceiling too soon.
	// 3) The current score is worse than the last score.
	//     a) The last score was better than the one before.
	//        The modifications might have been too strong, try
	//        values between the two.
	//     b) That was two worse tries in a row.
	//        The direction was wrong, and the last modifications
	//        must be reverted and strengthened by the current set
	//        mods.
	// 4) No last score or the same, just adapt the mods according to
	//    whether the shot was too short or too long.


	bool angle_was_optimized = false;
	if ( ( ( curr_angle > ( env.is_boxed ? 205 : 180 ) ) && ( curr_angle <= 225 ) && ( last_ang_mod > 0 ) )
	     || ( ( curr_angle < ( env.is_boxed ? 155 : 180 ) ) && ( curr_angle >= 135 ) && ( last_ang_mod < 0 ) ) ) {
		angle_was_optimized = true; // Optimized towards 45° on its side
	}

	// reset hill detection if the current overshoot isn't short
	if ( ( curr_overshoot > 0 ) || ( hit_score > 0 ) ) {
		hill_detected = false;
	}

	if ( last_score && ( hit_score > 0 ) && ( hit_score > last_score ) ) {

		// 1) Better score with worse overshoot.
		// Here a best score might happen. But this is only noteworthy
		// if the hit_score is positive. Otherwise it would simply
		// mean that less damage was done (team and others) and that is
		// hardly anything to note down.
		// Note: if this is a new best score, some adaptation has
		//       already been made in aim().
		if ( hit_score < best_score ) {
			// Only adapt the signedness of the mods and note that
			// the aiming is not there, yet:
			DEBUG_LOG_AIM( player->get_name(), " => Better score %d [%d last]", hit_score, last_score )
			ang_mod = std::abs( ang_mod ) * SIGN( last_ang_mod );
			pow_mod = std::abs( pow_mod ) * SIGN( curr_overshoot ) * -1;
		}

		// At least better than the last
		last_was_better = true;
		hill_detected   = false;
	} else if (
		( hit_score <= 0 ) && ( last_overshoot < 0 ) && ( curr_overshoot <= last_overshoot ) // Keeps being too short
	        && angle_was_optimized
	) {

		// 2) Assume a hill in the path
		pow_mod = ( std::abs( pow_mod ) + std::abs( last_pow_mod ) ) / 2; // raise it
		ang_mod = ROUND( ( std::abs( last_ang_mod ) + std::abs( ang_mod ) ) * ai_over_mod * SIGNd( last_ang_mod ) * -1. );
		// Note: This accumulates the last and the current angle modification,
		//       strengthens depending on AI level and ensures it has the
		//       opposite direction from the last modification.

		// Make sure the new ang_mod really gets the angle upwards:
		if ( SIGN( curr_angle - 180 ) == SIGN( ang_mod ) ) {
			ang_mod *= -1;
		}

		// Make sure the new ang_mod doesn't make the angle to flip over:
		if ( ( curr_angle > 180 ) && ( ( curr_angle + ang_mod ) <= 180 ) ) {
			ang_mod = 181 - curr_angle;
		}
		if ( ( curr_angle < 180 ) && ( ( curr_angle + ang_mod ) >= 180 ) ) {
			ang_mod = curr_angle - 181;
		}

		DEBUG_LOG_AIM( player->get_name(), "Assuming hill crash, reverting ang_mod to %d", ang_mod )

		last_was_better = false; // false, so this change won't get directly reverted again.
		hill_detected   = true;
	} else if ( last_score && ( last_score > hit_score ) ) {
		// 3) Wrong direction!
		if ( last_was_better ) {

			DEBUG_LOG_AIM( player->get_name(), " => Worse score %d [%d was better]", hit_score, last_score )

			// Try a mod in between the two
			if ( std::abs( last_ang_mod ) > 1 ) {
				ang_mod = -1 * ( last_ang_mod / 2 );
			} else {
				ang_mod = -1 * SIGN( last_ang_mod );
			}
			if ( std::abs( last_pow_mod ) > 9 ) {
				pow_mod = -1 * ( last_pow_mod / 2 );
			} else {
				pow_mod = -5 * SIGN( last_pow_mod );
			}
		} else {
			// b) Revert and go in the opposite direction:

			DEBUG_LOG_AIM( player->get_name(), " => Worse score %d [%d was worse]", hit_score, last_score )

			// If the last was reverted already, strengthen the
			// move in the opposite direction
			if ( last_reverted ) {
				// First make positive and strengthen
				ang_mod = std::abs( ang_mod ) * ( RAND_AI_0P + 1 );
				pow_mod = std::abs( pow_mod ) * ( RAND_AI_1P + 1 );
			}

			// Then strengthen the last values by the current and revert:
			ang_mod = -1 * ( last_ang_mod + ( SIGN( last_ang_mod ) * ang_mod ) );
			pow_mod = -1 * ( last_pow_mod + ( SIGN( last_pow_mod ) * pow_mod ) );

			// Adapt by overshoot: (Yes, still necessary)
			if ( SIGN( curr_overshoot ) == SIGN( pow_mod ) ) {
				pow_mod *= -1;
			}
		}

		last_was_better = false;
	} else {
		// 4) Just do the set mod according to overshoot

		DEBUG_LOG_AIM( player->get_name(), "=> Same score %d, overshoot %d", hit_score, curr_overshoot )

		// Put in some limits for the angle according to where the opponent is
		auto ang_limit = ROUND( focus_rate * static_cast< double >( get_rand() % 16 ) );
		auto dist_y    = ROUND( mem_curr->opY - y );
		if ( dist_y < -100 ) {       /* above */
			ang_limit += 60;
		} else if ( dist_y > 100 ) { /* below */
			ang_limit += 20;
		} else {                     /* equal */
			ang_limit += 40;
		}

		// If a hill was detected, do not modify angle more than 1
		if ( hill_detected && ( ang_mod > 1 ) ) {
			ang_mod = 1;
		}

		if ( ( ( curr_angle <= 180 ) && ( curr_angle > ( 90 + ang_limit ) ) ) || (( curr_angle > ( 270 - ang_limit ) )) ) {
			ang_mod *= -1;
		}

		// Adapt pow_mod by overshoot
		pow_mod         = std::abs( pow_mod ) * SIGN( curr_overshoot ) * -1;

		last_was_better = false;
	}
}

/// @brief adapt @a ang_mod and @a pow_mod when a shot using them did not
/// finish.
void CAICore::fix_unfinished( int32_t& ang_mod, int32_t& pow_mod ) const {
	// Put in some limits for the angle according to where the opponent is
	auto ang_limit = ROUND( focus_rate * static_cast< double >( get_rand() % 16 ) );
	auto dist_y    = ROUND( mem_curr->opY - y );
	if ( dist_y < -100 ) {       /* above */
		ang_limit += 60;
	} else if ( dist_y > 100 ) { /* below */
		ang_limit += 20;
	} else {                     /* equal */
		ang_limit += 40;
	}

	// If a hill was detected on the path, do not alter the angle
	// more than by 1
	if ( hill_detected && ( ang_mod > 1 ) ) {
		ang_mod = 1;
	}

	// If the angle is too steep to the right, or too flat
	// to the left, make ang_mod negative:
	// Note: If the shot is to the right, it is in the range
	// 90-180 and going down needs a negative mod.
	// If going to the left it needs a positive mod to go
	// down as it is in the range 180 - 270.

	if ( ( ( curr_angle <= 180 ) && ( curr_angle > ( 90 + ang_limit ) ) ) || (( curr_angle > ( 270 - ang_limit ) )) ) {
		ang_mod *= -1;
	}

	// The power must be reduced if it is greater than twice the
	// x distance, but raised if less than the simple x distance.
	// If the power is too low, shots can quickly end up with too
	// many bounces if the wall is rubber or spring.
	auto dist_x = ROUND( std::abs( mem_curr->opX - x ) );
	if ( curr_power > ( 2 * dist_x ) ) {
		pow_mod *= -1;
	} else if ( curr_power > dist_x ) {
		pow_mod = 0; // Do not change
	}
}

/// @return true once the operator() ends
bool CAICore::has_exited() const {
	return is_finished;
}

/** @brief Signal the AI that the tank was moved
 *
 * The direction signals the following:
 *
 * < 0 : Moved to the left
 * = 0 : Movement not possible
 * > 0 : Moved to the right.
 *
 * @param[in] direction indicate movement direction
 **/
void CAICore::has_moved( int32_t direction ) {
	if ( direction ) {
		is_moved_by.store( direction, ATOMIC_WRITE );
		can_move.store( true, ATOMIC_WRITE );
	} else {
		is_moved_by.store( 0, ATOMIC_WRITE );
		can_move.store( false, ATOMIC_WRITE );
	}
}

/// @brief initialize work with the current players data
bool CAICore::initialize() {
	DEBUG_LOG_AI( player->get_name(), "Starting think work, setting up.", 0 )

	/// === Step 1 : Copy relevant data ===
	ai_level    = static_cast< int32_t >( player->type );
	ai_level_d  = static_cast< double >( ai_level );
	ai_over_mod = 1. + ( ai_level_d / 10. ); // [1.1;1.5]
	ai_type_mod = ( 1. + ai_level_d ) / 2.;  // [1.0;3.0]
	blast_min   = 0.;
	blast_med   = 0.;
	blast_big   = 0.;
	blast_max   = 0.;
	can_move.store( true, ATOMIC_WRITE );
	is_moved_by.store( 0, ATOMIC_WRITE );
	is_shocked    = false;
	revengee      = nullptr;
	shocker       = nullptr;
	need_success  = true;
	need_aim      = true;
	is_blocked    = false;
	hill_detected = false;

	// Data from player:
	need_money = ( ( player->get_money_to_save( false ) - player->money ) > 0 );
	last_opp   = player->get_opp_mem( -1 );

	// Data from tank:
	tank = player->tank;
	if ( tank && !tank->destroy ) {
		angle     = tank->a;
		power     = tank->p;
		weap_idx  = tank->cw;
		buried    = tank->how_buried( &buried_l, &buried_r );
		curr_life = tank->l + tank->sh;
		max_life  = tank->get_max_life();
		x         = tank->x;
		y         = tank->y;
		last_ang  = 180;
		last_pow  = 1'000;
		last_weap = 0;

		// Is there a last opponent?
		if ( last_opp ) {
			last_ang  = angle;
			last_pow  = power;
			last_weap = weap_idx;
			DEBUG_LOG_AI( player->get_name(), "Last opponent was %s", last_opp->opponent->get_name() )
		}

		// Select last weapon/item used
		if ( weap_idx > WEAPONS ) {
			use_item( weap_idx );
		} else {
			use_weapon( weap_idx );
		}
	} else {
		return false;
	}

	// reset calculation values
	curr_angle = angle;
	curr_power = power;

	// Reset setup values:
	best_round_score     = NEUTRAL_ROUND_SCORE;
	best_setup_angle     = angle;
	best_setup_damage    = 0;
	best_setup_item      = nullptr;
	best_setup_mem       = nullptr;
	best_setup_overshoot = MAX_OVERSHOOT;
	best_setup_power     = power;
	best_setup_score     = NEUTRAL_ROUND_SCORE;
	best_setup_weap      = nullptr;


	/// === Step 2: See whether this bot gets lucky ===
	if ( ( get_rand() % 100 ) < ai_level ) {
		// So the useless bot has a 1% and the deadly bot a 5% chance
		int32_t raise = 1 + ( ( 5 - ai_level ) / 2 );
		/* Useless: 1 + ((5 - 1) / 2) =>  1 + (4 / 2) => 1 + 2 => 3
		 * Guesser: 1 + ((5 - 2) / 2) =>  1 + (3 / 2) => 1 + 1 => 2
		 * Ranger : 1 + ((5 - 3) / 2) =>  1 + (2 / 2) => 1 + 1 => 2
		 * Target : 1 + ((5 - 4) / 2) =>  1 + (1 / 2) => 1 + 0 => 1
		 * Deadly : 1 + ((5 - 5) / 2) =>  1 + (0 / 2) => 1 + 0 => 1
		 */
		DEBUG_LOG_AI(
			player->get_name(),
			"Lucky Turn: Raise from \"%s\" to \"%s\"",
			getLevelName( ai_level ),
			getLevelName( ai_level + raise )
		)
		ai_level    += raise;
		ai_level_d   = static_cast< double >( ai_level );
		ai_over_mod  = 1. + ( ai_level_d / 10. ); // [1.1;1.5]
		ai_type_mod  = ( 1. + ai_level_d ) / 2.;  // [1.0;3.0]
		show_feedback( "*lucky*", GREEN, -.8, TS_NO_SWAY, 100 );
	}


	/// === Step 3 : Set stage and allow the work to be done ===
	if ( tank && !tank->destroy && !is_stopped && allow_work && get_memory() ) {
		// Note: Without the memory, no real work is possible.

		text_allowed.store( true );
		pl_stage   = PS_SELECT_TARGET;
		is_working = true;
		return true;
	}

	return false;
}

/// @brief Sanitize curr_angle and curr_power.
void CAICore::sanitize_curr() {
	if ( curr_angle < 90 ) {
		curr_angle = 90;
	}
	if ( curr_angle > 270 ) {
		curr_angle = 270;
	}
	if ( curr_power > MAX_POWER ) {
		curr_power = MAX_POWER;
	}
	if ( curr_power < MIN_POWER ) {
		curr_power = MIN_POWER;
	}
	curr_power -= curr_power % 5;
}

/// @brief show ai feedback if allowed and not skipping computer play.
/// Whenever a feedback message is shown, the AI sleeps for dur/10 + 1 ms.
void CAICore::show_feedback( char const* const feedback, int32_t col, double yv, ETextSway text_sway, int32_t dur ) const {
	if ( env.show_ai_feedback && !global.skipping_computer_play ) {
		// Wait for the AI to be allowed to create texts
		while ( !text_allowed.load( ATOMIC_READ ) ) {
			std::this_thread::yield();
		}

		auto y_pos = ROUND( y - ( 50. + ( get_rand() % 21 ) ) );
		new CFloatText( feedback, x, y_pos, .0, yv, col, CENTRE, text_sway, dur, false );
		MSLEEP( ( dur / 10 ) + 1 );
	}
}

/** @brief Select the next item to use on the current target.
 *
 * This method tries to determine the best item / weapon selection
 * to be used on the currently selected target (mem_curr).
 *
 * Some of the thinking depend on random numbers, so calling this
 * method twice on the same target might lead to different selections.
 *
 * This is wanted, so many tries on higher ai levels with a small
 * number of difficult to reach targets might eventually lead to
 * a sane result.
 *
 * The selected item / weapon is saved in item_curr or weap_curr. The
 * method makes sure that it is not the same as item_last / weap_last.
 * Please note, however, that if there is only one selectable item,
 * if the bot is out of stock of everything but small missiles for example,
 * then item_curr / weap_curr might end up the same as the last selections.
 *
 * The method returns true if the selection it ends up with makes sense.
 * If @a is_last is set to true, the method itself returns true, too.
 *
 * @param[in] is_last If set to true, the method will return true in any case.
 * @return true if the selection makes sense, or if @a is_last is set to true.
 **/
bool CAICore::select_item( bool is_last ) {
	// Back up current selections
	item_last = item_curr;
	weap_last = weap_curr;

	// Advance to the next weapon
	bool has_weap = true;

	// If a best setup with primary target hit has been achieved
	// already, or if the bot is shocked, select a random weapon.
	// Otherwise do an ordered advance down the chain.
	if ( best_setup_prime || is_shocked ) {
		int32_t weap_num = get_rand() % WEAPONS;
		weap_curr        = weap_head;

		// If weap_last was not set, set it to head, too.
		if ( !weap_last ) {
			weap_last = weap_curr;
		}

		// Now rotate until the weapon is found.
		while ( weap_num ) {
			weap_curr = weap_curr->next;

			// Skip not available weapons, non-damage entries, the last weapon
			// and weapons with a negative score.
			while ( weap_curr
			        && ( ( weap_curr->amount <= 0 ) || ( weap_curr->dmg_single < 2. ) || ( weap_curr->score <= 0 )
			             || ( weap_curr == weap_last )
			             // Reducer and dirt weapons are non-damage, too.
				     // they have a fake damage set, so filter them here.
			             || ( REDUCER == weap_curr->type )
			             || ( ( DIRT_BALL <= weap_curr->type ) && ( SMALL_DIRT_SPREAD >= weap_curr->type ) ) ) ) {
				weap_curr = weap_curr->next;
			}

			// Rotate if the end was hit
			if ( !weap_curr ) {
				weap_curr = weap_head;
			}

			--weap_num;
		}
	} else {
		while ( has_weap
		        && ( !weap_curr || ( weap_curr == weap_last ) || ( weap_curr->amount <= 0 ) || ( weap_curr->dmg_single < 2. )
		             || ( weap_curr->score <= 0 )
		             || ( mem_curr->hasRepulse && RAND_AI_1P && ( SML_NAPALM <= weap_curr->type )
		                  && ( LRG_NAPALM >= weap_curr->type ) && RAND_AI_1P ) ) ) {
			weap_curr = weap_curr ? weap_curr->next : weap_head;

			// If no weapon was selected at the start, weap_last is
			// now nullptr, but must be weap_head once weap_curr is
			// beyond head.
			if ( !weap_last && ( weap_curr != weap_head ) ) {
				weap_last = weap_head;
			}

			// If this rotated once through everything, there is
			// only this one weapon left or a lot of tries are through:
			if ( weap_last == weap_curr ) {
				has_weap = false;
			}
		}
	} // end of ordered rotation

	// Use weap_head if there is no other weapon:
	if ( !has_weap ) {
		weap_curr = weap_head;
		weap_last = weap_curr;
	}

	// If the bot is shocked, the next weapon is selected,
	// someone in panic does not do much thinking any more
	if ( is_shocked ) {
		item_curr = nullptr;
		if ( nullptr == weap_curr ) {
			weap_curr = weap_head;
		}

		weap_idx = weap_curr->type;

		DEBUG_LOG_EMO( player->get_name(), "(SHOCKED) Quick selected %s", weapon[weap_idx].get_name() )
		return true;
	}

	// Advance to the next item
	bool has_item = true;
	while ( has_item
	        && ( !item_curr || ( item_curr == item_last ) || ( item_curr->amount <= 0 ) || !item_curr->selectable
	             || ( item_curr->score <= 0 ) ) ) {
		item_curr = item_curr ? item_curr->next : item_head;

		// If no item was selected at the start, item_last is
		// now nullptr, but must be item_head once item_curr is
		// beyond head.
		if ( !item_last && ( item_curr != item_head ) ) {
			item_last = item_head;
		}

		// If this rotated once through everything, there is
		// only this one weapon left:
		if ( item_last == item_curr ) {
			has_item = false;
		}
	}

	// If no items are available, it has to be taken out of consideration:
	if ( !has_item ) {
		item_curr = nullptr;
		item_last = nullptr;
	}


	// Do not use items with a negative score, as those are items
	// that are unavailable.
	if ( item_curr && ( ( item_curr->score < 0 ) || ( player->ni[item_curr->type] <= 0 ) ) ) {
		item_curr = nullptr;
	}

	// Note: sub-optimal weapons ( too low damage ) can be negative.

	// Do not use self destruct items/weapons unless the
	// bot wants to self destruct
	if ( mem_curr->opLife <= ( curr_life * 10. ) ) {
		if ( item_curr && ( item_curr->kamikaze ) ) {
			item_curr = nullptr;
		}
		if ( weap_curr && ( weap_curr->kamikaze ) ) {
			weap_curr = nullptr;
		}
	}

	// Do not use teleporters unless buried or blocked
	if ( item_curr && ( item_curr->type >= ITEM_TELEPORT ) && ( item_curr->type <= ITEM_MASS_TELEPORT ) && !is_blocked
	     && !item_curr->escape ) {
		item_curr = nullptr;
	}

	// Do not use riot bombs if the path is not blocked
	if ( weap_curr && ( weap_curr->type >= RIOT_BOMB ) && ( weap_curr->type <= HVY_RIOT_BOMB ) && !is_blocked ) {
		weap_curr = nullptr;
	}

	// If both are still set, take what has the higher score
	if ( item_curr && weap_curr ) {
		if ( item_curr->score > weap_curr->score ) {
			weap_curr = nullptr;
			weap_idx  = item_curr->type + WEAPONS;
		} else {
			item_curr = nullptr;
			weap_idx  = weap_curr->type;
		}
	} else if ( item_curr ) {
		weap_idx = item_curr->type + WEAPONS;
	} else if ( weap_curr ) {
		weap_idx = weap_curr->type;
	} else {
		weap_idx = -1;
	}

	if ( !item_curr && !weap_curr && is_last ) {
		// If nothing is set but this is the last chance,
		// use the small missile as a fallback weapon
		use_weapon( SML_MIS );
	}


	DEBUG_LOG_EMO(
		player->get_name(),
		"Next selection: %s",
		weap_curr   ? weapon[weap_idx].get_name()
		: item_curr ? item[weap_idx - WEAPONS].get_name()
		            : "NOTHING (fail)"
	)

	return ( item_curr || weap_curr );
}

/** @brief Select the next target to try  to hit or handle.
 *
 * This method selects and sets the current target. Basically it
 * just wanders down the memory chain as it is sorted by score already.
 *
 * The following additional rules (besides the ordering by score) apply:
 *   - If the bot is shocked, the shocker is always selected.
 *   - If a revenge is sought, that opponent is always selected if it
 *     is not currently selected or was the last one.
 *   - If @a is_last is set to true, the target with the highest score
 *     or that is sought revenge against is selected and the method
 *     returns true.
 *
 * Whenever the selection makes sense, the method returns true.
 *
 * @param[in] is_last If set to true, the method will return true in any case.
 * @return true if the selection makes sense, or if @a is_last is set to true.
 **/
bool CAICore::select_target( bool is_last ) {
	// Be quickly done if this bot is shocked
	if ( is_shocked ) {

		// Is the shocker still there?
		if ( shocker->opponent->tank && !shocker->opponent->tank->destroy ) {
			mem_last = mem_curr;
			if ( !mem_curr || ( mem_curr->entry != shocker ) ) {
				mem_curr = mem_head;
				while ( mem_curr && ( mem_curr->entry != shocker ) ) {
					mem_curr = mem_curr->next;
				}
			}

			DEBUG_LOG_EMO( player->get_name(), "(SHOCKED) Targetting %s", mem_curr ? mem_curr->entry->opponent->get_name() : "NONE?" )

			return ( mem_curr->entry == shocker );
		} else {
			// Nope, gone with the wind.
			shocker    = nullptr;
			is_shocked = false;
		}
	}

	// Preselect the revengee if not done already and there is one:
	// Note: Of course the revengee is forced if this is the very last try!
	if ( revengee
	     && ( is_last || ( ( !mem_curr || ( mem_curr->entry != revengee ) ) && ( !mem_last || ( mem_last->entry != revengee ) ) ) ) ) {

		// is the revengee still alive?
		if ( revengee->opponent->tank && !revengee->opponent->tank->destroy ) {
			mem_last = mem_curr;
			mem_curr = mem_head;
			while ( mem_curr && ( mem_curr->entry != revengee ) ) {
				mem_curr = mem_curr->next;
			}

			DEBUG_LOG_EMO( player->get_name(), "(REVENGE) Targetting %s", mem_curr ? mem_curr->entry->opponent->get_name() : "NONE?" )

			return ( mem_curr->entry == revengee );
		} else {
			// No longer relevant...
			revengee = nullptr;
		}
	}

	// If nothing was preselected, a simple walk down the chain
	// is in order. (revengees must be skipped, though)
	// However, if this is the last_try, the primary target is always selected.
	sOppMemEntry* mem_old = mem_curr; // backup

	if ( is_last || ( nullptr == mem_curr ) ) {
		mem_curr = mem_head;
	}

	// If the revengee is currently selected, the "walk" must continue
	// from the last opponent on, or the bot will have a flip between
	// the revengee and the first other opponent only.
	if ( !is_last && revengee && mem_curr && ( mem_curr->entry == revengee ) ) {
		if ( mem_last ) {
			mem_curr = mem_last;
		} else {
			mem_curr = mem_head;
		}
	}

	// Now walk down the list skipping the revengee if set
	if ( !is_last ) {
		while ( mem_curr
		        && ( ( mem_curr == mem_old ) || ( mem_curr == mem_last ) || ( revengee && ( mem_curr->entry == revengee ) )
		             || ( mem_curr->entry->opponent == player ) || ( nullptr == mem_curr->entry->opponent->tank )
		             || mem_curr->entry->opponent->tank->destroy ) ) {
			mem_curr = mem_curr->next;
		}
		mem_last = mem_old;
	}

	// If is_last is set, mem_curr must not be nullptr. Otherwise a
	// nullptr can happen if too few tanks are left.
	assert( ( !is_last || mem_curr ) && "ERROR: Is last but nullptr curr!" );

	DEBUG_LOG_EMO( player->get_name(), "( normal) Targetting %s", mem_curr ? mem_curr->entry->opponent->get_name() : "nobody" )

	return ( nullptr != mem_curr );
}

/** @brief setup the basic attack values
 *
 * This method tries to find a sane target-weapon-combination.
 * The number of tries to do so is dictated by the AI level,
 * and if this is the very last targeting try, the method will
 * come up with the minimum possible combination.
 *
 * @param[in] is_last If set to true, something usable is forced to be set.
 * @return true if a viable combination was found, false otherwise.
 **/
bool CAICore::setup_attack( bool is_last, int32_t& opp_attempt, int32_t& weap_attempt ) {
	bool selectDone     = false;
	bool breakUp        = false;
	bool has_new_target = false;

	while ( is_working && !is_stopped && !selectDone && !breakUp ) {

		// Yield on each iteration to not hog the CPUs
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		// 1) Select a target
		//====================
		if ( !weap_attempt || !mem_curr ) {
			pl_stage = PS_SELECT_TARGET;

			DEBUG_LOG_AIM( player->get_name(), "Selecting target, try %d / %d", opp_attempt + 1, findOppAttempts )

			selectDone = select_target( ( ++opp_attempt == findOppAttempts ) && is_last && need_success );
			if ( selectDone && ( mem_curr != mem_last ) ) {
				best_round_score = NEUTRAL_ROUND_SCORE; // New target!
				item_curr        = nullptr;
				item_last        = nullptr;
				weap_curr        = nullptr;
				weap_last        = nullptr;
				has_new_target   = true;
			}
		} else {
			has_new_target = false;
			selectDone     = true;
		}

		// 2) Select an item / weapon
		//============================
		if ( selectDone ) {
			pl_stage = PS_SELECT_WEAPON;

			/* --- Ensure dedicated lists and starting values --- */
			if ( has_new_target ) {
				check_weap_mem();
				check_item_mem();
				weap_idx = -1;
			}

			/* --- Now do the selection --- */
			selectDone = false;
			while ( is_working && !is_stopped && !selectDone && ( weap_attempt < findWeapAttempts ) ) {
				DEBUG_LOG_AIM( player->get_name(), "Selecting item, try %d / %d", weap_attempt + 1, findWeapAttempts )

				// Yield on each iteration to not hog the CPUs
				if ( !global.skipping_computer_play ) {
					std::this_thread::yield();
				}

				selectDone = select_item( ( ++weap_attempt == findWeapAttempts ) && is_last && need_success );
			}

			// select_item() must have set weap_idx properly:
			assert( ( !selectDone || ( item_curr && ( weap_idx == WEAPONS + item_curr->type ) )
			          || ( weap_curr && ( weap_idx == weap_curr->type ) ) )
			        && "ERROR: Selection done but weap_idx does not" && " contain the correct index!" );

			// If this was the last attempt to select a weapon, and another
			// opponent selection is in order, reset weap_attempt so a new
			// opponent can be selected
			if ( ( weap_attempt == findWeapAttempts ) && ( opp_attempt < findOppAttempts ) ) {
				weap_attempt = 0;
			}
		} // end of item selection

		// break up this round if no selection was done but the
		// opponent selection has ended
		if ( !selectDone && ( opp_attempt >= findOppAttempts ) && ( weap_attempt >= findWeapAttempts ) ) {
			breakUp = true;
		}
	} // end of basic setup cycle

	// If the selection was not done properly (not possible) but this
	// is the absolutely final round and there has been no good setup, yet,
	// an emergency plan is used:
	if ( is_working && !is_stopped && is_last && !selectDone && need_success ) {
		// Try to "get out" first:
		selectDone = use_item( ITEM_TELEPORT );

		if ( !selectDone && !mem_curr->is_buried ) {
			selectDone = use_item( ITEM_SWAPPER );
		}

		if ( !selectDone ) {
			selectDone = use_item( ITEM_MASS_TELEPORT );
		}

		// If this still isn't going anywhere, revert to first target with
		// small missiles, that might be useless now, but there is always
		// a next round.
		if ( !selectDone ) {
			item_curr  = nullptr;
			mem_curr   = mem_head;
			selectDone = use_weapon( SML_MIS );

			DEBUG_LOG_EMO(
				player->get_name(),
				"Last Try Selection: %s against %s",
				weapon[SML_MIS].get_name(),
				mem_head->entry->opponent->get_name()
			)

			assert( selectDone && "ERROR: Not even small missile can be selected?" );
		} else {
			weap_curr = nullptr; // Emergency plan working.
		}
	}


	// The last thing to consider is kamikaze.
	// While the appropriate items *are* self destruct devices, choosing
	// a weapon with kamikaze potential (and score) does not mean that the
	// bot *must* destroy themselves. So in that case an extra test is in order.
	if ( selectDone && ( ( item_curr && item_curr->kamikaze ) || ( weap_curr && weap_curr->kamikaze ) ) ) {
		bool self_destruct = true;

		// self_preservation is a value in the interval [0;3]
		if ( weap_curr && ( ( get_rand() % 35 ) < ( player->self_preservation * 10. ) ) ) {
			self_destruct = false;
		}

		// Another "way out" is if a weapon shall be used but it does
		// not do enough damage:
		if ( self_destruct && weap_curr && ( weap_curr->dmg_single < curr_life ) ) {
			self_destruct = false;
		}

		// do not self destruct if a good best setup was already found
		if ( !need_success && best_setup_prime && ( best_setup_score > 0 ) ) {
			self_destruct = false;
		}

		// If the bot still wants to self destruct, change mem_curr
		// to reflect this:
		if ( self_destruct && ( mem_curr->entry->opponent != player ) ) {
			mem_last = mem_curr;
			mem_curr = mem_head;
			while ( mem_curr->entry->opponent != player ) {
				mem_curr = mem_curr->next;
			}

			// This must never fail:
			assert( mem_curr && "ERROR: Self not found in memory?" );

			DEBUG_LOG_EMO(
				player->get_name(),
				"Chosen to self destruct using %s",
				weap_curr   ? weapon[weap_idx].get_name()
				: item_curr ? item[weap_idx - WEAPONS].get_name()
				            : "NOTHING (fail)"
			)

		} else {
			// To chicken out, the small missile is chosen if this is
			// the last thing to consider
			if ( is_last && need_success ) {
				use_weapon( SML_MIS );
			} else {
				selectDone = false;
			}
		}
	}


	// If both attempts, selecting an opponent and a weapon, have reached
	// the maximum tries, they get reset, so the next full targeting cycle
	// is triggered:
	if ( !is_last && breakUp ) {
		opp_attempt  = 0;
		weap_attempt = 0;
	}

	return selectDone;
}

/** @brief start the work on one player
 *
 * This method starts working on a new player. All relevant
 * data is fetched from @a player_ that must not be nullptr.
 *
 * If a job is running, no new one is started.
 * If @a player_ is not an AI player, it is not handled.
 *
 * @param[in] player_ Pointer to the player to handle.
 * @return true if a job was started, false otherwise.
 */
bool CAICore::start( CPlayer* player_ ) {
	if ( allow_work && player_ && !is_working && !is_stopped && ( player_->type > HUMAN_PLAYER ) && ( player_->type < NETWORK_CLIENT )
	     && ( PS_AI_IS_IDLE == pl_stage ) ) {

		DEBUG_LOG_AI( player_->get_name(), "==============================", 0 )
		DEBUG_LOG_AI( player_->get_name(), " CAICore started for %s", player_->get_name() )
		DEBUG_LOG_AI( player_->get_name(), "------------------------------", 0 )

		lguard_t guard( action_mutex );
		pl_stage   = PS_AI_INITIALIZE;
		player     = player_;
		is_working = true;
		action_condition.notify_one();

		return true;
	}

	return false;
}

/** @brief Retrieve the current status of the AI
 *
 * The arguments will not be changed if the AI is not working
 * on any player.
 *
 * @param[out] a_item Receives the currently selected weapon/item
 * @param[out] a_angle Receives the currently set angle
 * @param[out] a_power Receives the currently set power
 * @param[out] ai_stage Receives the current stage of the AI. This is always sent.
 * @return true if the AI is still working, false if it has finished.
 */
bool CAICore::status( int32_t& a_item, int32_t& a_angle, int32_t& a_power, EPlayerStages& ai_stage ) const {
	ai_stage = this->pl_stage;

	if ( is_working ) {
		a_item  = weap_idx;
		a_angle = angle;
		a_power = power;
		return true;
	}

	return false;
}

/// @brief Tell the thread to stop even if it is not finished.
void CAICore::stop() {
	lguard_t guard( action_mutex );
	is_stopped = true;
	action_condition.notify_one();
}

/** @brief Trace the sub munition of a cluster type weapon
 *
 * Damage is recorded in the opponent memory dmg_done value.
 *
 * @param[in] sub_type The type of the sub munition. No checks done!
 * @param[in] sub_count The number of sub munition parts.
 * @param[in] sub_x Trigger x coordinate.
 * @param[in] sub_y Trigger y coordinate.
 * @param[in] inh_xv Parent missile xv the moment it triggered.
 * @param[in] inh_yv Parent missile yv the moment it triggered.
 **/
void CAICore::trace_cluster( int32_t sub_type, int32_t sub_count, int32_t sub_x, int32_t sub_y, double inh_xv, double inh_yv ) {
	double    divergence    = weapon[weap_curr->type].divergence;
	double    speedVar      = weapon[weap_curr->type].speedVariation;
	double    spreadVar     = weapon[weap_curr->type].spreadVariation;
	CWeapon*  sub_weap      = &weapon[sub_type];
	double    divStep       = static_cast< double >( divergence ) / static_cast< double >( sub_count - 1 );
	double    startPoint    = divStep < 0. ? 0. : 180.;
	int32_t   randStart     = get_rand() % 1'000'000;
	EPhysType subPhys       = PT_NORMAL;
	int32_t   start_y       = sub_y - 20;
	int32_t   cl_overshoot  = MAX_OVERSHOOT;
	int32_t   old_overshoot = curr_overshoot; // overshoot is only used for mirvs and funkies
	double    radius        = sub_weap->radius;
	double    sub_dmg       = sub_weap->damage * player->damage_multiplier;

	// If the weapon is fired into a ceiling, adapt starting y
	if ( env.is_boxed && ( start_y <= BOXED_TOP )
	     && ( ( WALL_STEEL == env.current_wall_type )
	          || ( ( WALL_WRAP == env.current_wall_type ) && ( !env.is_boxed || !env.do_box_wrap ) ) ) ) {
		start_y = MENUHEIGHT + 20;
	}

	// Change physics of the sub munitions for the funky bomb
	if ( ( weap_curr->type == FUNKY_BOMB ) || ( weap_curr->type == FUNKY_DEATH ) ) {
		subPhys = PT_FUNKY_FLOAT;
	}

	// If this is a steel wall hit, the start point angle needs
	// to be adapted. And erased if this is a ceiling hit
	if ( WALL_STEEL == env.current_wall_type ) {
		if ( ( CLUSTER <= weap_curr->type ) && ( SUP_CLUSTER >= weap_curr->type ) ) {
			if ( x < 2 ) {
				startPoint -= divergence + 1 + ( get_rand() % 10 );
			} else if ( x > ( env.screen_width - 3 ) ) {
				startPoint += divergence + 1 + ( get_rand() % 10 );
			} else if ( y <= BOXED_TOP ) {
				startPoint = 0;
			}
		} else if ( ( SML_NAPALM <= weap_curr->type ) && ( LRG_NAPALM >= weap_curr->type ) ) {
			if ( x < 2 ) {
				startPoint -= 10 + get_rand() % 21;
			} else if ( x > ( env.screen_width - 3 ) ) {
				startPoint += 10 + get_rand() % 21;
			} else if ( y <= BOXED_TOP ) {
				startPoint = 0;
			}
		} else if ( ( ( SMALL_MIRV == weap_curr->type ) || ( CLUSTER_MIRV == weap_curr->type ) ) && ( y <= BOXED_TOP ) ) {
			startPoint = 0;
			inh_yv     = std::abs( inh_yv );
		}
	}

	// The spread can be created!
	for ( int32_t sc = 0; sc < sub_count; ++sc ) {
		double  speed        = weapon[weap_curr->type].launchSpeed;
		int32_t newMissCount = sub_weap->countdown;
		auto    newMissAngle = ROUND( ( divStep * sc ) + startPoint - ( divergence / 2. ) );

		// trace hard, but yield per sub mun
		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}

		// Manipulate angle if applicable
		if ( speedVar > 0. ) {
			newMissAngle += ROUND( static_cast< double >( divergence ) * spreadVar * noise( randStart + 1'054 + sc ) );
		}

		// Be sure the angle is valid
		while ( newMissAngle < 0 ) {
			newMissAngle += 360;
		}
		newMissAngle %= 360;

		// Manipulate number of submunition projectiles if applicable
		if ( sub_weap->countVariation > 0 ) {
			newMissCount += ROUND(
				static_cast< double >( sub_weap->countdown ) * sub_weap->countVariation * noise( randStart + 78'689 + sc )
			);
			// This might go wrong, so be sure it doesn't
			if ( newMissCount <= 0 ) {
				newMissCount = 0;
			}
		}

		// Manipulate launching speed if applicable
		if ( speedVar > 0 ) {
			speed += ROUND( speedVar * speed * noise( randStart + 124'786 + sc ) );
		}

		// Launch new submunition missile
		CMissile mind_shot(
			player,
			sub_x,
			start_y,
			env.slope[newMissAngle][0] * speed * env.fps_mod + inh_xv,
			env.slope[newMissAngle][1] * speed * env.fps_mod + inh_yv,
			sub_type,
			MT_MIND_SHOT,
			ai_level,
			0
		);
		mind_shot.update_submun( subPhys, newMissCount );

		// Keep flying/rolling/digging/whatever until the missile hits something
		// or the number of bounces is too high for this bot to keep track.
		while ( !mind_shot.destroy && ( max_bounce >= mind_shot.bounced() ) ) {
			mind_shot.applyPhysics();

			// Yield on each iteration to not hog the CPUs
			if ( !global.skipping_computer_play ) {
				std::this_thread::yield();
			}
		}

		// If the missile is destroyed, the number of bounces is in order.
		if ( mind_shot.destroy ) {
			// The distance from the target must take both the direction
			// of the last movement of the mind shot and the positions of
			// both tanks into account:
			int32_t tank_dir = SIGN( mem_curr->opX - x );
			int32_t hit_dir  = SIGN( mem_curr->opX - mind_shot.x );
			int32_t shot_dir = mind_shot.direction();

			curr_overshoot   = ABSDISTANCE2( mem_curr->opX + offset_x, mem_curr->opY + offset_y, mind_shot.x, mind_shot.y );

			if ( tank_dir == shot_dir ) {
				curr_overshoot *= ( tank_dir == hit_dir ? -1 : 1 );
			} else {
				curr_overshoot *= ( tank_dir == hit_dir ? 1 : -1 );
			}

			// Note it down if this mind shot is better than the best
			if ( std::abs( curr_overshoot ) < std::abs( cl_overshoot ) ) {
				cl_overshoot = curr_overshoot;
			}

			// eventually add the score:
			calc_hit_damage( ROUND( mind_shot.x ), ROUND( mind_shot.y ), radius, sub_dmg, static_cast< EWeaponType >( sub_type ) );
		}
	} // end of looping submunitions

	// Write back best overshoot if this is a MIRV or funky weapon.
	// clusters and napalm weapons spread stuff out, there the hit
	// of the parent weapon is what counts.
	if ( ( SMALL_MIRV == weap_curr->type ) || ( FUNKY_BOMB == weap_curr->type ) || ( FUNKY_DEATH == weap_curr->type )
	     || ( CLUSTER_MIRV == weap_curr->type ) ) {
		curr_overshoot = cl_overshoot;
	} else {
		curr_overshoot = old_overshoot;
	}
}

/** @brief Fire a mind shot and see where it goes.
 *
 * If the mind shot got destroyed, curr_overshoot is set to the distance of
 * mem_curr->opponent to reached_x_/reached_y_. Positive values mean "too far",
 * negative values mean the shot went "too short".
 *
 * @param[in]  trace_angle The angle to use, normally a spread variation of
 *             curr_angle.
 * @param[in]  delay_idx Index of this shot in a delayed shot. Used to simulate
 *             "bombing through" terrain.
 * @param[out] finished true if the mind shot got destroyed before reaching
 *             max_bounce wall bounces/wraps.
 * @param[out] top_wrapped Set to true if the the missile wrapped through a
 *             wrap wall ceiling.
 * @param[out] reached_x_ The x-coordinate on destruction. If the mind shot was
 *             cancelled before being destroyed, @a reached_x_ is not changed.
 * @param[out] reached_y_ The y-coordinate on destruction. If the mind shot was
 *             cancelled before being destroyed, @a reached_y_ is not changed.
 * @param[out] end_xv The x velocity the moment the projectile ends.
 * @param[out] end_yv The y velocity the moment the projectile ends.
 * @return true if all went well, false if any new() operator failed. If this
 *         method returns false, the AI can no longer work.
 **/
bool CAICore::trace_shot(
	int32_t  trace_angle,
	int32_t  delay_idx,
	bool&    finished,
	bool&    top_wrapped,
	int32_t& reached_x_,
	int32_t& reached_y_,
	double&  end_xv,
	double&  end_yv
) {
	double  top_x        = x;
	double  top_y        = y;
	double  vel_mod      = static_cast< double >( curr_power ) * env.fps_mod;
	double  vel_x        = env.slope[trace_angle][0] * vel_mod / 100.;
	int32_t aim_dir      = SIGN( vel_x );
	double  vel_y        = env.slope[trace_angle][1] * vel_mod / 100.;
	bool    can_top_wrap = ( env.is_boxed && ( WALL_WRAP == env.current_wall_type ) && env.do_box_wrap );
	double  old_yv       = 0;
	double  old_y        = 0;

	tank->get_guntop( trace_angle, top_x, top_y );

	CMissile mind_shot( player, top_x, top_y, vel_x, vel_y, weap_idx, MT_MIND_SHOT, ai_level, delay_idx );

	// Adapt missile drag if the player has dimpled/slick projectiles
	if ( player->ni[ITEM_DIMPLEP] ) {
		mind_shot.drag *= item[ITEM_DIMPLEP].vals[0];
	} else if ( player->ni[ITEM_SLICKP] ) {
		mind_shot.drag *= item[ITEM_SLICKP].vals[0];
	}

	// Keep flying/rolling/digging/whatever until the missile hits something
	// or the number of bounces is too high for this bot to keep track.
	while ( !mind_shot.destroy && ( max_bounce >= mind_shot.bounced() ) ) {
		if ( can_top_wrap ) {
			old_yv = vel_y;
			old_y  = mind_shot.y;
		}

		mind_shot.applyPhysics();

		if ( can_top_wrap ) {
			mind_shot.get_velocity( vel_x, vel_y );
			if ( ( ( old_yv < 0. ) && ( vel_y < 0. ) && ( mind_shot.y > old_y ) )
			     || ( ( old_yv > 0. ) && ( vel_y > 0. ) && ( mind_shot.y < old_y ) ) ) {
				top_wrapped = true;
			}
		}

		if ( !global.skipping_computer_play ) {
			std::this_thread::yield();
		}
	}

	// If the missile is destroyed, the number of bounces is in order.
	if ( mind_shot.destroy ) {
		mind_shot.get_velocity( end_xv, end_yv );
		finished   = true;
		reached_x_ = ROUND( mind_shot.x );
		reached_y_ = ROUND( mind_shot.y );

		// The distance from the target must take both the direction
		// of the last movement of the mind shot and the positions of
		// both tanks into account:
		int32_t tank_dir = SIGN( mem_curr->opX - x );
		int32_t hit_dir  = SIGN( mem_curr->opX - reached_x_ );
		int32_t shot_dir = mind_shot.direction();

		// If the shot was flipped, and is therefore shooting into the opposite
		// direction of the target, the distance between the impact and the wall
		// shooting at must be added if the shot went away from the target.
		// However, this only matters if no bounces have been recorded, yet.
		int32_t flip_offset = 0;
		if ( has_flipped && ( aim_dir == shot_dir ) && ( WALL_STEEL != env.current_wall_type ) && ( 0 == mind_shot.bounced() ) ) {
			if ( reached_x_ < x ) {
				flip_offset = 2 * reached_x_;
			} else {
				flip_offset = 2 * ( env.screen_width - reached_x_ );
			}
		}

		curr_overshoot = ABSDISTANCE2( mem_curr->opX + offset_x + flip_offset, mem_curr->opY + offset_y, reached_x_, reached_y_ );

		if ( tank_dir == shot_dir ) {
			curr_overshoot *= ( tank_dir == hit_dir ? -1 : 1 );
		} else {
			curr_overshoot *= ( tank_dir == hit_dir ? 1 : -1 );
		}
	} else {
		finished       = false;
		curr_overshoot = MAX_OVERSHOOT;
	}

	return true;
}

/** @brief trace all shots from weap_curr and fill in the arguments.
 *
 * Damage is recorded in the opponent memory dmg_done value.
 *
 * @return best overshoot.
 **/
void CAICore::trace_weapon( int32_t& has_crashed, int32_t& has_finished ) {
	assert( weap_curr && "ERROR: trace_weapon() with nullptr weap_curr?" );

	int32_t trace_overshoot = MAX_OVERSHOOT;
	int32_t curr_reached_x  = reached_x;
	int32_t curr_reached_y  = reached_y;
	double  end_xv          = 0.;
	double  end_yv          = 0.;

	has_crashed             = 0;
	has_finished            = 0;
	curr_prime_hit          = false;

	// reset virtual damage on opponents.
	opentry_t* opp = mem_head;
	while ( opp ) {
		opp->dmg_done = 0;
		opp           = opp->next;
	}

	// Loop by spread, weapons that do not spread have a value of 1.
	for ( int32_t i = 0; allow_work && !is_stopped && ( i < weap_curr->spread ); ++i ) {
		int32_t tr_a     = curr_angle + ( ( SPREAD * i ) - ( SPREAD * ( weap_curr->spread - 1 ) / 2 ) );
		bool    finished = false;
		bool    top_wrap = false;
		reached_x        = ROUND( x );
		reached_y        = ROUND( y );

		// Loop again by delay, weapons that have no delay default to 1 here.
		for ( int32_t j = 0; allow_work && !is_stopped && ( j < weap_curr->delay ); ++j ) {
			if ( trace_shot( tr_a, j, finished, top_wrap, curr_reached_x, curr_reached_y, end_xv, end_yv ) && finished ) {

				++has_finished;

				// Check whether the shot crashed
				if ( ( env.is_boxed
				       && ( ( curr_reached_y <= BOXED_TOP ) // crashed on top
				                                            // wrapped to bottom with dirt above is a ceiling
				                                            // crash, too.
				            || ( top_wrap && ( ( weap_curr->type < BURROWER ) || ( weap_curr->type > PENETRATOR ) )
				                 && ( curr_reached_y > global.surface[curr_reached_x].load() ) ) ) )
				     // Steel wall have additional wall crashes
				     || ( ( WALL_STEEL == env.current_wall_type ) && ( weap_curr->subMunCount < 1 ) // Clusters
				                                                                                    // never
				                                                                                    // crash
				          && ( ( curr_reached_x <= 2 ) || ( curr_reached_x >= ( env.screen_width - 3 ) ) ) ) ) {

					++has_crashed;
				} // end of omni-crash-check

				if ( weap_curr->subMunCount > 0 ) {
					double inh_xv = weapon[weap_curr->type].impartVelocity * end_xv;
					double inh_yv = weapon[weap_curr->type].impartVelocity * end_yv;
					// Trace the cluster parts and add their score:
					trace_cluster(
						weap_curr->subMunType,
						weap_curr->subMunCount,
						curr_reached_x,
						curr_reached_y,
						inh_xv,
						inh_yv
					);
				} else {
					// Calculate the damage for the hit, crashes are handled later
					calc_hit_damage(
						curr_reached_x,
						curr_reached_y,
						static_cast< double >( weap_curr->radius ),
						weap_curr->dmg_single,
						static_cast< EWeaponType >( weap_curr->type )
					);
				}

				// Note best overshoot if any
				if ( std::abs( curr_overshoot ) < std::abs( trace_overshoot ) ) {
					trace_overshoot = curr_overshoot;
					reached_x       = curr_reached_x;
					reached_y       = curr_reached_y;
				}
			} // end of if trace_shot
		} // end of delay loop
	} // end of spread loop

	// Write back best data found:
	curr_overshoot = trace_overshoot;
}

/// @brief Set a new score to an items entry
void CAICore::update_item_score( itentry_t* pItem ) const {
	/* There aren't many items that are actually usable.
	 * 1. Teleporters
	 *    These can be used to get out of a buried scenario.
	 *    Further they might be an alternative if the
	 *    targeted tank is far away.
	 * 2. Fan
	 *    This item has no real use for the AI but one:
	 *    If the enemy is behind a mountain and the AI has tail wind,
	 *    then it might be helpful to change the wind direction.
	 *    However, this does only make sense if not that many other
	 *    bots have their shot until this one gets its next try.
	 * 3. Self destruct devices
	 *    If this bots tanks is almost dead, and the preferred target
	 *    has a lot of health left, then trying to take them with us
	 *    using a big boom is somewhat compelling.
	 * 4. Fuel and rockets.
	 *    Fuel can be used to get away from a steep wall, rockets can
	 *    be used to get out of a steep canyon.
	 */


	// === Get out quickly if the chosen item is out of stock ===
	// ==========================================================
	if ( 0 == pItem->amount ) {
		pItem->score = -50'000;
		return;
	}


	// === Only evaluate items that are available ===
	// ==============================================
	if ( !env.is_item_available( pItem->type + WEAPONS ) ) {
		pItem->score = -100'000;
		return;
	}

	DEBUG_LOG_AI( player->get_name(), "Evaluating score for %s", item[pItem->type].get_name() )

	// reset helper boolean
	pItem->escape = false;


	/* -------------------------------------------------------------
	 * --- 1) Set score for freeing capabilities while buried    ---
	 * ------------------------------------------------------------- */
	double unbury_score = 0;
	if ( buried > BURIED_LEVEL ) {
		double bury_diff = buried - BURIED_LEVEL;
		double off_mod   = ( player->defensive - 1.5 ) / -2.;

		if ( ITEM_TELEPORT == pItem->type ) {
			unbury_score =
				bury_diff * ai_level
			        * off_mod
			        // It is more valuable when the target is buried,
				// as it is not desirable to swap with them
			        * ( mem_curr->is_buried ? 3. : 1. );
		}

		else if ( ITEM_SWAPPER == pItem->type ) {
			unbury_score =
				bury_diff * ai_level * off_mod
			        * 2.
			        // If the target is buried, the swapper is no good.
			        * ( mem_curr->is_buried ? -50. : 2. );
		}

		else if ( ITEM_MASS_TELEPORT == pItem->type ) {
			unbury_score = bury_diff * ai_level * off_mod * 1.25;
		}

		else if ( ITEM_FAN == pItem->type ) {
			// at least the bot might think the usage is safe.
			unbury_score = bury_diff * static_cast< double >( maxAiLevel + 1 - ai_level ) * off_mod / 2.;
		} else {
			// Everything else is useless
			unbury_score = -5000.;
		}

		// "escape" tool ?
		if ( ( ITEM_TELEPORT <= pItem->type ) && ( ITEM_MASS_TELEPORT >= pItem->type ) ) {
			pItem->escape = true;
		}
	}


	/* -------------------------------------------------------------
	 * --- 2) The wind direction change score for fans           ---
	 * ------------------------------------------------------------- */
	double fan_score = 0.;
	if ( ( ITEM_FAN == pItem->type )
	     // The fan can only be considered useful if the bot has tail wind:
	     && ( ( ( mem_curr->opX > tank->x ) && ( global.wind > 0. ) ) || ( ( mem_curr->opX < tank->x ) && ( global.wind < 0. ) ) ) ) {

		// First count how many other bots can have their turn until
		// this one will get its next chance:
		opentry_t* check   = mem_head;
		int32_t    between = 0;
		while ( check ) {
			if ( ( check->entry->opponent != player ) && check->alive ) {
				++between;
			}
			check = check->next;
		}

		// Now look whether there is a mountain in between:
		auto    check_x   = ROUND( mem_curr->opX );
		int32_t checked   = 0;
		int32_t direction = SIGN( global.wind ) * -1;
		int32_t range_x   = 10 * ( ai_level + RAND_AI_0P );
		int32_t top_ledge = env.screen_height;

		while ( ( checked < range_x ) && ( check_x > 1 ) && ( check_x < ( env.screen_width - 2 ) ) ) {
			int32_t check_y = global.surface[check_x].load( ATOMIC_READ );
			if ( check_y < top_ledge ) {
				top_ledge = check_y;
			}
			check_x += direction;
		}

		// Now the score is a simple height difference modified by defensiveness
		// (The more defensive the player is, the more it is inclined to prepare
		//  the next attack instead of pissing them of with a weak shot.)
		fan_score = static_cast< double >( top_ledge - mem_curr->opY ) * ( player->defensive + ai_level_d );

		// However, the score is multiplied again with the count of opponents
		// that will have their try until this one gets its next shot.
		fan_score *= fan_score > 0. ? static_cast< double >( ai_level - between ) // normal multiplier
		                            : static_cast< double >( between );           // The more bots, the more useless.

		// However, if the bot already used the fan in the last round,
		// do not repeat, no matter what:
		if ( last_weap == ( ITEM_FAN + WEAPONS ) ) {
			DEBUG_LOG_EMO(
				player->get_name(),
				"=> Reducing fan score %6.2lf to %62lf (no repeat)",
				fan_score,
				std::abs( fan_score * ai_level ) * -1.
			)
			fan_score = std::abs( fan_score * ai_level ) * -1.;
		}
	} // end of calculating a fan score


	/* -------------------------------------------------------------
	 * --- 3) Set score for self destruct probability            ---
	 * ------------------------------------------------------------- */
	double selfde_score = 0.;
	if ( ( mem_curr->opLife > ( curr_life * 10. ) ) || ( is_shocked && ( mem_curr->opLife > ( curr_life * 5. ) ) ) ) {
		if ( ( ITEM_VENGEANCE <= pItem->type ) && ( ITEM_FATAL_FURY >= pItem->type ) ) {
			selfde_score = static_cast< double >( pItem->type - ITEM_VENGEANCE + 1 ) * mem_curr->diff_life
			             / ( player->self_preservation + .5 );
		}
	}


	/* -------------------------------------------------------------
	 * --- 4) The "useless" score, for not usable items          ---
	 * ------------------------------------------------------------- */
	double useless_score = 0.;
	if ( ( ITEM_FATAL_FURY < pItem->type ) && ( ITEM_ROCKET != pItem->type ) ) {
		useless_score = -50000.;
	}


	/* -------------------------------------------------------------
	 * --- 5) Sum up the score                                   ---
	 * --- This will be used for sorting the items list          ---
	 * ------------------------------------------------------------- */
	double pref_score = pItem->preference / static_cast< double >( ai_level * 10 );

	double xScore     = unbury_score + selfde_score + useless_score + fan_score;

	if ( useless_score > -1. ) {
		DEBUG_LOG_EMO( player->get_name(), "  preference   : %6.2lf%s", pref_score, xScore > 1. ? "" : " (ignored)" )
		DEBUG_LOG_EMO( player->get_name(), "  unbury_score : %6.2lf", unbury_score )
		DEBUG_LOG_EMO( player->get_name(), "  fan_score    : %6.2lf", fan_score )
		DEBUG_LOG_EMO( player->get_name(), "  selfde_score : %6.2lf", selfde_score )
		DEBUG_LOG_EMO( player->get_name(), "  useless_score: %6.2lf", useless_score )
	}

	// Only add preferences if there is any use for the item:
	if ( xScore > 1. ) {
		xScore += pref_score;
	}

	pItem->score = ROUND( xScore );

	DEBUG_LOG_EMO( player->get_name(), "  Final Score  : %8d", pItem->score )
}

/// @brief Set a new score to an opponents entry
void CAICore::update_opp_score( opentry_t* pOpp ) {
	sOpponent* entry    = pOpp->entry;
	CPlayer*   opponent = entry->opponent;
	CTank*     oppTank  = opponent->tank;

	DEBUG_LOG_AI( player->get_name(), "Evaluating score for %s", opponent->get_name() )

	/* Quickly handle dead tanks and the own entry */
	if ( ( player == opponent ) || !oppTank || oppTank->destroy ) {
		entry->damage_from += entry->damage_last;
		entry->damage_last  = 0;
		pOpp->score         = ( player == opponent ) ? -2 : -1;

		DEBUG_LOG_AI(
			player->get_name(),
			"%s%s",
			player == opponent ? "" : opponent->get_name(),
			player == opponent ? "Not evaluating myself!" : " is dead and not selectable!"
		)
		return;
	}


	/* -------------------------------------------------------------
	 * --- 1) Set up a fear value (if needed)                    ---
	 * --- This is used to possibly trigger actions that may not ---
	 * --- be wise but are imposed by a sudden surge of fear.    ---
	 * ------------------------------------------------------------- */
	double fear_damage = 0.;
	double fear_shock  = 0.;
	if ( !pOpp->onSameTeam ) {
		entry->fear_shock = 0.;

		// The higher the AI level, the more the taken over fear is
		// reduced. *But* the more defensive the player is, the less
		// it is reduced. (Even more on a lucky turn. ;-)
		// Ranges are from useless full defensive to deadly full offensive:
		// From: 2.5 + 1 - ( 1 + 1) => 3.5 - 2 => 1.5 (only one third reduced)
		// To  : 2.5 + 5 - (-1 + 1) => 7.5 - 0 => 7.5 (~87% taken off)
		entry->fear /= 2.5 + ai_level_d - ( player->defensive + 1.0 );

		// Only add new fear if there was any damage
		if ( entry->damage_last > 0 ) {
			fear_damage  = player->pain_sensitivity * entry->damage_last;
			entry->fear += player->self_preservation;
		}

		// first fear check:
		// If the AI can not stand the pain, the damage done is multiplied
		// with the fear value. This does not trigger any action, yet, but
		// the score will go up a lot.
		fear_shock = entry->fear - static_cast< double >( RAND_AI_0P );
		if ( ( fear_damage > 0. ) && ( fear_shock > 0. ) ) {
			fear_damage *= entry->fear;
			DEBUG_LOG_EMO(
				player->get_name(),
				"%s caused fear shock %lf with damage %u",
				opponent->get_name(),
				fear_shock,
				ROUNDu( fear_damage )
			)
			// Is this the new shocker?
			if ( ( nullptr == shocker ) || ( fear_shock > shocker->fear_shock ) ) {
				DEBUG_LOG_EMO(
					player->get_name(),
					"%s %s%s%s as new shocker",
					opponent->get_name(),
					shocker ? "replaces" : "set",
					shocker ? " " : "",
					shocker ? shocker->opponent->get_name() : ""
				)
				shocker = entry;
			}
		} // end of having a fear shock
		entry->fear_shock = fear_shock;
	} // end of fear value handling

	/* -------------------------------------------------------------
	 * --- 2) Check damage for whether revenge is called for     ---
	 * ------------------------------------------------------------- */
	double revenge_score = 0.;
	if ( ( entry->damage_last > 0 ) && !pOpp->onSameTeam ) {

		// First reduce the current damage accumulated. More for lower level bots.
		if ( !pOpp->revengeDone ) {
			DEBUG_LOG_EMO( player->get_name(), "Current anger damage from %s: %d", opponent->get_name(), entry->revenge_dmg )

			entry->revenge_dmg = ROUND( static_cast< double >( entry->revenge_dmg ) / ( 4.5 - ai_type_mod ) );

			DEBUG_LOG_EMO( player->get_name(), " --> Anger cooled down to   : %d", entry->revenge_dmg )

			// add current damage
			entry->revenge_dmg += entry->damage_last;

			DEBUG_LOG_EMO( player->get_name(), " --> Anger raised again to  : %d", entry->revenge_dmg )

			// Revenge damage handled:
			pOpp->revengeDone = true;
		}

		// Now see whether a new act of vengeance is initiated:
		if ( ( entry->revenge_dmg > ( player->vengeance_threshold * max_life ) ) && ( ( get_rand() % 100 ) <= player->vengeful ) ) {

			// Okay, the potential is there...
			revenge_score = static_cast< double >( entry->damage_last * player->vengeful ) / 100.;
			if ( ( nullptr == revengee ) || ( entry->revenge_dmg > revengee->revenge_dmg ) ) {
				// A new one!
				DEBUG_LOG_EMO(
					player->get_name(),
					" --> [%d] %s %s%s for revenge!",
					entry->revenge_dmg,
					entry->opponent->get_name(),
					revengee ? "replaces " : "is set ",
					revengee ? revengee->opponent->get_name() : ""
				)
				revengee = entry;
			}
		}
	} // end of revenge value handling


	/* -------------------------------------------------------------
	 * --- 3) Check opponents health compared to this tank       ---
	 * --- The more health they got, the more money can be made. ---
	 * --- On the other hand, the bigger the difference, the     ---
	 * --- more impressive they are.                             ---
	 * -------------------------------------------------------------
	 */
	double life_score = 0.;
	if ( pOpp->diff_life < 0. ) {
		// The opponent has more health. This might impress the bot:
		if ( ( get_rand() % static_cast< int32_t >( DEADLY_PLAYER ) ) < ai_level ) {
			// No, there is nothing impressive with that...
			life_score = ( player->defensive - 3. ) / 2. * pOpp->diff_life;
			// Note:
			// Full Defensive : (-1 - 3) / 2 * -x => -4 / 2 * -x => -2 * -x = 2 * x
			// Full Offensive : ( 1 - 3) / 2 * -x => -2 / 2 * -x => -1 * -x = 1 * x

			// If the bot needs money, the opponents health might be added:
			if ( need_money && RAND_AI_0P ) {
				life_score += pOpp->opLife;
			}
		}
	} else {
		// add points for their weakness, more if the bot is offensive
		life_score = ( player->defensive + 3. ) / 2. * pOpp->diff_life;
	}
	// Note:
	// Full Defensive : (-1 + 3) / 2 * x => 2 / 2 * x = 1 * x
	// Full Offensive : ( 1 + 3) / 2 * x => 4 / 2 * x = 2 * x


	/* -------------------------------------------------------------
	 * --- 4) add points for distance                            ---
	 * --- The theory is, that weaker bots concentrate on nearer ---
	 * --- enemies first, while stronger bots do not mind.       ---
	 * ------------------------------------------------------------- */
	double dist_score = ( static_cast< double >( env.half_width ) - pOpp->distance ) / ai_level_d;


	/* -------------------------------------------------------------
	 * --- 5) add points the easier the target is to be killed.  ---
	 * --- The easier, and cheaper, the better. But even much    ---
	 * --- better if this bot needs money.                       ---
	 * ------------------------------------------------------------- */
	double vict_score = 0.;
	double vict_mod   = need_money ? static_cast< double >( 8 - ai_level ) : 1.;
	if ( pOpp->opLife < blast_max ) {
		vict_score += vict_mod * ( blast_max - pOpp->opLife ) * 1. * ai_type_mod;
	}
	if ( pOpp->opLife < blast_big ) {
		vict_score += vict_mod * ( blast_big - pOpp->opLife ) * 2. * ai_type_mod;
	}
	if ( pOpp->opLife < blast_med ) {
		vict_score += vict_mod * ( blast_med - pOpp->opLife ) * 4. * ai_type_mod;
	}
	if ( pOpp->opLife < blast_min ) {
		vict_score += vict_mod * ( blast_min - pOpp->opLife ) * 8. * ai_type_mod;
	}


	/* --------------------------------------------------------------
	 * --- 6) add or dock points regarding AI level               ---
	 * --- More powerful opponents are targeted preferably, while ---
	 * --- weaker ones are not considered to be such a threat.    ---
	 * --- Note: Human players are handled like the best AI       ---
	 * ---       that is present in the game.                     ---
	 * -------------------------------------------------------------- */
	double level_score = 0.;
	if ( !pOpp->onSameTeam ) {
		if ( ( HUMAN_PLAYER < opponent->type ) && ( LAST_PLAYER_TYPE > opponent->type ) ) {
			level_score = static_cast< double >( opponent->type - player->type );
		} else {
			level_score = static_cast< double >( best_type - player->type );
		}

		// The higher the self preservation, the more urgent deadlier
		// bots are targeted to get them down early.
		if ( level_score > 0. ) {
			level_score *= player->self_preservation + 1.;
		}

		// The more defensive the bot is, the more does it want to target
		// weaker opponents to not aggravate the stronger ones
		if ( level_score < 0. ) {
			level_score *= player->defensive + 2.;
		}

		// The more health this bots tank has, the more prominent is this score
		level_score = std::abs( level_score ) * curr_life;
	}


	/* -------------------------------------------------------------
	 * --- 7) add points for score difference                    ---
	 * --- Target the leading bots earlier, losing ones later.   ---
	 * ------------------------------------------------------------- */
	auto opp_level_d = static_cast< double >(
		( HUMAN_PLAYER == opponent->type ) ? DEADLY_PLAYER + 1 + ( DEADLY_PLAYER - ai_level ) : opponent->type
	);
	double win_score =
		pOpp->onSameTeam
			? 0.
			: ( opponent->score - player->score ) * ( player->self_preservation + 1. ) * ( player->defensive + 2. )
	                          * static_cast< double >( ai_level + 1 ) * ( static_cast< double >( pOpp->opLife ) / 10. )
	                          / ( player->pain_sensitivity - 0.5 + opp_level_d );
	// Note: The win_score is only used if positive.
	// 1 - Self preservation: Get rid of the winner as a threat soon.
	// 2 - Defensiveness : Even more if of the defensive type.
	// 3 - The smarter, the more they do care.
	// 4 - Multiply with 10% of the opponents tank life
	// 5 - Pain Sensitivity: Can they stand the answer? ( The higher the opponent type, the more the bot fears them. )


	/* -------------------------------------------------------------
	 * --- 8) Sum up the score                                   ---
	 * --- This will be used for sorting the opponents list      ---
	 * ------------------------------------------------------------- */
	double damage_score = ( entry->damage_from * ai_level_d ) - ( entry->damage_to * opp_level_d );
	double kill_score =
		entry->killed_them > 0. // If we did not kill them, yet, the score must not become too extreme
			? ( ( entry->killed_me * ai_level_d ) / ( entry->killed_them * opp_level_d ) ) * max_life
			: entry->killed_me * ai_level_d / opp_level_d; // Like 1 death but without life multiplier.
	double prev_score = entry->damage_last;

	DEBUG_LOG_EMO( player->get_name(), "  team_mod     : %6.2lf", pOpp->team_mod )
	DEBUG_LOG_EMO( player->get_name(), "  damage_score : %6.2lf", damage_score )
	DEBUG_LOG_EMO( player->get_name(), "  kill_score   : %6.2lf", kill_score )
	DEBUG_LOG_EMO( player->get_name(), "  prev_score   : %6.2lf", prev_score )
	DEBUG_LOG_EMO( player->get_name(), "  fear_damage  : %6.2lf", fear_damage )
	DEBUG_LOG_EMO( player->get_name(), "  revenge_score: %6.2lf", revenge_score )
	DEBUG_LOG_EMO( player->get_name(), "  life_score   : %6.2lf", life_score )
	DEBUG_LOG_EMO( player->get_name(), "  dist_score   : %6.2lf", dist_score )
	DEBUG_LOG_EMO( player->get_name(), "  vict_score   : %6.2lf", vict_score )
	DEBUG_LOG_EMO( player->get_name(), "  level_score  : %6.2lf", level_score )
	DEBUG_LOG_EMO( player->get_name(), "  win_score    : %6.2lf", win_score )

	double xScore =
		( damage_score > 0. ? pOpp->team_mod * damage_score : 0. ) + ( kill_score > 0. ? pOpp->team_mod * kill_score : 0. )
	        + ( prev_score > 0. ? pOpp->team_mod * prev_score : 0. ) + ( fear_damage > 0. ? pOpp->team_mod * fear_damage : 0. )
	        + ( fear_shock > 0. ? fear_shock * fear_damage : 0. ) + ( revenge_score > 0. ? pOpp->team_mod * revenge_score : 0. )
	        + ( life_score > 0. ? pOpp->team_mod * life_score : 0. ) + ( vict_score > 0. ? pOpp->team_mod * vict_score : 0. )
	        + ( win_score > 0. ? win_score : 0. ) + dist_score + level_score;
	pOpp->score = ROUND( xScore );

	DEBUG_LOG_EMO( player->get_name(), "  Final Score  : %8d", pOpp->score )

	// --- clean up damage_last ---
	if ( entry->damage_last ) {
		entry->damage_from += entry->damage_last;
		entry->damage_last  = 0;
	}
}

/// @brief Set a new score to a weapons entry
void CAICore::update_weap_score( weentry_t* pWeap ) const {
	// As this is used a few dozen times, a shortcut to pWeap->type is nice:
	EWeaponType wType = pWeap ? static_cast< EWeaponType >( pWeap->type ) : SML_MIS;


	// === Get out quickly if the chosen item is out of stock ===
	// ==========================================================
	if ( 0 == pWeap->amount ) {
		pWeap->score = -50'000;
		return;
	}


	// === Only evaluate items that are available ===
	// ==============================================
	if ( !env.is_item_available( wType ) ) {
		pWeap->score = -100'000;
		return;
	}

	DEBUG_LOG_AI( player->get_name(), "Evaluating score for %s", weapon[wType].get_name() )

	// reset boolean helpers
	pWeap->blast_out = false;
	pWeap->kamikaze  = false;


	// === If no opponent is chosen (however this may happen) then ===
	// === the pure preferences count.                             ===
	// ===============================================================
	if ( nullptr == mem_curr ) {
		pWeap->score = pWeap->preference;
		DEBUG_LOG_AI( player->get_name(), " -> Use preference %d", pWeap->preference )
		return;
	}


	// === If this is a laser, it will only be evaluated if the tank is ===
	// === not below this players tanks as it can not be reached then.  ===
	// ====================================================================
	if ( ( SML_LAZER <= wType ) && ( LRG_LAZER >= wType ) && ( mem_curr->opY > y ) ) {
		pWeap->score = -45'000;
		DEBUG_LOG_AI( player->get_name(), " -> Target y %d is not reachable from %d", ROUND( mem_curr->opY ), ROUND( y ) )
		return;
	}


	// === If this is the percent bomb, reducer or theft bomb, its  ===
	// === damage must be adapted, as it depends on the selected    ===
	// === target and/or current capabilities.                      ===
	// ================================================================
	if ( PERCENT_BOMB == wType ) {
		pWeap->dmg_cluster = 0.;
		pWeap->dmg_single  = mem_curr->opLife / 2;
		pWeap->dmg_spread  = pWeap->dmg_single;
	}

	// === The same applies to the reducer ===
	if ( REDUCER == wType ) {
		pWeap->dmg_cluster = 0.;
		pWeap->dmg_single =
			( mem_curr->opLife / 2. ) * ( mem_curr->entry->opponent->damage_multiplier / 2. )
		        * ( player->pain_sensitivity + ai_over_mod ) / ( -1. * ( player->defensive - 1.25 - ai_over_mod ) );
		pWeap->dmg_spread = pWeap->dmg_single;
	}

	// === And the theft bomb ===
	auto theft_size = static_cast< int32_t >( player->damage_multiplier * THEFT_AMOUNT );
	if ( THEFT_BOMB == wType ) {
		double steal_amount = std::min( mem_curr->entry->opponent->money, theft_size );
		pWeap->dmg_cluster  = 0.;
		pWeap->dmg_single   = ROUND( steal_amount / ai_level_d );
		pWeap->dmg_spread   = pWeap->dmg_single;
	}


	/* -------------------------------------------------------------
	 * --- 1) Set score for reaching the target health           ---
	 * ------------------------------------------------------------- */
	double weap_dmg    = 0.; // Filled here, used for splash score, too
	double dmg_diff    = 0.;
	double point_score = mem_curr->opLife;
	// If the bot is shocked, spread and cluster weapons get a bonus:
	double shock_bonus = is_shocked ? static_cast< double >( maxAiLevel - ai_level ) + ai_over_mod : 1.;

	if ( pWeap->dmg_cluster > 1. ) {
		weap_dmg = pWeap->dmg_cluster;
		dmg_diff = ( weap_dmg / ai_over_mod ) - point_score;
		if ( dmg_diff > 0. ) {
			dmg_diff *= shock_bonus;
		}
	} else if ( pWeap->dmg_spread > ( pWeap->dmg_single + 0.25 ) ) {
		weap_dmg = pWeap->dmg_spread;
		dmg_diff = ( weap_dmg / ( ai_over_mod / 2. ) ) - point_score;
		if ( dmg_diff > 0. ) {
			dmg_diff *= shock_bonus;
		}
	} else if ( pWeap->dmg_single > 1. ) {
		weap_dmg = pWeap->dmg_single;
		dmg_diff = weap_dmg / ai_over_mod - ( THEFT_BOMB == wType ? mem_curr->entry->opponent->money : point_score );
	} else {
		dmg_diff = -point_score;
	}

	if ( dmg_diff < 0. ) {
		// Too less damage
		point_score += dmg_diff;
	} else if ( dmg_diff > 0. ) {
		// Otherwise chop off a modified difference
		point_score -=
			dmg_diff
		        / ( -( player->defensive - 2.5 ) // 3.5 full offensive, 2.5 full defensive
		            * ai_over_mod );             // the higher the level, the more the reduction.
	}

	// If this is a REDUCER, THEFT_BOMB or dirt weapon, and the fake damage is
	// higher than the target health, modify the score. The AI wants
	// to finish off the almost dead and not debuff them
	if ( ( REDUCER == wType ) || ( THEFT_BOMB == wType ) || ( ( DIRT_BALL <= wType ) && ( SMALL_DIRT_SPREAD >= wType ) ) ) {
		if ( mem_curr->opLife <= blast_min ) {
			point_score = 0;
		} else if ( mem_curr->opLife <= blast_med ) {
			point_score /= static_cast< double >( ai_level + 3 );
		} else if ( mem_curr->opLife <= blast_big ) {
			point_score /= static_cast< double >( ai_level + 1 ) / ai_over_mod;
		} else if ( dmg_diff > 0. ) {
			point_score /= ai_over_mod;
		}
	}

	// If this is the theft bomb, but we do not need money urgently,
	// reduce the score
	if ( ( THEFT_BOMB == wType ) && !need_money ) {
		point_score /= ai_level_d + ai_over_mod;
	}


	/* -------------------------------------------------------------
	 * --- 2) check buried state, shaped charges and the driller ---
	 * ---    might still be usable.                             ---
	 * ------------------------------------------------------------- */
	double unbury_score = 0.;
	if ( buried > BURIED_LEVEL ) {
		// Shaped charges refer to the y coordinate
		if ( ( ( SHAPED_CHARGE <= wType ) && ( CUTTER >= wType )
		       && ( ROUND( std::abs( mem_curr->opY - y ) ) < ( weapon[wType].radius / 20 ) )
		       && ( mem_curr->distance < weapon[wType].radius ) )
		     || ( /* The driller is only usable in a vertical way: */
		          ( DRILLER == wType ) && ( ROUND( std::abs( mem_curr->opX - x ) ) < ( weapon[wType].radius / 20 ) )
		          && ( mem_curr->distance < weapon[wType].radius )
		     ) ) {
			// This one is usable.
			unbury_score = pWeap->dmg_single * ai_over_mod;
		}

		// Riot bombs and charges are the ultimate tools, of course
		else if (
			( ( RIOT_CHARGE <= wType ) && ( RIOT_BLAST >= wType ) ) || ( ( RIOT_BOMB <= wType ) && ( HVY_RIOT_BOMB >= wType ) )
		) {
			unbury_score = ai_type_mod * static_cast< double >( weapon[wType].radius )
			             * static_cast< double >( buried - BURIED_LEVEL + ai_level );
		}
		// Everything else is (mostly) useless
		else {
			if ( pWeap->dmg_cluster > 1. ) {
				unbury_score -= ai_type_mod * pWeap->dmg_cluster * pWeap->dmg_single;
			} else {
				unbury_score -= ai_type_mod * ( pWeap->dmg_spread + pWeap->dmg_single );
				// However, if the target is in range and a self hit would not
				// kill our own tank...
				if ( ( mem_curr->distance < weapon[wType].radius ) && ( curr_life > pWeap->dmg_single )
				     && ( curr_life > pWeap->dmg_spread ) ) {
					unbury_score += ai_over_mod * pWeap->dmg_single / player->self_preservation;
				}
			}
		} // end of "useless" weapons

		if ( unbury_score > 1. ) {
			pWeap->blast_out = true;
		}
	} // end of unbury score.

	// If not buried, riot weapons are useless:
	else if ( ( ( RIOT_CHARGE <= wType ) && ( RIOT_BLAST >= wType ) ) || ( ( RIOT_BOMB <= wType ) && ( HVY_RIOT_BOMB >= wType ) ) ) {
		unbury_score = -50000.;
	}


	/* -------------------------------------------------------------
	 * --- 3) Panic score - If this bot has panicked, the more   ---
	 * ---    damage the better.                                 ---
	 * ------------------------------------------------------------- */
	double panic_score = 0.;
	if ( is_shocked && ( mem_curr->entry == shocker ) && ( buried <= BURIED_LEVEL ) ) {
		panic_score = pWeap->dmg_cluster > 1. ? pWeap->dmg_cluster : pWeap->dmg_spread;
		// If this is a debuffing weapon like reducer or percent bomb,
		// it is valued even higher.
		if ( REDUCER == wType ) {
			panic_score += mem_curr->entry->opponent->damage_multiplier * mem_curr->opLife * player->self_preservation;
		} else if ( PERCENT_BOMB == wType ) {
			panic_score += pWeap->dmg_single / player->self_preservation;
		} else if ( ( DIRT_BALL <= wType ) && ( SMALL_DIRT_SPREAD >= wType ) ) {
			panic_score += weapon[wType].radius * weapon[wType].spread * ( 1.5 + player->defensive );
		} else if ( THEFT_BOMB == wType ) {
			panic_score += pWeap->dmg_single * player->self_preservation;
		}
	}


	/* ----------------------------------------------------------------------
	 * --- 4) Score for reaching buried opponents.                        ---
	 * ---    If an opponent is buried, burrowers and tremors are useful. ---
	 * ---------------------------------------------------------------------- */
	double dig_score = 0.;
	if ( mem_curr->is_buried || ( ( mem_curr->opX > x ) && ( mem_curr->buried_l >= BURIED_LEVEL_HALF ) )
	     || ( ( mem_curr->opX < x ) && ( mem_curr->buried_r >= BURIED_LEVEL_HALF ) ) ) {

		// Chain weapons can push through dirt, but are bad when the own tank
		// is buried.
		if ( ( CHAIN_GUN <= wType ) && ( JACK_HAMMER >= wType ) ) {
			dig_score = pWeap->dmg_single * static_cast< double >( weapon[wType].get_delay_div() )
			          / ( 1.75 + player->defensive ) * ( buried > BURIED_LEVEL ? -1. : 1. );
		}

		// Burrowers can actually directly reach the target
		else if ( ( BURROWER <= wType ) && ( PENETRATOR >= wType ) ) {
			dig_score = pWeap->dmg_single * ai_type_mod;
		}

		// tremors are somewhat weak, but the do not only (possibly) reach
		// the target but remove dirt as well.
		else if ( ( TREMOR <= wType ) && ( TECTONIC >= wType ) ) {
			dig_score = ( pWeap->dmg_single + weapon[wType].radius ) * ai_over_mod * ( 2.1 + player->defensive );
		}

		// Riot bombs are useful to undig an opponent as well.
		else if ( ( RIOT_BOMB <= wType ) && ( HVY_RIOT_BOMB >= wType ) ) {
			dig_score = weapon[wType].radius
			          // Note: pain sensitivity is used, as not doing any
				  //       damage won't trigger a vengeance reaction.
			          * ( 1. + player->defensive + player->pain_sensitivity );
		}

		// remember that this is chosen for blasting out an opponent:
		if ( dig_score > 1. ) {
			pWeap->blast_out = true;
		}
	}


	/* --------------------------------------------------------------
	 * --- 5) Splash damage                                       ---
	 * --- Check all tanks whether they are in "splash range" and ---
	 * --- add or dock points according to the team_mod value of  ---
	 * --- the hit tanks. This score can be negative and is meant ---
	 * --- to help bots to decide against oversized weapons if    ---
	 * --- good working alternatives are present.                 ---
	 * -------------------------------------------------------------- */
	double splash_score = 0.;
	double money_made   = 0.; // build here, used below
	double money_cost   = 0.; // build here, used below
	if ( buried <= BURIED_LEVEL ) {
		opentry_t* op = mem_head;

		// Always assume a full direct hit:
		double xhit = mem_curr->opX;
		double yhit = mem_curr->opY;

		// The minimum in_rate depends on the defensive level. CExplosion takes
		// different values for the shaped weapons and tectonics. Further the
		// full rate limit is 10% damage. The bot does not calculate minimum
		// axis rates, and the full rate limit might become lower or higher than
		// this 10%. This is wanted as bots "only estimate".
		double rate_limit = ( player->defensive + .75 ) / 10.;
		// result: Over-offensive Rogue: (-1.25 + 0.75) / 10. => 0.5 / 10. =>  5%
		//         Over-defensive Bastion: ( 1.25 + 0.75) / 10. => 2.0 / 10. => 20%

		while ( op ) {

			// Do not evaluate the target, as it will get the hit anyway
			if ( op == mem_curr ) {
				op = op->next;
				continue;
			}

			// Yield on each iteration to not hog the CPUs
			if ( !global.skipping_computer_play ) {
				std::this_thread::yield();
			}

			CPlayer* pl = op->entry->opponent;
			CTank*   lt = pl ? pl->tank : nullptr; // short cut

			if ( !lt || lt->destroy || ( op->opLife < 1. ) ) {
				// irrelevant
				op = op->next;
				continue;
			}

			double weap_rad  = weap_curr->radius;
			double xrad      = DRILLER == wType ? weap_rad / 20. : weap_rad;
			double yrad      = ( ( SHAPED_CHARGE <= wType ) && ( CUTTER >= wType ) ) ? weap_rad / 20. : weap_rad;
			double in_rate_x = 0.;
			double in_rate_y = 0.;

			if ( lt->is_in_ellipse( xhit, yhit, xrad, yrad, in_rate_x, in_rate_y ) ) {
				double in_rate = in_rate_x * in_rate_y;

				if ( in_rate < rate_limit ) {
					in_rate = rate_limit;
				}

				double score = std::min( weap_dmg * in_rate, op->opLife ) * op->team_mod;

				// Do not overdo positive scores
				if ( score >= 0 ) {
					// Note: That is [1.1;4.8]
					score /= ai_over_mod * static_cast< double >( ( ai_level + 1 ) / 2. );
				}

				splash_score += score;

				DEBUG_LOG_EMO(
					player->get_name(),
					"%s in splash range, %s %d points",
					pl == player ? "I am" : pl->get_name(),
					score > 0 ? "add " : "dock",
					std::abs( ROUND( score ) )
				)

				// Note down money made or cost:
				if ( THEFT_BOMB == wType ) {
					double theft_done =
						std::min( in_rate * theft_size, static_cast< double >( op->entry->opponent->money ) );
					if ( op->team_mod > 0. ) {
						money_made += theft_done;
					} else if ( lt != tank ) { // No effect on self!
						money_cost += theft_done / ( 10. - ai_level_d );
					}
				} else {
					if ( op->team_mod > 0. ) {
						money_made +=
							( std::min( weap_dmg * in_rate, op->opLife )
						          * static_cast< double >( env.scoreHitUnit ) )
						        + ( ( weap_dmg * in_rate ) >= op->opLife
						                    ? static_cast< double >( env.scoreUnitDestroyBonus )
						                    : 0. );
					} else if ( lt == tank ) {
						money_cost +=
							( std::min( weap_dmg * in_rate, curr_life )
						          * static_cast< double >( env.scoreSelfHit ) )
						        + ( ( weap_dmg * in_rate ) >= curr_life
						                    ? static_cast< double >( env.scoreUnitSelfDestroy )
						                    : 0. );
					} else {
						money_cost +=
							( std::min( weap_dmg * in_rate, op->opLife )
						          * static_cast< double >( env.scoreTeamHit ) )
						        + ( ( weap_dmg * in_rate ) >= op->opLife
						                    ? static_cast< double >( env.scoreUnitSelfDestroy )
						                    : 0. );
					}
				} // end of regular weapon check
			} // end of opponent in explosion

			op = op->next;
		} // end of looping opponents memory
	} // end of calculating splash damage score


	/* -------------------------------------------------------------
	 * --- 6) Kamikaze potential                                 ---
	 * --- If the bot decides to self destruct, it is important  ---
	 * --- to check what this weapon would do.                   ---
	 * ------------------------------------------------------------- */
	double selfde_score = 0.;
	if ( ( mem_curr->opLife > ( curr_life * 10. ) ) || ( is_shocked && ( mem_curr->opLife > ( curr_life * 5. ) ) ) ) {

		// Only some weapons are considered for a big boom bye bye
		if ( ( ( SML_NUKE <= pWeap->type ) && ( DTH_HEAD >= pWeap->type ) )
		     || ( ( WIDE_BOY == pWeap->type ) || ( CUTTER == pWeap->type ) )
		     || ( ( MED_NAPALM == pWeap->type ) || ( LRG_NAPALM == pWeap->type ) ) ) {
			double kRad = pWeap->radius;
			double kDmg = pWeap->dmg_single;

			// for a kamikaze, the shaped weapons have to be shot somewhat to
			// the side, so extend the radius if this tank is not buried, or
			// reduce it to zero if it is.
			if ( ( WIDE_BOY == pWeap->type ) || ( CUTTER == pWeap->type ) ) {
				if ( buried >= ( BURIED_LEVEL / ai_level ) ) {
					kRad = 0.;
				} else {
					kRad += 50. * ai_over_mod;
				}
			}

			// The same counts for the napalm, although it does not really have
			// a radius. This must be estimated according to the current wind.
			else if ( ( MED_NAPALM == pWeap->type ) || ( LRG_NAPALM == pWeap->type ) ) {
				if ( buried >= ( BURIED_LEVEL / ai_level ) ) {
					kRad = 0.;
				} else {
					kRad = std::abs( global.wind / ( env.wind_strength / 4. ) ) + 1.;
					/* This produces the following multiplier: (with max wind = 8)
					 * wind = 0 : (0 / (8 / 4)) + 1 = (0 / 2) + 1 = = 1
					 * wind = 1 : (1 / (8 / 4)) + 1 = (1 / 2) + 1 = = 1.5
					 * wind = 4 : (4 / (8 / 4)) + 1 = (4 / 2) + 1 = = 3
					 * wind = 6 : (6 / (8 / 4)) + 1 = (6 / 2) + 1 = = 4
					 * wind = 8 : (8 / (8 / 4)) + 1 = (8 / 2) + 1 = = 5
					 */
					kRad *= weapon[pWeap->type].launchSpeed / ai_level;

					// Napalm is a cluster, but not everything will hit
					kDmg = pWeap->dmg_cluster / ai_level_d;
				}
			}

			// Check against collateral damage unless shocked and the shocker is
			// in range
			bool tgt_in_range = false;
			if ( is_shocked && ( mem_curr->entry == shocker ) // can this be false?
			     && ( mem_curr->distance < kRad ) ) {
				// No check, just do it
				selfde_score = ( pWeap->dmg_cluster + pWeap->dmg_single ) * ai_over_mod;
				tgt_in_range = true;
			} else {
				// Nope, be reasonable
				selfde_score        = kDmg;
				sOppMemEntry* check = mem_head;
				while ( check ) {

					// Yield on each iteration to not hog the CPUs
					if ( !global.skipping_computer_play ) {
						std::this_thread::yield();
					}

					if ( ( check->opLife > 0. ) && ( check->distance < kRad ) ) {
						if ( check->onSameTeam ) {
							selfde_score -= kDmg * ai_type_mod * ( 1.25 + player->defensive );
						} else {
							selfde_score += kDmg * ai_over_mod * check->team_mod;
						}

						// Award extra points if this is the current target
						if ( mem_curr == check ) {
							selfde_score += check->opLife * check->team_mod;
							tgt_in_range  = true;
						}
					}
					check = check->next;
				}
			} // end of checking done damage

			// Now, if the score is positive, this is a kamikaze choice:
			if ( ( selfde_score > 0. ) && tgt_in_range ) {
				pWeap->kamikaze = true;
			}

		} else if ( THEFT_BOMB == pWeap->type ) {
			// In such a situation a (non-aggravating!) theft might
			// be considered useful the more defensive and self preservative
			// a bot is.
			selfde_score = pWeap->dmg_single * ai_type_mod * ( 2. + player->defensive + player->self_preservation );
		} else {
			// Unsuitable
			selfde_score -= pWeap->dmg_spread * ai_type_mod;
		}
	}


	/* -------------------------------------------------------------
	 * --- 7) Economic evaluation                                ---
	 * --- If the maximum damage bounty the weapon can generate  ---
	 * --- is lower than the weapon score, points are docked.    ---
	 * --- Generating more money than the weapon is worth adds   ---
	 * --- some bonus points.                                    ---
	 * ------------------------------------------------------------- */
	double eco_score  = 0.;
	double money_mod  = need_money ? ai_type_mod : ( static_cast< double >( RAND_AI_1P + 1 ) * 10. );
	double money_diff = money_made - money_cost;
	if ( !is_shocked ) {
		// Note: Shocked bots do not care about money!
		if ( money_diff >= weapon[pWeap->type].cost ) {
			eco_score += ( money_made / money_mod ) - ( money_cost / money_mod );
		} else {
			eco_score -= money_diff / money_mod;
		}
	}


	/* -------------------------------------------------------------
	 * --- 8) Sum up the score                                   ---
	 * --- This will be used for sorting the weapons list        ---
	 * ------------------------------------------------------------- */
	double pref_score = pWeap->preference / ai_level_d / ( std::abs( dmg_diff ) > 1. ? std::abs( dmg_diff ) : 1. );
	// the further away, the less likely.

	DEBUG_LOG_EMO( player->get_name(), "  preference   : %6.2lf", pref_score )
	DEBUG_LOG_EMO( player->get_name(), "  point_score  : %6.2lf [diff %6.2lf]", point_score, dmg_diff )
	DEBUG_LOG_EMO( player->get_name(), "  unbury_score : %6.2lf", unbury_score )
	DEBUG_LOG_EMO( player->get_name(), "  panic_score  : %6.2lf", panic_score )
	DEBUG_LOG_EMO( player->get_name(), "  splash_score : %6.2lf", splash_score )
	DEBUG_LOG_EMO( player->get_name(), "  selfde_score : %6.2lf", selfde_score )
	DEBUG_LOG_EMO( player->get_name(), "  dig_score    : %6.2lf", dig_score )
	DEBUG_LOG_EMO( player->get_name(), "  eco_score    : %6.2lf (M %6.2lf / C -%6.2lf / D %6.2lf)", eco_score, money_made, money_cost, money_diff )

	double xScore = pref_score + point_score + unbury_score + panic_score + splash_score + selfde_score + dig_score + eco_score;

	pWeap->score  = ROUND( xScore );

	DEBUG_LOG_EMO( player->get_name(), "  Final Score  : %8d", pWeap->score )
}

/** @brief Select a tool to free the tank or clear a path
 * @param[in] free_tank If set to true, a tool to free the tank is chosen,
 *            a tool to clear the path otherwise.
 * @param[in] is_last If set to true, an emergency selection is done to force
 *            this method to succeed.
 * @return true if the selection succeeded, false otherwise.
 **/
bool CAICore::use_freeing_tool( bool free_tank, bool is_last ) {
	if ( /* If the current weapon is already used to blast out an opponent, no other tool is needed. */
	     ( !free_tank && weap_curr && weap_curr->blast_out )
	     /* Standard freeing tools in buried situation */
	     || ( free_tank
	          && ( use_weapon( RIOT_BLAST ) || use_weapon( RIOT_CHARGE )
	               || ( !mem_curr->is_buried && use_item( ITEM_SWAPPER ) )
	               /* Note: No mass teleport here! */
	               || use_item( ITEM_TELEPORT ) ) )
	     || use_weapon( HVY_RIOT_BOMB )
	     || use_weapon( RIOT_BOMB )
	     /* non-freeing tools that can be used to blast free an opponent. */
	     || ( !free_tank && ( use_weapon( CHAIN_GUN ) || use_weapon( DRILLER ) || use_weapon( CHAIN_MISSILE ) ) )
	     /* If the "normal" selection is not possible (out of stock) but this is the last
	      * try, the bot has to revert to standard missiles. Expensive, but should work. */
	     || ( is_last
	          && ( ( !free_tank && ( use_weapon( SML_NUKE ) || use_weapon( LRG_MIS ) || use_weapon( MED_MIS ) ) )
	               || use_item( ITEM_TELEPORT ) || ( use_item( ITEM_SWAPPER ) && !mem_curr->is_buried )
	               || use_item( ITEM_MASS_TELEPORT ) // As a last resort this is okay.
	               || use_weapon( SML_MIS ) ) )
	) {

		DEBUG_LOG_AIM(
			player->get_name(),
			"%sSelected %s to %s",
			is_last ? "(LAST) " : "",
			weap_idx < WEAPONS ? weapon[weap_idx].get_name() : item[weap_idx - WEAPONS].get_name(),
			free_tank ? "free my tank" : "clear firing path"
		)

		return true;
	}

	return is_last;
}

/// @brief explicitly select @a item_type, returns true if available and chosen.
bool CAICore::use_item( EItemType item_type ) {
	if ( env.is_item_available( item_type ) && ( player->ni[item_type] > 0 ) ) {
		item_curr = item_head;
		while ( item_curr && ( item_curr->type != item_type ) ) {
			item_curr = item_curr->next;
		}

		if ( item_curr && ( item_curr->type == item_type ) ) {
			weap_idx  = WEAPONS + item_type;
			weap_curr = nullptr;
			return true;
		} else if ( weap_curr ) {
			item_curr = nullptr;
		}
	}

	return false;
}

/// @brief convenience function to use the full index as an integer to choose
/// an item. Full index means the value is beyond the WEAPONS constant.
bool CAICore::use_item( int32_t item_index ) {
	if ( ( item_index >= WEAPONS ) && ( item_index < THINGS ) ) {
		return use_item( static_cast< EItemType >( item_index - WEAPONS ) );
	}
	return false;
}

/// @brief explicitly select @a weapon_type, returns true if available and chosen.
bool CAICore::use_weapon( EWeaponType weap_type ) {
	if ( env.is_item_available( weap_type ) && ( player->nm[weap_type] > 0 ) ) {
		weap_curr = weap_head;
		while ( weap_curr && ( weap_curr->type != weap_type ) ) {
			weap_curr = weap_curr->next;
		}

		if ( weap_curr && ( weap_curr->type == weap_type ) ) {
			weap_idx  = weap_type;
			item_curr = nullptr;
			return true;
		} else if ( item_curr ) {
			weap_curr = nullptr;
		}
	}

	return false;
}

/// @brief convenience function to use the numeric index as an integer to choose
/// a weapon.
bool CAICore::use_weapon( int32_t weap_index ) {
	if ( weap_index < WEAPONS ) {
		return use_weapon( static_cast< EWeaponType >( weap_index ) );
	}
	return false;
}

/// @brief Call this once the AI weapon is fired to signal the end of the
/// players turn
void CAICore::weapon_fired() {
	if ( is_working && !is_stopped && ( PS_FIRE == pl_stage ) ) {
		DEBUG_LOG_AI( player->get_name(), "------------------------------", 0 )
		DEBUG_LOG_AI( player->get_name(), " Weapon fired for %s", player->get_name() )

		lguard_t guard( action_mutex );
		pl_stage = PS_CLEANUP;
		action_condition.notify_one();
	}
}

/** @brief Attempt to move the tank
 *
 * @return true if the tank was moved
 **/
bool CAICore::move_tank() {
	/* Moving the AI tank is easy. Just set the command and wait for the
	 * main thread to react.
	 * However, where shall the tank move and what distance?
	 *
	 * - If the target is very near (under two bitmap widths) then move
	 *   away.
	 * Otherwise:
	 * - If the angle is steep (75° and up), assume the shot must go
	 *   over a hill and move away from the target.
	 * - If the angle is flat  (15° and down), move towards the target,
	 *   the way seems clear at least.
	 * Otherwise:
	 * - If the overshoot is negative (too short), move towards the target.
	 * - If the overshoot is positive (too far), move away from the target.
	 */
	double  min_dist  = tank->get_diameter() + mem_curr->entry->opponent->tank->get_diameter();
	int32_t want_dist = 0; // Eventually move in this direction ...
	int32_t want_dir  = 0; // ... by this amount

	if ( mem_curr->distance < min_dist ) {
		// The first case: we are too near and want to move away
		want_dir  = mem_curr->opX > x ? DIR_LEFT : DIR_RIGHT;
		want_dist = ROUND( want_dir * ( min_dist - mem_curr->distance + RAND_AI_1P ) );
	} else if ( ( curr_angle <= 195 ) && ( curr_angle >= 165 ) ) {
		// The second case, the angle is steep
		want_dir  = mem_curr->opX > x ? DIR_LEFT : DIR_RIGHT;
		want_dist = want_dir * ( 20 - std::abs( 180 - curr_angle ) + RAND_AI_1P );
	} else if ( ( curr_angle <= 105 ) || ( curr_angle >= 255 ) ) {
		// The third case, the angle is very flat
		want_dir  = mem_curr->opX > x ? DIR_RIGHT : DIR_LEFT;
		want_dist = want_dir * ( std::abs( curr_angle - 180 ) - 70 + RAND_AI_1P );
	} else {
		// Last two cases use the overshoot as a distance to use
		want_dist = best_overshoot != MAX_OVERSHOOT ? best_overshoot : curr_overshoot;
		want_dir  = SIGN( want_dist );
		// "tune" the distance
		want_dist += RAND_AI_1P * want_dir;
	}

	// Now that direction and distance are set up, go for it.
	bool tank_was_moved = false;

	DEBUG_LOG_AIM( player->get_name(), "Starting to move %s for %d", DIR_LEFT == want_dir ? "left" : "right", want_dist )

	while ( !is_stopped && can_move.load( ATOMIC_READ ) && want_dist ) {

		is_moved_by.store( 0, ATOMIC_WRITE );
		pl_stage = DIR_LEFT == want_dir ? PS_MOVE_LEFT : PS_MOVE_RIGHT;

		// Wait for the move to happen
		while ( !is_stopped && can_move.load( ATOMIC_READ ) && ( 0 == is_moved_by.load( ATOMIC_READ ) ) ) {
			std::this_thread::yield();
		}

		// Do not do double moves!
		pl_stage = PS_AIM;

		if ( !tank_was_moved && is_moved_by.load( ATOMIC_READ ) ) {
			tank_was_moved = true;
		}

		want_dist -= is_moved_by.load( ATOMIC_READ );
	} // That's it, really!

	// No matter how much movement was done, this tank
	// won't move again in this turn.
	can_move.store( false, ATOMIC_WRITE );

	// However, if the tank was moved, all distances are different now:
	if ( tank_was_moved ) {

		// Update current position
		x = tank->x;
		y = tank->y;

		// Update all distances
		opentry_t* op = mem_head;
		while ( op ) {
			if ( op->entry->opponent->tank && !op->entry->opponent->tank->destroy ) {
				op->distance = FABSDISTANCE2( x, y, op->opX, op->opY );
			}
			op = op->next;
		}
	}

	DEBUG_LOG_AIM( player->get_name(), "Moving finished %s (%d distance left)", want_dist ? "incompletely" : "successfully", want_dist )

	return tank_was_moved;
}

/// @brief Core threading operator
void CAICore::operator() () {
	while ( allow_work && !is_stopped ) {

		// Go to sleep until the thread is woken up
		luniq_t actionLock( action_mutex );
		action_condition.wait( actionLock, [this] { return ( is_working || is_stopped ); } );

		// If the thread is to be stopped, exit the loop
		if ( is_stopped ) {
			// Cleaner than "break", but only on a philosophical level... ;-)
			continue;
		}

		if ( !initialize() ) {
			pl_stage   = PS_AI_IS_IDLE;
			is_working = false;
			continue;
		}

		// --------------------------------------------------------------------
		// --- First update the foe list, only then a target can be picked- ---
		// --------------------------------------------------------------------
		check_opp_mem();


		// -----------------------------------------------------------------
		// --- See whether the bot falls for a fear shock.               ---
		// --- If they are mortally afraid of a shocker, no other target ---
		// --- will be picked. It is fixed on that one then.             ---
		// -----------------------------------------------------------------
		if ( shocker ) {
			DEBUG_LOG_EMO( player->get_name(), "Terrified by %s (fear shock: %lf)", shocker->opponent->get_name(), shocker->fear_shock )
			double reshock = shocker->fear - ( static_cast< double >( RAND_AI_0P + 2 ) / 2. );
			if ( reshock >= shocker->fear_shock ) {
				is_shocked = true;

				// Generate a nice message telling the world that we are in awe:
				if ( !is_stopped && !global.skipping_computer_play ) {
					char const* text = CPlayer::select_panic_phrase( shocker->opponent );
					try {
						if ( text ) {
							// Wait for the AI to be allowed to create texts
							while ( !text_allowed.load( ATOMIC_READ ) ) {
								std::this_thread::yield();
							}

							// Now create the instance
							new CFloatText( text, x, y - 30., .0, -.4, player->color, CENTRE, TS_NO_SWAY, 150, false );
						}
					} catch ( std::exception& e ) {
						std::cerr << __func__ << " new CFloatText: " << e.what() << std::endl;
					}
					if ( text ) {
						free( const_cast< char* >( text ) );
					}
				}


				DEBUG_LOG_EMO( player->get_name(), "Shock confirmed with %lf over %lf", reshock, shocker->fear_shock )
			} else {
				is_shocked = false;
				DEBUG_LOG_EMO( player->get_name(), "Overcame shock with %lf under %lf", reshock, shocker->fear_shock )
			}
		}


		// ---------------------------------------------------
		// --- Set basic behaviour values                  ---
		// --- Done here and not in initialize so the full ---
		// --- shock check is already done.                ---
		// ---------------------------------------------------
		findOppAttempts  = ai_level + 1 - ( is_shocked ? ai_level / 2 : 0 );
		findRngAttempts  = ( ( ai_level + 1 ) * 2 ) / ( is_shocked ? 2 : 1 );
		findTgtAttempts  = ai_level + 1 - ( is_shocked ? ai_level : 0 );
		findWeapAttempts = ai_level * 2 - ( is_shocked ? ai_level : 0 );
		focus_rate       = ai_level_d * 2. / ( static_cast< double >( maxAiLevel * 2 ) + 1. );
		error_multiplier = static_cast< double >( maxAiLevel + 1 - ai_level ) / static_cast< double >( findRngAttempts );
		max_bounce       = ROUND( ai_level * 3. * focus_rate ) + 2;
		/* The results should be [if shocked]:
		 * findOppAttempts : Useless   2   [1], Deadly + 1:  7    [4]
		 * findRngAttempts : Useless:  4   [2], Deadly + 1: 14    [7]
		 * findTgtAttempts : Useless:  2   [1], Deadly + 1:  7    [1]
		 * findWeapAttempts: Useless:  2   [1], Deadly + 1: 12    [6]
		 * focus_rate       : Useless:  0.154,   Deadly + 1:  0.923
		 * error_multiplier : Useless:  1.5 [3], Deadly + 1:  0.071 [0.143]
		 * max_bounce       : Useless:  2        Deadly + 1:  19
		 */

		DEBUG_LOG_AI( player->get_name(), "AI Level       : %d (%s)", ai_level, getLevelName( ai_level ) )
		DEBUG_LOG_AI( player->get_name(), "error_multiplier: %4.3lf", error_multiplier )
		DEBUG_LOG_AI( player->get_name(), "findOppAttempts: %d", findOppAttempts )
		DEBUG_LOG_AI( player->get_name(), "findRngAttempts: %d", findRngAttempts )
		DEBUG_LOG_AI( player->get_name(), "findTgtAttempts: %d", findTgtAttempts )
		DEBUG_LOG_AI( player->get_name(), "focus_rate      : %4.3lf", focus_rate )
		DEBUG_LOG_AI( player->get_name(), "max_bounce      : %d", max_bounce )
		DEBUG_LOG_AI( player->get_name(), "need_money      : %s", need_money ? "Yes" : "No" )


		// ------------------------------------------------------------------
		// --- The full cycle of target selection, weapon/item selection, ---
		// --- setting up the basic combat values and targeting the       ---
		// --- selected weapon might need a few attempts. The higher the  ---
		// --- AI level, the more attempts the bot gets. If the maximum   ---
		// --- number of attempts is reached, all used methods are forced ---
		// --- to come up with a minimum result.                          ---
		// ------------------------------------------------------------------
		int32_t tgt_attempts  = 0;
		int32_t opp_attempts  = 0;
		int32_t weap_attempts = 0;
		int32_t total_tries   = findTgtAttempts * findOppAttempts * findWeapAttempts;
		bool    done          = false;

		while ( allow_work && is_working && !is_stopped && ( need_aim || !is_blocked ) // end if a free is needed
		        && ( tgt_attempts < findTgtAttempts ) ) {

			// Yield on each iteration to not hog the CPUs
			if ( !global.skipping_computer_play ) {
				std::this_thread::yield();
			}

			// ----------------------------------------------------------
			// --- 1) Cycle target and item selection.                ---
			// --- Those are combined, because selecting a different  ---
			// --- target later might make the current item selection ---
			// --- less effective or even useless. Thus the item is   ---
			// --- chosen individually.                               ---
			// ----------------------------------------------------------
			if ( !opp_attempts && !weap_attempts ) {
				++tgt_attempts;
				mem_curr = nullptr;
				DEBUG_LOG_AIM( player->get_name(), "Starting setup %d / %d", tgt_attempts, findTgtAttempts )
			}
			done = setup_attack( tgt_attempts == findTgtAttempts, opp_attempts, weap_attempts );

			// ----------------------------------------------------------
			// --- 2) Calculate basic attack values.                  ---
			// --- If the target and item selection is different than ---
			// --- in the last round, new basic values must be        ---
			// --- calculated. If the selections are what this player ---
			// --- had in the last round, this won't be needed. Just  ---
			// --- continue were we left off last round.              ---
			// ----------------------------------------------------------
			if ( done ) {
				done = calc_attack( tgt_attempts * opp_attempts * weap_attempts, total_tries );
			}

			// ----------------------------------------------------------
			// --- 3) Aim the current selection                       ---
			// ----------------------------------------------------------
			if ( done && need_aim && !is_blocked ) {
				done = aim( tgt_attempts * opp_attempts * weap_attempts, total_tries, opp_attempts >= ( findOppAttempts / 2 ) );
			} else if ( !need_aim || is_blocked ) {
				DEBUG_LOG_AIM(
					player->get_name(),
					"No aiming done: %s, %s",
					need_aim ? "Aiming needed" : "Aiming NOT needed",
					is_blocked ? "shot is blocked" : "Shot is NOT blocked"
				)
			}


			// ------------------------------------------------------
			// --- 4) If this round was successful, check whether ---
			// ---    A new best setup is found                   ---
			// ------------------------------------------------------
			if ( done ) {
				// Reset opponent and weapon attempts if a positive score
				// was achieved and the AI has tried enough items or the
				// opponent selection is finished.
				if ( best_round_score > 0 ) {
					if ( ( weap_attempts > ai_level ) || ( opp_attempts == findOppAttempts ) ) {
						opp_attempts  = 0;
						weap_attempts = 0;
					}

					// Tweak the score if the primary target was hit:
					if ( best_prime_hit ) {

						// add the weapon and opponent score, so attacks, even
						// if they are not perfect, get emphasized if the preferred
						// setup is chosen:
						if ( mem_curr && revengee && ( player != mem_curr->entry->opponent ) ) {
							best_round_score +=
								mem_curr->score / ( revengee == mem_curr->entry ? ai_level : ai_level * 10 );
						}
						if ( weap_curr && ( weap_curr->dmg_single > 0 ) ) {
							best_round_score += weap_curr->score / ( ai_level * 10 );
						}
					}
				} // end of having a best_round_score greater than zero

				// Note down best setup score and settings if better or
				// forced to succeed due to last attempt condition
				bool new_best_setup_score = ( best_round_score > best_setup_score );
				if ( ( ( new_best_setup_score && best_prime_hit )
				       || ( !best_setup_prime && ( new_best_setup_score || best_prime_hit ) ) )
				     || ( ( NEUTRAL_ROUND_SCORE == best_setup_score ) && ( tgt_attempts == findTgtAttempts )
				          && need_success ) ) {
					best_setup_angle     = curr_angle;
					best_setup_damage    = mem_curr->dmg_done;
					best_setup_item      = item_curr;
					best_setup_mem       = mem_curr;
					best_setup_overshoot = best_overshoot;
					best_setup_power     = curr_power;
					best_setup_prime     = best_prime_hit;
					best_setup_weap      = weap_curr;
					DEBUG_LOG_AIM(
						player->get_name(),
						"New best setup with angle %d, power %d using %s : (%d > %d)",
						GET_DISP_ANGLE( curr_angle ),
						curr_power,
						weap_idx < WEAPONS ? weapon[weap_idx].get_name() : item[weap_idx - WEAPONS].get_name(),
						best_round_score,
						best_setup_score
					)
					best_setup_score = best_round_score;

					// This targeting round is definitely over
					opp_attempts  = 0;
					weap_attempts = 0;

					if ( need_success && best_setup_prime && ( best_setup_score > 0 ) ) {
						// There is no need to force anything any more:
						need_success = false;
					}

					// Give feedback according to what has happened
					if ( ( best_round_score > 0 ) && best_prime_hit ) {
						show_feedback( "!!!", GREEN, -.5, TS_NO_SWAY, 150 );
					} else {
						show_feedback( "!", GREEN, -.6, TS_HORIZONTAL, 120 );
					}
				} else if ( need_success ) {
					show_feedback( "?", RED, -.7, TS_HORIZONTAL, 90 );
				}
			} // end of setup score handling
		} // end of full preparation cycle


		// ---------------------------------------------------------
		// --- If the revengee has been changed due to the score ---
		// --- considerations, write back the new victim:        ---
		// ---------------------------------------------------------
		if ( revengee && ( revengee->opponent != player->revenge ) ) {
			if ( revengee->opponent != player ) {
				player->revenge = revengee->opponent;
			} else {
				revengee = nullptr;
			}
		} else if ( !revengee ) {
			player->revenge = nullptr;
		}


		// --------------------------------------------
		// --- If no real setup could be found, see ---
		// --- whether a freeing attempt is needed. ---
		// --------------------------------------------
		if ( !is_stopped && !is_shocked && !is_blocked // If these fail, aim() already has set up
		     && need_aim                               // a freeing attempt. Do not do it twice!
		     && best_setup_weap && ( ( best_setup_score < 0 ) || ( ( 0 == best_setup_score ) && RAND_AI_0P ) ) ) {
			DEBUG_LOG_AIM( player->get_name(), "Best setup score %d too low!", best_setup_score )

			// First, copy best noted data (if any)
			if ( NEUTRAL_ROUND_SCORE != best_setup_score ) {
				curr_angle = best_setup_angle;
				curr_power = best_setup_power;
			}

			// Now see whether to unbury or clear the path:
			if ( ( buried_l >= ( BURIED_LEVEL_HALF / 2 ) ) || ( buried_r >= ( BURIED_LEVEL_HALF / 2 ) ) ) {
				use_freeing_tool( true, true );
				calc_unbury( true );
			} else {
				curr_angle = best_setup_angle;
				curr_power = best_setup_power;
				if ( RAND_AI_0P || hill_detected || !best_setup_weap || ( best_setup_weap->spread > 1 )
				     || ( best_setup_weap->subMunCount > 0 ) || ( REDUCER == best_setup_weap->type )
				     || ( PERCENT_BOMB == best_setup_weap->type ) ) {
					use_freeing_tool( false, true );

					// If this is a riot bomb, flatten the angle,
					// but only if the best overshoot (we took the
					// angle and power from its setup) was too long.
					// No use in firing a riot bomb behind the opponent
					if ( best_setup_overshoot > 0 ) {
						flatten_curr_ang();
					}
				} else {
					// In this case use the current weapon, but go a bit down
					// with the angle:
					int32_t ang_mod = 5 + RAND_AI_1P;

					if ( curr_angle < 180 ) {
						curr_angle -= ang_mod;
						if ( curr_angle < 95 ) {
							curr_angle = 95;
						}
					} else {
						curr_angle += ang_mod;
						if ( curr_angle > 265 ) {
							curr_angle = 265;
						}
					}

					if ( ( REDUCER != best_setup_weap->type ) && ( PERCENT_BOMB != best_setup_weap->type )
					     && ( ( RIOT_BOMB > best_setup_weap->type ) || ( RIOT_BLAST < best_setup_weap->type ) ) ) {
						use_weapon( best_setup_weap->type );
					} else {
						use_weapon( SML_MIS );
					}
				} // end of using best setup weapon.
			}

			sanitize_curr();
			angle           = curr_angle;
			power           = curr_power;
			need_aim        = false;
			best_setup_weap = nullptr;
			is_blocked      = true;

			show_feedback( "???", PURPLE, -.6, TS_NO_SWAY, 100 );
		}


		// ----------------------------------------
		// --- Write back the best attack setup ---
		// ----------------------------------------
		if ( !is_stopped && need_aim && !is_blocked ) {
			curr_angle = best_setup_angle;
			curr_power = best_setup_power;
			item_curr  = best_setup_weap ? nullptr : best_setup_item;
			mem_curr   = best_setup_mem;
			weap_curr  = item_curr ? nullptr : best_setup_weap;
			weap_idx   = weap_curr ? weap_curr->type : item_curr ? item_curr->type + WEAPONS : 0;

			sanitize_curr();
			angle = curr_angle;
			power = curr_power;

			DEBUG_LOG_AIM(
				player->get_name(),
				"Using best setup with angle %d, power %d using %s (Score %d)",
				GET_DISP_ANGLE( angle ),
				power,
				weap_idx < WEAPONS ? weapon[weap_idx].get_name() : item[weap_idx - WEAPONS].get_name(),
				best_setup_score
			)
		} else if ( !is_stopped ) {
			// Note: Without aiming or when blocked, the setup memory
			//       was not used in some cases.
			curr_angle = angle;
			curr_power = power;
			sanitize_curr();
			angle = curr_angle;
			power = curr_power;
		}


		// ---------------------------------------------------------
		// --- For the bot to yell out a retaliation phrase, the ---
		// --- following conditions must be true:                ---
		// --- 1) A weapon is chosen                             ---
		// --- 2) The primary target must be hit                 ---
		// --- 3 a) The target is the revengee and               ---
		// --- 3 b) the damage is at least 10% per AI level or   ---
		// --- 4 a) the target is not the revengee and           ---
		// --- 4 b) the damage is at least 20% per AI level      ---
		// ---------------------------------------------------------
		int32_t min_rev_dmg = best_setup_mem ? ROUND( best_setup_mem->opLife * ( ai_level - RAND_AI_0P ) / 10. ) : 0;
		int32_t min_oth_dmg = best_setup_mem ? ROUND( best_setup_mem->opLife * ( ai_level - RAND_AI_0P ) / 5. ) : 0;
		if ( !is_stopped && !global.skipping_computer_play                    // allowed to issue texts
		     && weap_curr && need_aim && !need_success                        // (1) targeting was successful
		     && best_setup_prime                                              // (2) primary target gets damage
		     && ( ( revengee && ( revengee == best_setup_mem->entry )         // (3 a) revengee targeted
		            && ( best_setup_damage >= min_rev_dmg ) )                 // (3 b) enough damage done
		          || ( ( !revengee || ( revengee != best_setup_mem->entry ) ) // (4 a) not the revengee
		               && ( best_setup_damage >= min_oth_dmg ) ) ) ) {        // (4 b) enough damage done
			char const* text = player->select_retaliation_phrase();
			try {
				if ( text ) {
					// Wait for the AI to be allowed to create texts
					while ( !text_allowed.load( ATOMIC_READ ) ) {
						std::this_thread::yield();
					}

					// Now create the instance
					new CFloatText( text, x, y - 30., .0, -.4, player->color, CENTRE, TS_NO_SWAY, 150, false );
				}
			} catch ( std::exception& e ) {
				std::cerr << __func__ << " new CFloatText: " << e.what() << std::endl;
			}
			if ( text ) {
				free( const_cast< char* >( text ) );
			}
		}


		// -------------------------------------------------
		// --- Tell the world this tank is going bye bye ---
		// -------------------------------------------------
		if ( !is_stopped && mem_curr && mem_curr->entry && ( mem_curr->entry->opponent == player )
		     && !global.skipping_computer_play ) {
			try {
				// Wait for the AI to be allowed to create texts
				while ( !text_allowed.load( ATOMIC_READ ) ) {
					std::this_thread::yield();
				}

				// Now create it
				new CFloatText( CPlayer::select_kamikaze_phrase(), x, y - 30, .0, -.4, player->color, CENTRE, TS_NO_SWAY, 300, false );
			} catch ( std::exception& e ) {
				std::cerr << __func__ << " new CFloatText: " << e.what() << std::endl;
			}
		}


		// ---------------------------------------
		// --- Apply some "last second" errors ---
		// ---------------------------------------
		if (
			!is_stopped && need_aim && !is_blocked    // Don't temper with blocked shots, makes no sense
			                                          // And assume that bots can 'fix' errors from the last round:
		        && ( ( nullptr == mem_curr )              // No current aopponent set or ...
		             || ( mem_curr->entry != last_opp ) ) // ...current opponent is not the same as last round
		        && RAND_AI_1N                             // But don't let them "fumble" too often.
		) {
			int32_t ang_mod = ( get_rand() % 6 ) + 3; // [ 3; 8]
			int32_t pow_mod = ( ang_mod * 10 ) + 1;   // [31;81]
			double  ang_err = get_rand() % ang_mod;   // [ 1; 6]
			double  pow_err = get_rand() % pow_mod;   // [10;35]

			// Angles always go 'up', but never over the top
			if ( angle > ( 180. + ang_err ) ) {
				curr_angle = ROUND( angle - ( error_multiplier * ang_err ) );
			} else if ( angle < ( 180. - ang_err ) ) {
				curr_angle = ROUND( angle + ( error_multiplier * ang_err ) );
			}

			// Power error is always a raise
			curr_power = ROUND( power + ( error_multiplier * pow_err ) );

			sanitize_curr();

			DEBUG_LOG_AIM(
				player->get_name(),
				"Last second errors: Angle %d° -> %d°), Power: %d -> %d)",
				GET_DISP_ANGLE( angle ),
				GET_DISP_ANGLE( curr_angle ),
				power,
				curr_power
			)

			show_feedback( "*fumble*", RED, -.8, TS_NO_SWAY, 100 );

			angle = curr_angle;
			power = curr_power;
		}
		assert( ( angle == curr_angle ) && "ERROR: Finished but angle not set!" );
		assert( ( power == ( curr_power - ( curr_power % 5 ) ) ) && "ERROR: Finished but power not set!" );
		assert( ( ( weap_idx >= WEAPONS ) || ( 0 == weapon[weap_idx].warhead ) ) && "ERROR: Not usable warhead chosen!" );
		assert( ( weap_idx >= 0 ) && ( weap_idx < THINGS ) && env.is_item_available( weap_idx )
		        && "ERROR: Unavailable or invalid weap_idx!" );
		assert( ( ( weap_idx >= WEAPONS ) || ( player->nm[weap_idx] > 0 ) ) && "ERROR: Weapon chosen that is out of stock!" );
		assert( ( ( weap_idx < WEAPONS ) || ( player->ni[weap_idx - WEAPONS] > 0 ) ) && "ERROR: Item chosen that is out of stock!" );
		assert( ( ( weap_idx < WEAPONS ) || ( ( weap_idx - WEAPONS ) < ITEM_LGT_SHIELD ) || ( ( weap_idx - WEAPONS ) == ITEM_FUEL )
		          || ( ( weap_idx - WEAPONS ) == ITEM_ROCKET ) )
		        && "ERROR: The chosen item is not usable!" );


		// ---------------------------------------
		// --- Wait for the weapon to be fired ---
		// ---------------------------------------
		if ( !is_stopped ) {
			pl_stage = PS_FIRE; // It can be fired now

			DEBUG_LOG_AI(
				player->get_name(),
				"Finished thinking, waiting to fire %s against %s",
				weap_curr ? weapon[weap_idx].get_name() : item[weap_idx - WEAPONS].get_name(),
				mem_curr ? mem_curr->entry->opponent->get_name() : "Nobody"
			)

			action_condition.wait( actionLock, [this] {
				return ( !allow_work || !is_working || is_stopped || ( PS_CLEANUP == pl_stage ) );
			} );
		}


		// --------------------------------------
		// --- Remember the current selection ---
		// --- (But only if it was hit)       ---
		// --------------------------------------
		if ( !is_stopped && best_setup_prime && ( best_setup_mem == mem_curr ) ) {
			player->set_last_opponent( mem_curr ? mem_curr->entry : nullptr );
		} else {
			player->set_last_opponent( nullptr );
		}

		DEBUG_LOG_AI( player->get_name(), "Cleaning up...", 0 )

		// --------------------
		// ---   Clean up   ---
		// --------------------
		angle          = 180;
		power          = MAX_POWER / 2;
		curr_angle     = 180;
		curr_power     = MAX_POWER / 2;
		curr_overshoot = MAX_OVERSHOOT;
		weap_idx       = SML_MIS;
		blast_big      = 0.;
		blast_max      = 0.;
		blast_med      = 0.;
		blast_min      = 0.;
		text_allowed.store( false, ATOMIC_WRITE );

		// Note: There is no need to clean up the memory chain.
		// It is only created once, all players have the same
		// size, and the get_memory() method reuses an existing
		// one.
		player = nullptr;
		tank   = nullptr;

		// Eventually signal that the work has finished.
		pl_stage   = PS_AI_IS_IDLE;
		is_working = false;
	} // end of not being stopped

	is_finished = true;
}

/// =========================================
/// === Helper list entry implementations ===
/// =========================================


/// @brief explicit constructor adding the instance to the list
sItemListEntry::sItemListEntry( sItemListEntry* prev_ ) : prev( prev_ ) {
	if ( prev ) {
		next       = prev->next;
		prev->next = this;

		if ( next ) {
			next->prev = this;
		}
	}
}

/// @brief The destructor removes the element from the list
sItemListEntry::~sItemListEntry() {
	if ( prev ) {
		prev->next = next;
		prev       = nullptr;
	}
	if ( next ) {
		next->prev = prev;
		next       = nullptr;
	}
}

/// @brief explicit constructor adding the instance to the list
sOppMemEntry::sOppMemEntry( sOppMemEntry* prev_ ) : prev( prev_ ) {
	if ( prev ) {
		next       = prev->next;
		prev->next = this;

		if ( next ) {
			next->prev = this;
		}
	}
}

/// @brief The destructor removes the element from the list
sOppMemEntry::~sOppMemEntry() {
	if ( prev ) {
		prev->next = next;
		prev       = nullptr;
	}
	if ( next ) {
		next->prev = prev;
		next       = nullptr;
	}
	entry = nullptr;
}

/// @brief explicit constructor adding the instance to the list
sWeapListEntry::sWeapListEntry( sWeapListEntry* prev_ ) : prev( prev_ ) {
	if ( prev ) {
		next       = prev->next;
		prev->next = this;

		if ( next ) {
			next->prev = this;
		}
	}
}

/// @brief The destructor removes the element from the list
sWeapListEntry::~sWeapListEntry() {
	if ( prev ) {
		prev->next = next;
		prev       = nullptr;
	}
	if ( next ) {
		next->prev = prev;
		next       = nullptr;
	}
}
