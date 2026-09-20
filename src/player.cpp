/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2003  Thomas Hudson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * */


#include "player.h"

#include "aicore.h"
#include "environment.h"
#include "files.h"
#include "floattext.h"
#include "globaldata.h"
#include "menu.h"
#include "network.h"
#include "random.h"
#include "tank.h"

#include <cassert>
#include <sstream>

/// Static helper values to help with keyboard controls:
static bool ctrlUsedUp        = false;
static bool has_ctrl_pressed  = false;
static bool has_shift_pressed = false;

/// @brief default ctor
CPlayer::CPlayer() {

	// 25% of the time set to perplay weapon preferences
	pref_type = ( get_rand() % 4 ) ? ALWAYS_PREF : PERPLAY_PREF;

	/* Generate a set of preferences now. The reason is:
	 * If the player is a PERPLAY_PREF type player, no preferences are loaded from the atanks configuration file.
	 * But if the user changes the type to ALWAYS_PREF and starts a new game, no new preferences are generated. So then
	 * these are used, which is safe enough.
	 */
	generate_preferences();

	switch ( get_rand() % 4 ) {
		case 0: // === red type ===
			color = makecol( 200 + ( get_rand() % 56 ), get_rand() % 25, get_rand() % 25 );
			break;
		case 1: // === green type ===
			color = makecol( get_rand() % 25, 200 + ( get_rand() % 56 ), get_rand() % 25 );
			break;
		case 2: // === blue type ===
			color = makecol( get_rand() % 25, get_rand() % 25, 200 + ( get_rand() % 56 ) );
			break;
		case 3:
		default: // === violet type ===
			color = makecol( 200 + ( get_rand() % 56 ), get_rand() % 25, 200 + ( get_rand() % 56 ) );
			break;
	}
}

/// @brief default dtor
CPlayer::~CPlayer() {
	if ( tank ) {
		delete ( tank );
		tank = nullptr;
	}

	if ( opponents ) {
		delete[] opponents;
		opponents = nullptr;
	}
}

/// @brief specifically boost amp preference item[idx] from @arg old_pref using @arg ai_level
double CPlayer::boost_amp_pref( double old_pref, int32_t idx [[maybe_unused]], int32_t ai_level ) const {
	double pref  = old_pref;
	double boost = 1. + ( ( -1. * defensive + 2. + RAND_AI_10P /* [1;12] */ ) / 10. );
	if ( pref < 1. ) {
		pref = 1.;
	}
	DEBUG_LOG_FIN( name.c_str(), "Boost %s : %3.2f * %3.2f = %3.2f", item[ idx ].getName(), pref, boost, pref * boost )
	pref *= boost;
	return pref;
}

/// @brief specifically boost armour preference item[idx] from @arg old_pref using @arg ai_level
double CPlayer::boost_armour_pref( double old_pref, int32_t idx [[maybe_unused]], int32_t ai_level ) const {
	double pref  = old_pref;
	double boost = 1. + ( ( defensive + 2. + RAND_AI_10P /* [1;12] */ ) / 10. );
	if ( pref < 1. ) {
		pref = 1.;
	}
	DEBUG_LOG_FIN( name.c_str(), "Boost %s : %3.2f * %3.2f = %3.2f", item[ idx ].getName(), pref, boost, pref * boost )
	pref *= boost;
	return pref;
}

/// @brief update currPrefs array with considering needs and stock amounts
void CPlayer::boost_prefences( bool boost_armour, bool boost_amps, bool boost_weapons ) {
	auto ai_level = static_cast< int32_t >( type );

	for ( int32_t i = 1; i < THINGS; ++i ) {
		double pref = curr_pref[ i ];

		// Lower weapon preferences if there is enough in stock already
		if ( i && ( i < WEAPONS ) ) {
			auto   delay_div  = static_cast< double >( weapon[ i ].getDelayDiv() );
			auto   cur_amount = ROUND( nm[ i ] / delay_div );
			double one_amount = weapon[ i ].amt / delay_div;
			double max_amount = one_amount * ai_level;
			double div_amount = cur_amount - max_amount;

			// - cur_amount is the total amount of single shots. getDelayDiv()
			// is used, because it simply returns the number of shots fired by
			// delayed weapons, while it returns always 1 for the other weapons.
			// - one_amount - The number of nm[i] that is gotten by buying one
			//                unit.
			// - max_amount - below this no reduction or sale is considered.
			// - div_amount - if positive, the bot has enough in stock.
			//              - if larger than one_amount, selling the excess
			//                amount is considered.

			if ( boost_weapons && i && ( i < WEAPONS ) ) {
				double boost = 1. + ( ( std::abs( defensive ) + RAND_AI_10P /* [0;10] */ ) / 10. );
				if ( pref < 1. ) {
					pref = 1.;
				}
				DEBUG_LOG_FIN(
					name.c_str(),
					"Boost %s : %3.2f * %3.2f = %3.2f",
					weapon[ i ].getName(),
					pref,
					boost,
					pref * boost
				)
				pref *= boost;
			}

			if ( ( div_amount > 1. ) && ( pref >= 1. ) ) {
				pref /= div_amount;
				DEBUG_LOG_FIN(
					name.c_str(),
					"Lower %s pref (%d in stock) %d -> %d",
					weapon[ i ].getName(),
					ROUND( cur_amount ),
					curr_pref[ i ],
					ROUND( pref )
				)

				if ( env.sell_percent > 0.01 ) {

					// saleable are the units considered to be sold:
					int32_t saleable = ROUND( div_amount - RAND_AI_1P ) / one_amount;

					if ( saleable > 0 ) {
						money   += ROUND( weapon[ i ].cost * env.sell_percent * saleable );
						nm[ i ] -= weapon[ i ].amt * saleable;
						DEBUG_LOG_FIN(
							name.c_str(),
							"Sold %d %s for $%s",
							saleable,
							weapon[ i ].getName(),
							Add_Comma( ROUNDu( weapon[ i ].cost * env.sell_percent ) * saleable )
						)
					}
				} // end of selling allowed
			}         // end of having enough in stock
		}                 // end of watching weapons

		// Lower item preferences if there is enough in stock already
		if ( i >= WEAPONS ) {
			int32_t j = i - WEAPONS;
			if ( ( j < ITEM_ARMOUR ) || ( j > ITEM_VIOLENT_FORCE ) ) {
				double cur_amount = ni[ j ];
				double one_amount = item[ j ].amt;
				double max_amount = one_amount * ai_level;

				// Note: The values are the same as above.

				// Repair kit and SDI are limited differently
				if ( ITEM_REPAIRKIT == j ) {
					max_amount *= pain_sensitivity + defensive + 2.;
				} else if ( ITEM_SDI == j ) {
					max_amount *= defensive + 2. + ( ai_level / 2. );
				}

				double div_amount = cur_amount - max_amount;

				if ( boost_armour && ( ( WEAPONS + ITEM_ARMOUR ) <= i ) && ( ( WEAPONS + ITEM_PLASTEEL ) >= i ) ) {
					pref = boost_armour_pref( pref, j, ai_level );
				}

				if ( boost_amps && ( ( WEAPONS + ITEM_INTENSITY_AMP ) <= i )
				     && ( ( WEAPONS + ITEM_VIOLENT_FORCE ) >= i ) ) {
					pref = boost_amp_pref( pref, j, ai_level );
				}

				if ( ( div_amount > 1. ) && ( pref >= 1. ) ) {
					pref /= div_amount;
					DEBUG_LOG_FIN(
						name.c_str(),
						"Lower %s pref (%d in stock) %d -> %d",
						item[ j ].getName(),
						ROUND( cur_amount ),
						curr_pref[ i ],
						ROUND( pref )
					)

					if ( ( env.sell_percent > 0.01 ) && ( j < ITEM_VENGEANCE ) ) {

						// Note: Armour and Amps are not considered here!
						auto saleable = ROUND( ( div_amount - RAND_AI_1P ) / one_amount );

						if ( saleable > 0 ) {
							money   += ROUND( item[ j ].cost * env.sell_percent * saleable );
							ni[ j ] -= item[ j ].amt * saleable;
							DEBUG_LOG_FIN(
								name.c_str(),
								"Sold %d %s for $%s",
								saleable,
								item[ j ].getName(),
								Add_Comma( ROUND( item[ j ].cost * env.sell_percent * saleable ) )
							)
						}
					} // end of selling allowed
				}         // end of having enough in stock
			}                 // end of item type limitation
		}                         // end of being in items range

		// Write back preferences:
		curr_pref[ i ] = ROUND( pref );
	}
}

/** @brief Buy item with index @a item_index
 * An item has been selected, this function merely buys it. It
 * first does checks to make sure the item can be bought.
 * The function returns true if we successfully bought the item or
 * false if we could not get it for some reason.
 **/
bool CPlayer::buy_item( int32_t item_index, int32_t max_boost ) {
	bool bought = false;

	if ( item_index < WEAPONS ) {
		// The three things to test:
		// 1: Enough money?
		// 2: Space free in stock?
		// 3: Tech level not too high?
		if ( ( money >= weapon[ item_index ].cost ) && ( nm[ item_index ] < MAX_ITEMS_IN_STOCK )
		     && ( weapon[ item_index ].techLevel <= env.weapontech_level ) ) {
			money           -= weapon[ item_index ].cost;
			nm[ item_index ] += weapon[ item_index ].amt;

			// don't allow more than MAX_ITEMS_IN_STOCK
			if ( nm[ item_index ] > MAX_ITEMS_IN_STOCK ) {
				nm[ item_index ] = MAX_ITEMS_IN_STOCK;
			}
			bought = true;
		}
	} // end of buying a weapon

	else {

		// Items need an additional check:
		// The purchase of boost items is limited by
		// both AI type and overall boost level.
		// The same applies to shields

		auto    ai_level = static_cast< int32_t >( type );
		int32_t itemNum  = item_index - WEAPONS;
		bool    isBoost  = ( ( itemNum >= ITEM_ARMOUR ) && ( itemNum <= ITEM_VIOLENT_FORCE ) );
		bool    isShield = ( ( itemNum >= ITEM_LGT_SHIELD ) && ( itemNum <= ITEM_HVY_REPULSOR_SHIELD ) );

		if ( ( money > item[ itemNum ].cost ) && ( ni[ itemNum ] < MAX_ITEMS_IN_STOCK ) && env.is_item_available( itemNum )
		     && ( ( HUMAN_PLAYER == type ) || !( isBoost || isShield )
		          || ( isBoost && ( ai_level > boost_bought ) && ( get_boost_value() < max_boost ) )
		          || ( isShield && ( ai_level > shield_bought ) ) ) ) {
			money         -= item[ itemNum ].cost;
			ni[ itemNum ] += item[ itemNum ].amt;

			// Count it if it was a boost item
			if ( isBoost ) {
				boost_bought++; // Okay, take it!
			}

			// Count it if it was a shield
			if ( isShield ) {
				shield_bought++; // Okay, same procedure
			}

			// don't allow more than MAX_ITEMS_IN_STOCK
			if ( ni[ itemNum ] > MAX_ITEMS_IN_STOCK ) {
				ni[ itemNum ] = MAX_ITEMS_IN_STOCK;
			}
			bought = true;
		}
	}

	// Reset boost_pref if we bought the item
	if ( bought ) {
		boost_pref[ item_index ] = 0;
	}

	return bought;
}

/// @brief call this after loading a game to ensure backwards compatibility
void CPlayer::check_opp_mem() {
	if ( !opp_count ) {
		new_game();
	}
}

/// @brief Have the AI choosing something to buy.
int32_t CPlayer::choose_item_to_buy( int32_t max_boost, int32_t& last_idx ) {

	// Do not do this if there is no money:
	if ( money < 1000 ) {
		return -1;
	}

	// Possibly pre-select an item by checking the current situation:
	int32_t currItem = computer_select_pre_buy_item( max_boost );

	// Be done already if the pre-selection provided a "must have"
	if ( ( currItem > 0 ) && buy_item( currItem, max_boost ) ) {
		return currItem;
	}

	// Loop through the wish list and try to buy something.
	// The more of the item is in the inventory, the less likely
	// it is that the AI tries to buy the item. If the inventory
	// is empty, the chance is 50% for a useless bot and 88% for
	// a deadly bot.
	// There do not need to be any further modifications, the
	// preferences are already tweaked by defensiveness and
	// AI level.
	auto ai_level = static_cast< int32_t >( type );
	for ( ; last_idx < THINGS; ++last_idx ) {

		currItem = desired[ last_idx ];

		// Skip small missile, restart instead
		if ( !currItem ) {
			last_idx = 0;
			// Note: Small missiles are sorted to the end of the list.
			continue;
		}

		// Skip unaffordable items
		if ( ( ( currItem < WEAPONS ) && ( weapon[ currItem ].cost > money ) )
		     || ( ( currItem >= WEAPONS ) && ( item[ currItem - WEAPONS ].cost > money ) ) ) {
			continue;
		}

		// Now take the chance
		int32_t amount  = currItem < WEAPONS ? weapon[ currItem ].amt : item[ currItem - WEAPONS ].amt;
		int32_t maxAmt  = amount * ai_level;
		double  currAmt = currItem < WEAPONS ? nm[ currItem ] : ni[ currItem - WEAPONS ];
		double  newAmt  = currAmt + amount;
		double  amtMod  = currAmt > 0. ? newAmt / currAmt : 2.;
		// Note: The more items there are already, the more amtMod
		// will go down near 1.0, from a maximum of 2.0.

		int32_t chance = ROUNDu( amtMod * ( static_cast< double >( ai_level ) - .5 ) );
		/* Results:
		 * Useless : 1 * (1 - 0.5) = 1 * (0.5) = 0.5 => 50% (rounded to 1)
		 * Useless : 2 * (1 - 0.5) = 2 * (0.5) = 1   => 50%
		 * Deadly  : 1 * (5 - 0.5) = 1 * (4.5) = 4.5 => 80% (rounded to 5)
		 * Deadly  : 2 * (5 - 0.5) = 2 * (4.5) = 9   => 87.5%
		 */

		// Bots have far higher limits for repair units and SDIs:
		if ( ( ITEM_SDI == ( currItem - WEAPONS ) ) || ( ITEM_REPAIRKIT == ( currItem - WEAPONS ) ) ) {
			maxAmt *= 10;
		}

		/* The bot tries to buy the item if:
		 * a) It does not have any and the chance is taken, or
		 * b) It has equal or more than the weapons amount times its level and
		 *    a negative (low) chance against its ai_level is taken.
		 */
		if ( ( ( ( currAmt < maxAmt ) && ( get_rand() % ( chance + 1 ) ) ) /* Scenario a) */
		       || ( ( currAmt >= maxAmt ) && RAND_AI_0N ) )                /* Scenario b) */
		     && buy_item( currItem, max_boost ) ) {

			// Advance index to not buy the same item over and over again
			if ( RAND_AI_1P ) {
				++last_idx;
			}

			return currItem;
		}
	}

	return -1;
}

EControl CPlayer::computer_controls( CAICore* aicore, bool allow_fire ) {
	// Don't act at all when in scoreboard or endgame stage
	if ( STAGE_SCOREBOARD <= global.stage ) {
		return CONTROL_NONE;
	}

	int32_t       ai_weap    = tank ? tank->cw : SML_MIS;
	int32_t       ai_angle   = 0;
	int32_t       ai_power   = 0;
	EPlayerStages ai_stage   = PS_STAGE_COUNT;
	bool          is_working = aicore->status( ai_weap, ai_angle, ai_power, ai_stage );

	// If the AI is working with a different player or the AI is dead, return
	if ( !aicore->can_work() || ( is_working && ( this != aicore->active_player() ) ) ) {
		return CONTROL_NONE;
	}

	// Do not try to start the AI if this players tank is
	// about to be destroyed or while it is moving.
	if ( !tank || tank->destroy || tank->is_flying() || ( tank->l < 1 ) ) {
		return CONTROL_NONE;
	}

	tank->require_update();

	/* Start the AI for this player if:
	 * 1) The AI is idle and
	 * 2) the game is in aiming stage and
	 * 3) this player still has a tank that (checked above)
	 * 4) is not about to get destroyed and (checked above)
	 * 5) stands still on the ground.       (checked above)
	 */
	if ( ( PS_AI_IS_IDLE == ai_stage ) && ( STAGE_AIM == global.stage ) ) {

		if ( aicore->start( this ) ) {
			return CONTROL_NONE;
		} else {
			cerr << "FATAL: Can not start idle AI with this player!" << endl;
			global.set_command( GLOBAL_COMMAND_MENU );
			return CONTROL_QUIT;
		}
	}

	// Return at once if the AI is still initializing or cleaning up
	else if ( ( PS_AI_INITIALIZE == ai_stage ) || ( PS_CLEANUP == ai_stage ) ) {
		return CONTROL_NONE;
	}

	// If the AI is not working (yet), return
	if ( !is_working ) {
		return CONTROL_NONE;
	}

	// Now, being here, the ai is working on this very player.

	// Copy stage now, as it is this players stage as well
	pl_stage = ai_stage;

	// Sanitize AI values:
	// Note: None of these should ever kick in!
	assert( ( ai_angle >= 90 ) && "ERROR: AI set too low angle!" );
	assert( ( ai_angle <= 270 ) && "ERROR: AI set too high angle!" );
	assert( ( ai_power >= 0 ) && "ERROR: AI set too low power!" );
	assert( ( ai_power <= MAX_POWER ) && "ERROR: AI set too high power!" );
	assert( ( 0 == ( ai_power % 5 ) ) && "ERROR: AI set non mod 5 power!" );
	assert( ( ( ai_weap < 0 ) /* unset ! */
	          || ( ( ai_weap < WEAPONS ) && nm[ ai_weap ] > 0 ) || ( ( ai_weap >= WEAPONS ) && ni[ ai_weap - WEAPONS ] > 0 ) )
	        && "ERROR: AI set weapon that has a zero stock!" );
	if ( ai_angle < 90 ) {
		ai_angle = 90;
	}
	if ( ai_angle > 270 ) {
		ai_angle = 270;
	}
	if ( ai_power < 0 ) {
		ai_power = 0;
	}
	if ( ai_power > MAX_POWER ) {
		ai_power = MAX_POWER;
	}
	ai_power -= ai_power % 5;
	if ( ( ( ai_weap < WEAPONS ) && nm[ ai_weap ] <= 0 ) || ( ( ai_weap >= WEAPONS ) && ni[ ai_weap - WEAPONS ] <= 0 ) ) {
		ai_weap = tank->cw;
	}

	// Only put anything on the screen if this is the firing stage
	if ( PS_FIRE == pl_stage ) {

		// If there is a difference between the AI selection and the
		// display, transport the values on the screen.
		if ( ( ai_angle != tank->a ) || ( ai_power != tank->p ) || ( ai_weap != tank->cw ) ) {
			global.update_menu = true;

			if ( global.skipping_computer_play ) {
				// When skipping, the values are simply copied:
				tank->a  = ai_angle;
				tank->p  = ai_power;
				tank->cw = ai_weap;
			} else {
				// Transfer angle:
				if ( ai_angle > tank->a ) {
					++tank->a;
				} else if ( ai_angle < tank->a ) {
					--tank->a;
				}

				// Transfer power:
				if ( ai_power > tank->p ) {
					tank->p += 5;
				} else if ( ai_power < tank->p ) {
					tank->p -= 5;
				}

				// Transfer weapon information:
				if ( ai_weap != tank->cw ) {
					changed_weapon  = false;

					int32_t cw_mod  = tank->cw < ai_weap ? 1 : -1;
					tank->cw       += cw_mod;

					// Skip unusable items and those that are
					// out of stock
					while ( ( !env.is_item_available( tank->cw ) || ( ( tank->cw < WEAPONS ) && !nm[ tank->cw ] )
					          || ( ( tank->cw >= WEAPONS ) && !ni[ tank->cw - WEAPONS ] ) )
					        && ( tank->cw != ai_weap ) ) {
						tank->cw += cw_mod;
					}
				}
			} // End of regular transfer
		}         // End of transferring difference

		// otherwise, fire the current weapon if this is allowed
		else if ( allow_fire ) {
			aicore->weapon_fired();
			global.update_menu = true;

			if ( type == VERY_PART_TIME_BOT ) {
				type = NETWORK_CLIENT;
			}

			gloating = false;
			pl_stage  = PS_CLEANUP;
			return CONTROL_FIRE;
		}
	} // End of handling firing stage

	// If the AI wants to move their tank, do so:
	else if ( PS_MOVE_LEFT == pl_stage ) {
		if ( tank->move_tank( DIR_LEFT ) ) {
			aicore->has_moved( DIR_LEFT );
			return CONTROL_OTHER;
		} else {
			aicore->has_moved( 0 ); // No movement possible
		}
	} else if ( PS_MOVE_RIGHT == pl_stage ) {
		if ( tank->move_tank( DIR_RIGHT ) ) {
			aicore->has_moved( DIR_RIGHT );
			return CONTROL_OTHER;
		} else {
			aicore->has_moved( 0 ); // No movement possible
		}
	}

	return CONTROL_NONE;
}

int32_t CPlayer::computer_select_pre_buy_item( int32_t max_boost ) {
	auto   max_level = static_cast< int32_t >( DEADLY_PLAYER );
	auto   ai_level  = static_cast< int32_t >( type );
	double mood = 1. + defensive + ( ( static_cast< double >( get_rand() ) / ( static_cast< double >( RAND_MAX ) / 2. ) ) );
	// mood is 0.0 <= x <= 4.0

	/*	Prior buying anything else, a 5 step system takes place:
	 * 1.: Parachutes (if gravity is on)
	 * 2.: Minimum weapon probability (aka 5 medium and 3 large missiles)
	 * 3.: The most expensive item from the save_money_for list
	 * 4.: Armour/Amps
	 * 5.: "Tools" to free themselves like Riot Blasts
	 * 6.: Shields, if enough money is there
	 * 7.: Fuel, everybody shall have at least 100 units.
	 * 8.: if all is set, look for dimpled/slick projectiles!
	 */


	// Step 1: Check for parachutes (if the bot remembers to check)
	if ( ( ( type >= RANGEFINDER_PLAYER ) || RAND_AI_1P ) && ( env.landslide_type > SLIDE_NONE )
	     && ( ni[ ITEM_PARACHUTE ] < 10 ) && ( money > item[ ITEM_PARACHUTE ].cost ) ) {

		DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Parachute", 0 )
		return ( WEAPONS + ITEM_PARACHUTE );
	}


	// Step 2: Minimum range of damaging weapons:
	// To be fair, this is always done and never forgotten.
	if ( ( nm[ LRG_MIS ] < 3 ) && ( money >= weapon[ LRG_MIS ].cost ) ) {

		DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Large Missile", 0 )
		return LRG_MIS;
	}

	if ( ( nm[ MED_MIS ] < 5 ) && ( money >= weapon[ MED_MIS ].cost ) ) {

		DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Medium Missile", 0 )
		return MED_MIS;
	}


	// Step 3: Check weapons/items currently saving money for:
	int32_t saved_cost = 0;
	int32_t saved_item = 0;
	for ( int32_t i = 0; i < THINGS; ++i ) {
		if ( ( save_money_for[ i ] > 0 ) && ( save_money_for[ i ] < money ) && ( save_money_for[ i ] > saved_cost ) ) {
			saved_cost = save_money_for[ i ];
			saved_item = i;
		}
	}
	// Got one?
	if ( saved_item > 0 ) {
		DEBUG_LOG_FIN(
			name.c_str(),
			"Finally got enough money for %s!",
			saved_item < WEAPONS ? weapon[ saved_item ].getName() : item[ saved_item - WEAPONS ].getName()
		)
		// Take it out from the wish list:
		save_money_for[ saved_item ] = 0;
		return saved_item;
	}


	// Step 4: Check for Armour / Amps (if the bot remembers to check)
	if ( ( boost_bought < ai_level ) && RAND_AI_1P ) {
		int32_t boost_value = get_boost_value();
		int32_t boost_limit = max_boost - boost_value;
		int32_t armour_val  = get_armour_value();
		int32_t amp_val     = get_amp_value();


		if ( ( boost_limit > ( max_level - ai_level + 1 ) ) && ( !need_damage || RAND_AI_0P ) ) {

			DEBUG_LOG_FIN(
				name.c_str(),
				"Pre-Check: Max Boost %d, Armour %d, Amp %d, Limit %d",
				max_boost,
				armour_val,
				amp_val,
				boost_limit
			)

			// See which is preferred:
			boost_limit = max_boost - ( 2 * ( DEADLY_PLAYER + 1 ) ) + RAND_AI_0P;

			if ( boost_value < boost_limit ) {
				double amp_mood = amp_val - mood + 2.;
				double arm_mood = armour_val + mood + 2.;
				if ( amp_mood < arm_mood ) {
					need_amp    = true;
					need_armour = false;
				} else {
					need_amp    = false;
					need_armour = true;
				}
				DEBUG_LOG_FIN(
					name.c_str(),
					"=> need %s: boost %d/%d, amp %3.2f %s %3.2f arm",
					need_amp ? "Amp" : "armor",
					boost_value,
					boost_limit,
					amp_mood,
					amp_mood < arm_mood ? "<" : ">",
					arm_mood
				)
			}


			// Prefer armour if the mood is defensive and armour isn't too
			// far ahead from the amps:
			if ( need_armour || ( ( mood >= 2.0 ) && ( armour_val <= amp_val ) ) ) {
				// The player is in a defensive mood or armour has fallen behind
				// If we have 25% more money than the plasteel cost, buy it,
				// else the armour will do. If the armour is far behind, no
				// money is spared.
				if ( ( money >= ( item[ ITEM_PLASTEEL ].cost * 1.25 ) )
				     || ( ( ( armour_val < ( amp_val * 0.5 ) ) || !armour_val )
				          && ( money > item[ ITEM_PLASTEEL ].cost ) ) ) {

					DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Plasteel Plating", 0 )
					return ( WEAPONS + ITEM_PLASTEEL );
				}

				if ( ( money >= ( item[ ITEM_ARMOUR ].cost * 2.0 ) )
				     && ( ni[ ITEM_ARMOUR ] < ni[ ITEM_PLASTEEL ] ) && ( mood >= 3.5 ) ) {

					DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Armour", 0 )
					return ( WEAPONS + ITEM_ARMOUR );
				}

				// If nothing could be selected, at least boost what we need for the next round
				int32_t i     = ITEM_PLASTEEL + WEAPONS;
				int32_t j     = ITEM_PLASTEEL;
				curr_pref[ i ] = ROUND( boost_armour_pref( curr_pref[ i ], j, ai_level ) );

			} // end of armour check

			// Otherwise go for a shining new amp:
			if ( need_amp || ( ( mood <= 2.0 ) && ( amp_val <= armour_val ) ) ) {
				// The player is in a offensive mood or amps have fallen behind
				// If we have 25% more money than the violent force cost, buy
				// it, else the normal amp will do.
				// If the amps have fallen behind too much, no money is spared.
				if ( ( money >= ( item[ ITEM_VIOLENT_FORCE ].cost * 1.5 ) )
				     || ( ( ( amp_val < ( armour_val * 0.5 ) ) || !amp_val )
				          && ( money > item[ ITEM_VIOLENT_FORCE ].cost ) ) ) {

					DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Violent Force", 0 )
					return ( WEAPONS + ITEM_VIOLENT_FORCE );
				}

				if ( ( money >= ( item[ ITEM_INTENSITY_AMP ].cost * 1.75 ) )
				     && ( ni[ ITEM_INTENSITY_AMP ] < ni[ ITEM_VIOLENT_FORCE ] ) && ( mood < 1.0 ) ) {

					DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Intensity Amp", 0 )
					return ( WEAPONS + ITEM_INTENSITY_AMP );
				}

				// If nothing could be selected, at least boost what we need for the next round
				int32_t i     = ITEM_VIOLENT_FORCE + WEAPONS;
				int32_t j     = ITEM_VIOLENT_FORCE;
				curr_pref[ i ] = ROUND( boost_amp_pref( curr_pref[ i ], j, ai_level ) );
			} // end of amp check
		}         // dnd of being allowed to by boost items
	}                 // end of step 3


	// 5.: Freeing tools
	if ( RAND_AI_0P ) {
		if ( !nm[ HVY_RIOT_BOMB ] || !nm[ RIOT_BOMB ] || ( mood < 2. ) ) {

			// More offensive in this round, check for riot bombs
			if ( ( nm[ HVY_RIOT_BOMB ] < 2 ) && ( money >= weapon[ HVY_RIOT_BOMB ].cost ) ) {

				DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Heavy Riot Bomb", 0 )
				return HVY_RIOT_BOMB;
			}

			if ( ( nm[ RIOT_BOMB ] < 5 ) && ( money >= weapon[ RIOT_BOMB ].cost ) ) {

				DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Riot Bomb", 0 )
				return RIOT_BOMB;
			}
		} else {
			// In a defensive mood the charges are checked
			if ( ( nm[ RIOT_BLAST ] < 2 ) && ( money >= weapon[ RIOT_BLAST ].cost ) ) {

				DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Riot Blast", 0 )
				return RIOT_BLAST;
			}

			if ( ( nm[ RIOT_CHARGE ] < 5 ) && ( money >= weapon[ RIOT_CHARGE ].cost ) ) {

				DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Riot Charge", 0 )
				return RIOT_CHARGE;
			}
		}
	} // End of step 4


	// 6.: Shields
	if ( ( shield_bought < ai_level ) && RAND_AI_1P ) {
		if ( mood <= 1.5 ) {

			// offensive type, go through reflectors
			if ( ( ni[ ITEM_LGT_REPULSOR_SHIELD ] <= ( item[ ITEM_LGT_REPULSOR_SHIELD ].amt * ai_level ) )
			     && ( money >= ( item[ ITEM_LGT_REPULSOR_SHIELD ].cost * 2.0 ) ) ) {

				DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Light Repulsor Shield", 0 )
				return ( WEAPONS + ITEM_LGT_REPULSOR_SHIELD );
			}

			if ( ( ni[ ITEM_MED_REPULSOR_SHIELD ] <= ( item[ ITEM_MED_REPULSOR_SHIELD ].amt * ai_level ) )
			     && ( money >= ( item[ ITEM_MED_REPULSOR_SHIELD ].cost * 1.75 ) ) ) {

				DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Medium Repulsor Shield", 0 )
				return ( WEAPONS + ITEM_MED_REPULSOR_SHIELD );
			}

			if ( ( ni[ ITEM_HVY_REPULSOR_SHIELD ] <= ( item[ ITEM_HVY_REPULSOR_SHIELD ].amt * ai_level ) )
			     && ( money >= ( item[ ITEM_HVY_REPULSOR_SHIELD ].cost * 1.5 ) ) ) {

				DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Heavy Repulsor Shield", 0 )
				return ( WEAPONS + ITEM_HVY_REPULSOR_SHIELD );
			}
		} // End of offensive mood

		if ( mood >= 2.5 ) {

			// defensive type, go through hard shields
			if ( ( ni[ ITEM_LGT_SHIELD ] <= ( item[ ITEM_LGT_SHIELD ].amt * ai_level ) )
			     && ( money >= ( item[ ITEM_LGT_SHIELD ].cost * 2.0 ) ) ) {

				DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Light Shield", 0 )
				return ( WEAPONS + ITEM_LGT_SHIELD );
			}

			if ( ( ni[ ITEM_MED_SHIELD ] <= ( item[ ITEM_MED_SHIELD ].amt * ai_level ) )
			     && ( money >= ( item[ ITEM_MED_SHIELD ].cost * 1.75 ) ) ) {

				DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Medium Shield", 0 )
				return ( WEAPONS + ITEM_MED_SHIELD );
			}

			if ( ( ni[ ITEM_HVY_SHIELD ] <= ( item[ ITEM_HVY_SHIELD ].amt * ai_level ) )
			     && ( money >= ( item[ ITEM_HVY_SHIELD ].cost * 1.5 ) ) ) {

				DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Heavy Shield", 0 )
				return ( WEAPONS + ITEM_HVY_SHIELD );
			}
		} // End of defensive mood
	}         // End of step 5


	// Step 7: Fuel
	if ( ( ni[ ITEM_FUEL ] < 100 ) && ( money >= item[ ITEM_FUEL ].cost ) ) {
		DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Fuel", 0 )
		return ( WEAPONS + ITEM_FUEL );
	}


	// Step 8: Slick / Dimpled Projectiles
	if ( ( ni[ ITEM_SLICKP ] + ni[ ITEM_DIMPLEP ] ) < 100 ) {

		if ( ( ni[ ITEM_DIMPLEP ] < 50 ) && ( money >= item[ ITEM_DIMPLEP ].cost ) ) {

			DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Dimpled Projectiles", 0 )
			return ( WEAPONS + ITEM_DIMPLEP );
		}

		if ( ( ni[ ITEM_SLICKP ] < 50 ) && ( money >= item[ ITEM_SLICKP ].cost ) ) {

			DEBUG_LOG_FIN( name.c_str(), "Pre-selecting Slick Projectiles", 0 )
			return ( WEAPONS + ITEM_SLICKP );
		}
	}

	return -1;
}

EControl CPlayer::control_tank( CAICore* aicore, bool allow_fire ) {
	// Handle User input, this is read for providing the ingame menu
	// even when no human player is active. Otherwise, the player would
	// not be able to enter the ingame menu whenever an AI player is
	// active.
	k = 0;
	K = 0;
	if ( key_shifts & KB_CTRL_FLAG ) {
		has_ctrl_pressed = true;
	} else {
		has_ctrl_pressed = false;
	}
	if ( key_shifts & KB_SHIFT_FLAG ) {
		has_shift_pressed = true;
	} else {
		has_shift_pressed = false;
	}
	if ( keypressed() ) {
		k = readkey();
		K = k >> 8;

		// Enter ingame menu?
		if ( ( KEY_ESC == K ) || ( KEY_P == K ) ) {
			int32_t mm = env.in_game_menu();

			global.make_update( 0, 0, env.screen_width, env.screen_height );
			global.make_bgupdate( 0, 0, env.screen_width, env.screen_height );

			switch ( mm ) {
				case 1:
					// Main CMenu
					global.set_command( GLOBAL_COMMAND_MENU );
					return CONTROL_QUIT;
				case 2:
					// Quit the game
					global.set_command( GLOBAL_COMMAND_QUIT );
					return CONTROL_QUIT;
				case 3:
					// Skip AI
					if ( STAGE_SCOREBOARD > global.stage ) {
						return CONTROL_SKIP;
					}
				default:
					break;
			}
		}

		// check for number key being pressed
		if ( ( K >= KEY_0 ) && ( K <= KEY_9 ) ) {
			int32_t value = K - KEY_0;
			K             = 0;

			// make sure the value is within range
			if ( ( value < env.num_game_players ) && env.players[ value ] ) {

				CTank* my_tank = env.players[ value ]->tank;

				if ( my_tank ) {
					snprintf(
						global.tank_status,
						127,
						"%s: %d + %d -- Team: %s",
						env.players[ value ]->name.c_str(),
						my_tank->l,
						my_tank->sh,
						env.players[ value ]->get_team_name()
					);
					global.tank_status_colour = env.players[ value ]->color;
					global.update_menu         = true;
				} else {
					memset( global.tank_status, 0, sizeof( char ) * 128 );
				}
			}
		} // end of check status keys

		// Check for scorboard toggle key
		if ( ( K == KEY_TILDE ) || ( K == KEY_SLASH ) ) {
			K                     = 0;
			global.show_score_board = !global.show_score_board;
			if ( !global.show_score_board ) {
				// erase it:
				global.make_update( 0, MENUHEIGHT, 300, ( env.max_num_tanks + 1 ) * env.font_height );
				global.make_bgupdate( 0, MENUHEIGHT, 300, ( env.max_num_tanks + 1 ) * env.font_height );
			}
		}

		// Handle volume control
		if ( KEY_V == K ) {
			K = 0;
			if ( has_shift_pressed ) {
				env.increase_volume();
			} else {
				env.decrease_volume();
			}
		}
	} // End of handling all time possible key presses.


	if ( key[ KEY_F1 ] ) {
		static char shot_file[ 26 ] = { 0x0 };
		int32_t     nr              = 0;
		do {
			snprintf( shot_file, 26, "screenshot_%04d.bmp", ++nr );
		} while ( !access( shot_file, F_OK ) );

		if ( nr < 1000 ) {
			save_bmp( shot_file, global.canvas, nullptr );
		}
	}


	if ( has_ctrl_pressed && ctrlUsedUp ) {
		if ( !( key[ KEY_LEFT ] || key[ KEY_RIGHT ] || key[ KEY_UP ] || key[ KEY_DOWN ] || key[ KEY_PGUP ]
		        || key[ KEY_PGDN ] || key[ KEY_A ]
		        || key[ KEY_D ]
		        // additional control
		        || key[ KEY_W ] || key[ KEY_S ] || key[ KEY_R ] || key[ KEY_F ] ) ) {
			ctrlUsedUp = false;
		}
	} else {
		ctrlUsedUp = false;
	}


	// A) HUMAN
	if ( ( HUMAN_PLAYER == type ) || !tank ) {
		return human_controls( aicore );
	}

	// B) Network Client
#ifdef NETWORK
	else if ( type == NETWORK_CLIENT ) {
		return execute_net_cmd( true, aicore );
	}
#endif // NETWORK

	// C) AI Player
	else if ( global.stage == STAGE_AIM ) {
		return computer_controls( aicore, allow_fire );
	}

	return CONTROL_NONE;
}

void CPlayer::draw_indicator( int32_t x, int32_t y, int32_t h ) const {
	if ( HUMAN_PLAYER == type ) {
		int32_t radius = ROUND( static_cast< double >( h ) / 2. ) - 2;
		circlefill( global.canvas, x + radius + 2, y + radius + 2, radius, makecol( 200, 100, 255 ) );
		circle( global.canvas, x + radius + 2, y + radius + 2, radius, BLACK );
	} else {
		rectfill( global.canvas, x, y + 2, x + 15, y + h - 1, BLACK );
		for ( int32_t i = 0; i < type; ++i ) {
			rectfill( global.canvas, x + ( 3 * i ) + 1, y + 3, x + ( 3 * i ) + 2, y + h - 2, makecol( 100, 255, 100 ) );
		}
	}
}


#ifdef NETWORK
// This function gets called during a round when a networked
// player gets to act. The function checks to see if anything
// is in the net_command variable. If there is, it handles
// the request. If not, the function returns.
//
// We should have some time keeping in here before this goes live
// to avoid hanging the game.
EControl CPlayer::execute_net_cmd( bool my_turn, CAICore* aicore ) {
	static int playerindex = -1;
	static int fire_delay = 0, net_delay = 0;

	if ( my_turn ) {
		fire_delay++;
		// if enough time has passed, we give up and turn control over to the AI
		if ( fire_delay >= NET_DELAY ) {
			type       = VERY_PART_TIME_BOT;
			fire_delay = 0;
			return computer_controls( aicore, true );
		}
	}

	if ( !net_command[ 0 ] ) {
		net_delay++;
		/*
		if (my_delay >= NET_DELAY)
		{
		   my_delay = 0;
		   setComputerValues();
		   type = VERY_PART_TIME_BOT;
		   strcpy(buffer, "PING");
		   write(server_socket, buffer, strlen(buffer));
		   return computer_controls();
		}
		else*/
		if ( net_delay >= NET_DELAY_SHORT ) {
			// prompt the client to respond
			SAFE_WRITE( server_socket, "%s", "PING" );
		}
		return CONTROL_NONE;
	} // we did not get a command to process

	else {
		net_delay = 0; // we got something, so reset timer
	}

	if ( !strncmp( net_command, "VERSION", 7 ) ) {
		SAFE_WRITE( server_socket, "SERVERVERSION %s", VERSION );
	} else if ( !strncmp( net_command, "CLOSE", 5 ) ) {
		close( server_socket );
		type = DEADLY_PLAYER;
	} else if ( !strncmp( net_command, "BOXED", 5 ) ) {
		SAFE_WRITE( server_socket, "BOXED %d", env.is_boxed ? 1 : 0 );
	} else if ( !strncmp( net_command, "GOSSIP", 6 ) ) {
		snprintf( global.tank_status, 127, "%s", &( net_command[ 7 ] ) );
		global.update_menu = true;
	} else if ( !strncmp( net_command, "HEALTH", 6 ) ) {
		int tank_index = 0;

		SAFE_STOI( tank_index, &( net_command[ 7 ] ) );
		if ( ( tank_index >= 0 ) && ( tank_index < env.num_game_players ) ) {
			if ( env.players[ tank_index ]->tank ) {
				SAFE_WRITE(
					server_socket,
					"HEALTH %d %d %d %d",
					tank_index,
					env.players[ tank_index ]->tank->l,
					env.players[ tank_index ]->tank->sh,
					env.players[ tank_index ]->tank->sht
				);
			}
		}

	} else if ( !strncmp( net_command, "CItem", 4 ) ) {
		int item_index = 0;
		SAFE_STOI( item_index, &( net_command[ 5 ] ) );
		if ( ( item_index >= 0 ) && ( item_index < ITEMS ) ) {
			SAFE_WRITE( server_socket, "CItem %d %d", item_index, ni[ item_index ] );
		}
	} else if ( !strncmp( net_command, "MOVE", 4 ) ) {
		if ( !my_turn ) {
			return CONTROL_NONE;
		}
		if ( tank ) {
			if ( strstr( net_command, "LEFT" ) ) {
				tank->move_tank( DIR_LEFT );
			} else {
				tank->move_tank( DIR_RIGHT );
			}
			global.update_menu = true;
		}
	} else if ( !strncmp( net_command, "FIRE", 4 ) ) {
		int angle = 180, power = 1000, item_to_use = 0;
		if ( !my_turn ) {
			return CONTROL_NONE;
		}

		std::istringstream iss( &( net_command[ 5 ] ) );
		iss >> item_to_use >> angle >> power;
		fire_delay = 0;
		if ( tank ) {
			if ( item_to_use >= THINGS ) {
				item_to_use = 0;
			}
			tank->cw = item_to_use;
			if ( item_to_use < WEAPONS ) {
				if ( nm[ tank->cw ] < 1 ) {
					tank->cw = 0;
				}
			} else // item_to_use
			{
				if ( ni[ tank->cw - WEAPONS ] < 1 ) {
					tank->cw = 0;
				}
			}
			tank->a = angle;
			if ( tank->a > 270 ) {
				tank->a = 270;
			} else if ( tank->a < 90 ) {
				tank->a = 90;
			}
			tank->p = power;
			if ( tank->p > 2000 ) {
				tank->p = 2000;
			} else if ( tank->p < 0 ) {
				tank->p = 0;
			}
			gloating         = false;
			net_command[ 0 ] = '\0';
			return CONTROL_FIRE;
		}
	}

	// find out which player this is
	else if ( !strncmp( net_command, "WHOAMI", 6 ) ) {
		bool found = false;

		while ( ( playerindex < env.num_game_players ) && ( !found ) ) {
			if ( env.players[ playerindex ] == this ) {
				found = true;
				SAFE_WRITE( server_socket, "YOUARE %d", playerindex );
			} else {
				playerindex++;
			}
		}
		// check to see if something went very wrong
		if ( !found ) {
			SAFE_WRITE( server_socket, "YOUARE %d", -1 );
		}
	}
	// return wind speed
	else if ( !strncmp( net_command, "WIND", 4 ) ) {
		SAFE_WRITE( server_socket, "WIND %f", global.wind );
	}

	// find out how many players we have
	else if ( !strncmp( net_command, "NUMPLAYERS", 10 ) ) {
		SAFE_WRITE( server_socket, "NUMPLAYERS %d", env.num_game_players );
	} else if ( !strncmp( net_command, "PLAYERNAME", 10 ) ) {
		int my_number = 0;
		SAFE_STOI( my_number, &( net_command[ 11 ] ) );
		if ( ( my_number >= 0 ) && ( my_number < env.num_game_players ) ) {
			SAFE_WRITE( server_socket, "PLAYERNAME %d %s", my_number, env.players[ my_number ]->get_name() );
		}
	}

	// how many rounds are we playing
	else if ( !strncmp( net_command, "ROUNDS", 6 ) ) {
		SAFE_WRITE( server_socket, "ROUNDS %d %d", env.rounds, global.current_round );
	}
	// send back the position of each tank
	else if ( !strncmp( net_command, "TANKPOSITION", 12 ) ) {
		int count = 0;

		SAFE_STOI( count, &( net_command[ 13 ] ) );
		if ( ( count >= 0 ) && ( count < env.num_game_players ) && ( env.players[ count ]->tank ) ) {
			SAFE_WRITE(
				server_socket,
				"TANKPOSITION %d %d %d",
				count,
				(int)env.players[ count ]->tank->x,
				(int)env.players[ count ]->tank->y
			);
		}
	}

	// send back the surface height of the dirt
	else if ( !strncmp( net_command, "SURFACE", 7 ) ) {
		int x = 0;

		SAFE_STOI( x, &( net_command[ 8 ] ) );
		if ( ( x >= 0 ) && ( x < env.screen_width ) ) {
			SAFE_WRITE( server_socket, "SURFACE %d %d", x, global.surface[ x ].load() );
		}
	} else if ( !strncmp( net_command, "SCREEN", 6 ) ) {
		SAFE_WRITE( server_socket, "SCREEN %d %d", env.screen_width, env.screen_height );
	} else if ( !strncmp( net_command, "TEAMS", 5 ) ) {
		int count = 0;

		SAFE_STOI( count, &( net_command[ 6 ] ) );
		if ( ( count < env.num_game_players ) && ( count >= 0 ) ) {
			SAFE_WRITE( server_socket, "TEAM %d %d", count, (int)env.players[ count ]->team );
		}
	} else if ( !strncmp( net_command, "WALLTYPE", 8 ) ) {
		SAFE_WRITE( server_socket, "WALLTYPE %d", env.current_wall_type );
	} else if ( !strncmp( net_command, "CWeapon", 6 ) ) {
		int weapon_number = 0;
		SAFE_STOI( weapon_number, &( net_command[ 7 ] ) );
		if ( ( weapon_number >= 0 ) && ( weapon_number < WEAPONS ) ) {
			SAFE_WRITE( server_socket, "CWeapon %d %d", weapon_number, nm[ weapon_number ] );
		}
	}

	net_command[ 0 ] = '\0';

	return CONTROL_NONE;
}
#endif // NETWORK


void CPlayer::exit_shop() {
	double tmpDM = ( ni[ ITEM_INTENSITY_AMP ] * item[ ITEM_INTENSITY_AMP ].vals[ 0 ] )
	             + ( ni[ ITEM_VIOLENT_FORCE ] * item[ ITEM_VIOLENT_FORCE ].vals[ 0 ] );

	damage_multiplier = 1.0;

	if ( tmpDM > 0 ) {
		damage_multiplier += std::pow( tmpDM, 0.6 );
	}

	// All players need small missiles:
	if ( nm[ SML_MIS ] < 100 ) {
		nm[ SML_MIS ] += 100 + ( get_rand() % 100 ); // + [100;199]
	}
	if ( nm[ SML_MIS ] < 250 ) {
		nm[ SML_MIS ] += 50 + ( get_rand() % 50 ); // + [ 50; 99]
	}
}

/// @brief fill the list of desired items and return the number of damaging weapons
int32_t CPlayer::generate_desired_list() {
	int32_t result = 0;

	memset( desired, 0, sizeof( int32_t ) * THINGS );

	for ( int32_t i = 1; i < THINGS; ++i ) {
		if ( env.is_item_available( i ) ) {
			desired[ i ]   = i;
			boost_pref[ i ] = std::max( boost_pref[ i ], curr_pref[ i ] - weap_pref[ i ] );
			curr_pref[ i ]  = weap_pref[ i ] + boost_pref[ i ];

			/* Notes on boost_pref:
			 * To be able to both reset to the static preferences and boost items over several rounds,
			 * the boostPrefs array is used as follows:
			 * It is reset to the difference between the current preferences (modified last round) and the
			 * static preference, if it is higher than the current value.
			 * So if it has a value of 0, and curr_pref is 550 because it was boosted 10% over a static 500
			 * preference, it would be set to 50.
			 * Whenever a weapon or an item having a boosPref value over zero is bought by the bot, the boost_pref
			 * entry is reset to 0 and the weapon/item has to be "re-boosted" to get the value back up again.
			 */

			// No negative prefs:
			if ( curr_pref[ i ] < 0 ) {
				curr_pref[ i ] = 0;
			}

			// Count weapons:
			if ( ( i < WEAPONS ) && nm[ i ] ) {
				++result;
			}
		} else {
			desired[ i ] = 0;
		}
		// Unavailable items will not be inserted, and
		// the slot is filled with a 0 (small missile)
		// that will be sorted away.
	}

	return result;
}

void CPlayer::generate_preferences() {
	double  baseProb    = static_cast< double >( MAX_WEAP_PROBABILITY ) / 2.;
	int32_t currItem    = 0;
	double  worth       = 0.;
	bool    isWarhead   = false;
	int32_t maxWeapPref = 0;
	int32_t maxItemPref = 0;
	double  ai_rate     = static_cast< double >( type ) / 2. + .5;

	/* --------------------------------------
	 * --- Generate basic characteristics ---
	 * --------------------------------------
	 */
	defensive          = ( static_cast< double >( get_rand() % 10001 ) / 5000. ) - 1.; // [-1;+1]
	vengeful           = 1 + ( get_rand() % 100 );                                     // [1;100]
	vengeance_threshold = 0.05 + ( static_cast< double >( get_rand() % 901 ) / 1000. ); // [0.05;0.95]
	self_preservation   = static_cast< double >( get_rand() % 3001 ) / 1000;            // [0;3]
	pain_sensitivity    = static_cast< double >( get_rand() % 3001 ) / 1000;            // [0;3]

	// Now 'defensive' can be modified by team:
	if ( team == TEAM_JEDI ) {
		defensive += static_cast< double >( get_rand() % 501 ) / 1000.;
		if ( defensive > 1.25 ) {
			defensive = 1.25; // + 1.25 is Super Defensive
		}
	} else if ( team == TEAM_SITH ) {
		defensive -= static_cast< double >( get_rand() % 501 ) / 1000.;
		if ( defensive < -1.25 ) {
			defensive = -1.25; // - 1.25 is Super Aggressive
		}
	}

	/* --------------------------------------------
	 * --- Generate weapon and item preferences ---
	 * --------------------------------------------
	 */
	if ( name != "New Player" ) {
		DEBUG_LOG_EMO( name.c_str(), "Generating preferences (defensive %lf)", defensive )
		DEBUG_LOG_EMO( name.c_str(), "---------------------------------------", 0 )
	}

	weap_pref[ 0 ] = 0; // small missiles are always zero!

	for ( int32_t i = 1; i < THINGS; ++i ) {
		worth     = baseProb / -2.;
		isWarhead = false;

		if ( i < WEAPONS ) {
			// Talking about weapons
			currItem = i;
			if ( weapon[ i ].warhead || ( ( currItem >= SML_METEOR ) && ( currItem <= LRG_LIGHTNING ) ) ) {
				isWarhead = true;
			}
			// Warheads are ignored, this way naturals
			// are taken out automatically.
			else {

				int32_t warheads = weapon[ currItem ].spread;

				// === 1. Damage: ===
				//--------------------
				if ( weapon[ currItem ].numSubmunitions > 0 ) {

					warheads = weapon[ currItem ].numSubmunitions;

					// Use the total damage for clusters
					worth = weapon[ weapon[ currItem ].submunition ].damage * warheads;

					if ( ( ( currItem >= SML_NAPALM ) && ( currItem <= LRG_NAPALM ) )
					     || ( ( currItem >= FUNKY_BOMB ) && ( currItem <= FUNKY_DEATH ) ) ) {
						worth /= ( defensive + 2. + ai_rate ) / 2.;
					}
					// These weapons are too unpredictable to be counted full.
					// But a true offensive useless bot divides only by 1.0
					// (so not all all, they do not mind) and a true
					// defensive deadly bot divides by 2.5

					// Napalm Jellies doe damage over time. So their worth
					// has to reflect that.
					if ( ( currItem >= SML_NAPALM ) && ( currItem <= LRG_NAPALM ) ) {
						worth *= static_cast< double >( EXPLOSIONFRAMES ) / 2.
						       / static_cast< double >( type );
					}

					if ( worth > baseProb ) {
						// Or Large Napalm will always be everybody favourite
						worth = baseProb;
					}
				} else {
					// Otherwise use spread value with damage. For non-spread
					// weapons this value is always 1.
					worth = weapon[ currItem ].damage * ( warheads * 2 ) * weapon[ currItem ].getDelayDiv();
				}
				// Note: warheads are counted twice, because otherwise spread
				//       weapons get a by far too low score!

				// 1 Damage is worth 0.5%o of the base probability.
				worth *= baseProb * 0.0005;

				// === 2. Defensiveness multiplier ===
				//-------------------------------------
				// As said above, defensive players avoid spread/cluster
				// weapons that are too unpredictable. Thus they rate
				// non-spreads higher:
				if ( warheads == 1 ) {
					worth *= ( defensive + 1.5 ) * ai_rate;
				}

				// === 3. Dirt weapons ===
				//-------------------------
				// Dirt balls and such weapons do no damage and have to be rated
				// by defensiveness value. Further more the higher the self
				// preservation value of a bot, the more likely they will try
				// to bury main damage dealers for one or two rounds of bought
				// silence.
				if ( ( currItem >= DIRT_BALL ) && ( currItem <= SMALL_DIRT_SPREAD ) ) {
					worth = warheads * weapon[ currItem ].radius * ai_rate * ( defensive + 2. )
					      * self_preservation;
				}

				// === 4. Debuff weapons ===
				//---------------------------
				// These are the opposite of dirt weapons, they are for the
				// offensive type with high self preservation.
				// Note: Although the percent bomb is not a de-buff weapon,
				// it can hardly be rated any other way, as it has no set yield.
				if ( ( currItem >= PERCENT_BOMB ) && ( currItem <= REDUCER ) ) {
					worth = 300. * ai_rate * -( defensive - 2. ) * self_preservation;
				}
				// Note: The theft bomb is a debuff weapon with extra benefits. ;-)
				if ( THEFT_BOMB == currItem ) {
					worth = ( 150. + vengeful ) * ai_rate * ( ( self_preservation + 2. ) / 2. )
					      * ( std::abs( defensive ) + 1.0 );
				}

				// === 5. Shaped weapons are deadly but limited ===
				//--------------------------------------------------
				if ( ( ( currItem >= SHAPED_CHARGE ) && ( currItem <= CUTTER ) ) || ( DRILLER == currItem ) ) {
					worth *= 1.0 - ( ( ( 2. * ai_rate ) + ( defensive * 5.0 ) ) / 20.0 );
				}
				// useless, full offensive: * 1.15
				// deadly, full defensive : * 0.45

				// === 6. Rollers and penetrators ===
				//------------------------------------
				// These are modified by type, as they *are* useful
				if ( ( ( currItem >= SML_ROLLER ) && ( currItem <= DTH_ROLLER ) )
				     || ( ( currItem >= BURROWER ) && ( currItem <= PENETRATOR ) ) ) {
					worth *= 1.0 + ( ai_rate / 5. ) + ( defensive / 2. );
				}

				// === 7. Tectonics need to be raised! ===
				//-----------------------------------------
				// These are nice to damage multiple buried enemies where
				// penetrators can only reach one.
				if ( ( currItem >= TREMOR ) && ( currItem <= TECTONIC ) ) {
					worth *= 2.0 + ( ai_rate / 5. ) + ( defensive / 3. );
				}

				// finally dWorth must not be greater than the 3/4 of MAX_WEAPON_PROBABILITY
				if ( worth > ( MAX_WEAP_PROBABILITY * 0.75 ) ) {
					worth = MAX_WEAP_PROBABILITY * 0.75;
				}
			} // End of "not a warhead"
		} else {
			// Talking about items
			currItem = i - WEAPONS;

			/* Theory:
			 * The more offensive a bot is, the more likely they go for
			 * damage amps and repulsor shields.
			 * The more defensive they are, the more likely they go for
			 * armour and hard shields.
			 */

			switch ( currItem ) {
				case ITEM_TELEPORT:
					worth = ( defensive - 1.5 ) * ( baseProb / -5.00 ) * self_preservation;
					break;
				case ITEM_SWAPPER:
					worth = ( defensive - 1.5 ) * ( baseProb / -3.75 ) * self_preservation;
					break;
				case ITEM_MASS_TELEPORT:
					worth = ( defensive - 1.5 ) * ( baseProb / -1.50 ) * self_preservation;
					break;
				case ITEM_FAN:
					worth = 0.0; // useless things!
					break;
				case ITEM_VENGEANCE:
				case ITEM_DYING_WRATH:
				case ITEM_FATAL_FURY:
					worth = ( defensive + 1.5 )
					      * static_cast< double >( weapon[ (int)item[ currItem ].vals[ 0 ] ].damage )
					      * item[ currItem ].vals[ 1 ];
					break;
				case ITEM_ARMOUR:
				case ITEM_PLASTEEL:
					worth = baseProb * ( item[ currItem ].vals[ 0 ] / item[ ITEM_PLASTEEL ].vals[ 0 ] )
					      * ( defensive + 1.25 );
					break;
				case ITEM_LGT_SHIELD:
				case ITEM_MED_SHIELD:
				case ITEM_HVY_SHIELD:
					worth = baseProb * ( item[ currItem ].vals[ 0 ] / item[ ITEM_HVY_SHIELD ].vals[ 0 ] )
					      * ( defensive + 1.25 );
					break;
				case ITEM_INTENSITY_AMP:
				case ITEM_VIOLENT_FORCE:
					worth = baseProb * ( item[ currItem ].vals[ 0 ] / item[ ITEM_VIOLENT_FORCE ].vals[ 0 ] )
					      * ( ( -1. * defensive ) + 1.25 );
					break;
				case ITEM_LGT_REPULSOR_SHIELD:
				case ITEM_MED_REPULSOR_SHIELD:
				case ITEM_HVY_REPULSOR_SHIELD:
					worth = baseProb
					      * ( item[ currItem ].vals[ 0 ] / item[ ITEM_HVY_REPULSOR_SHIELD ].vals[ 0 ] )
					      * ( ( -1. * defensive ) + 1.25 );
					break;
				case ITEM_REPAIRKIT:
					worth = ( baseProb / 12. * ai_rate ) * ( defensive + 2.25 + ( self_preservation / 2. ) );
					break;
				case ITEM_PARACHUTE:
					worth = ( baseProb / 10. * ai_rate ) * ( ( defensive + 1.5 ) / 1.5 );
					// Parachutes *are* popular! :)
					break;
				case ITEM_SLICKP:
					worth = baseProb / 25. * ai_rate;
					break;
				case ITEM_DIMPLEP:
					worth = baseProb / 15. * ai_rate;
					break;
				case ITEM_FUEL:
					worth     = baseProb / 30. * ai_rate;
					isWarhead = true; // Yes, it's a lie. ;-)
					break;
				case ITEM_ROCKET:
					worth     = -5000; // Bots don't use rockets
					isWarhead = true;  // The cake is a lie!
					break;
				case ITEM_SDI:
					worth = ( baseProb / 13. * ai_rate )
					      * ( ( defensive + 2.25 + self_preservation ) / 1.25 );
					break;
				default:
					cerr << "Error: Unhandled item " << currItem;
					cerr << "       in generate_preferences()!" << endl;
					worth = baseProb / ai_rate;
			}


			// worth must not be greater than the half of MAX_WEAPON_PROBABILITY
			if ( worth > ( MAX_WEAP_PROBABILITY / 2. ) ) {
				worth = MAX_WEAP_PROBABILITY / 2.;
			}
		}

		// Boost the tiny ones:
		if ( worth < ( MAX_WEAP_PROBABILITY / 25. ) ) {
			worth = MAX_WEAP_PROBABILITY / 25.; // Which is very, very little...
		}
		if ( worth < ( MAX_WEAP_PROBABILITY / 8. ) ) {
			// allow to double (more or less)
			worth += static_cast< double >( get_rand() % static_cast< int32_t >( std::abs( worth ) ) );
		}

		// But don't overdo either:
		if ( worth > MAX_WEAP_PROBABILITY ) {
			worth = MAX_WEAP_PROBABILITY;
		}

		if ( isWarhead ) {
			weap_pref[ i ] = 0; // It will not get any slot!
		} else {
			weap_pref[ i ] = ROUND( worth );
		}

		// Count statistical values
		if ( ( i < WEAPONS ) && ( weap_pref[ i ] > maxWeapPref ) ) {
			maxWeapPref = weap_pref[ i ];
		}
		if ( ( i >= WEAPONS ) && ( weap_pref[ i ] > maxItemPref ) ) {
			maxItemPref = weap_pref[ i ];
		}

		if ( name != "New Player" ) {
			DEBUG_LOG_EMO(
				name.c_str(),
				"%23s (%6s): %5d",
				i < WEAPONS ? weapon[ i ].getName() : item[ i - WEAPONS ].getName(),
				i < WEAPONS ? "weapon" : "item",
				weap_pref[ i ]
			)
		}
	} // end of looping THINGS

	// If the maximum preferences are too low, they have to be augmented
	if ( maxWeapPref < MAX_WEAP_PROBABILITY ) {
		worth = static_cast< double >( MAX_WEAP_PROBABILITY ) / static_cast< double >( maxWeapPref );

		for ( int32_t i = 1; i < WEAPONS; ++i ) {
			if ( weap_pref[ i ] > ( MAX_WEAP_PROBABILITY / 100.0 ) ) {
				weap_pref[ i ] = ROUND( worth * weap_pref[ i ] );
				if ( name != "New Player" ) {
					DEBUG_LOG_EMO(
						name.c_str(),
						"%23s (%6s) amplified to: %5d",
						weapon[ i ].getName(),
						"weapon",
						weap_pref[ i ]
					)
				}
			}
		}
	}

	if ( maxItemPref < ( MAX_WEAP_PROBABILITY * 0.75 ) ) {
		worth = static_cast< double >( MAX_WEAP_PROBABILITY ) * 0.75 / static_cast< double >( maxItemPref );

		for ( int32_t i = WEAPONS; i < THINGS; ++i ) {
			if ( weap_pref[ i ] > ( MAX_WEAP_PROBABILITY / 100.0 ) ) {
				weap_pref[ i ] = ROUND( worth * weap_pref[ i ] );
				if ( name != "New Player" ) {
					DEBUG_LOG_EMO(
						name.c_str(),
						"%23s (%6s) amplified to: %5d",
						item[ i - WEAPONS ].getName(),
						"item",
						weap_pref[ i ]
					)
				}
			}
		}
	}

	if ( name != "New Player" ) {
		DEBUG_LOG_EMO( name.c_str(), "=======================================", 0 )
	}
}

int CPlayer::get_amp_value() {
	double amp_val = ni[ ITEM_INTENSITY_AMP ] * item[ ITEM_INTENSITY_AMP ].vals[ 0 ];
	double vio_val = ni[ ITEM_VIOLENT_FORCE ] * item[ ITEM_VIOLENT_FORCE ].vals[ 0 ];
	return ROUNDu( ( amp_val + vio_val ) / static_cast< double >( item[ ITEM_VIOLENT_FORCE ].vals[ 0 ] ) );
}

int CPlayer::get_armour_value() {
	double arm_val = ni[ ITEM_ARMOUR ] * item[ ITEM_ARMOUR ].vals[ 0 ];
	double pla_val = ni[ ITEM_PLASTEEL ] * item[ ITEM_PLASTEEL ].vals[ 0 ];
	return ROUNDu( ( arm_val + pla_val ) / static_cast< double >( item[ ITEM_PLASTEEL ].vals[ 0 ] ) );
}

int CPlayer::get_boost_value() {
	return ( get_amp_value() + get_armour_value() );
}

/// @brief return the item preference of item @a idx or -1 if @a idx is out of
/// range
/// Note: This uses the static weap_pref instead of the adapted curr_pref,
///       because it is used by CAICore for point calculation.
int32_t CPlayer::get_item_pref( int32_t idx ) {
	if ( ( idx > -1 ) && ( idx < ITEMS ) ) {
		return weap_pref[ WEAPONS + idx ];
	}
	return -1;
}

int32_t CPlayer::get_money_to_save( bool first_look ) {
	// If this is the first look in a shopping round,
	// the list of items to save money for must be built:

	if ( first_look ) {
		int32_t avgPref   = 0;
		int32_t prefCount = 0;
		int32_t prefLimit = 0;
		memset( save_money_for, 0, sizeof( int32_t ) * THINGS );

		// if the preferences are exceptionally low, a div by 0
		// might occur, so it has to be made dynamic:
		for ( int i : curr_pref ) {
			if ( i > 0 ) {
				prefLimit += i;
				prefCount++;
			}
		}

		prefLimit /= prefCount ? prefCount : 1; // Rough average of all preferences we have
		DEBUG_LOG_FIN( name.c_str(), "Middle preference value is %d with %d counted", prefLimit, prefCount )
		prefCount = 0;

		// Now that the prefLimit is roughly the middle, let's get the average of everything above
		for ( int i : curr_pref ) {
			if ( i > prefLimit ) {
				prefCount++;
				avgPref += i;
			}
		}

		// Complete the average preference of the most valuable weapons:
		avgPref /= prefCount ? prefCount : 1;
		DEBUG_LOG_FIN( name.c_str(), "Average preference value above %d is %d with %d counted", avgPref, prefLimit, prefCount )


		// Now go through the list and add everything above the
		// average into the save money list if the amount in stock
		// is too low:
		for ( int32_t i = 0; i < THINGS; ++i ) {
			int32_t j = i - WEAPONS; // short cut
			if ( ( curr_pref[ i ] > avgPref )
			     && ( ( ( i < WEAPONS ) && ( nm[ i ] < weapon[ i ].amt ) ) // Stock up weapons
			          || ( ( j == ITEM_VIOLENT_FORCE ) && need_amp )        // Save up for amp if we need it
			          || ( ( j == ITEM_PLASTEEL ) && need_armour )          // Save up for armor if we need it
			     ) ) {
				save_money_for[ i ] = i < WEAPONS ? weapon[ i ].cost : item[ j ].cost;
				DEBUG_LOG_FIN(
					name.c_str(),
					" => Save money for %s!",
					i < WEAPONS ? weapon[ i ].getName() : item[ j ].getName()
				)
			} // end of having a big enough preference
		}         // end of looping THINGS
	}                 // end of building safe-for-list


	// moneyToSafe can be easily generated (and regenerated)
	// by walking through the list of things currently saved
	// money for:
	double moneyToSave = 0.;
	double wanted      = 0.;
	double max_cost    = 0.; // The most expensive item is counted twice
	for ( int32_t i = 0; i < THINGS; ++i ) {
		int32_t j = i - WEAPONS; // short cut

		if ( save_money_for[ i ] > 0 ) {
			// Still needed?
			if ( ( ( i < WEAPONS ) && ( nm[ i ] < weapon[ i ].amt ) ) // Stock up weapons
			     || ( ( j == ITEM_VIOLENT_FORCE ) && need_amp )        // Save up for amp if we need it
			     || ( ( j == ITEM_PLASTEEL ) && need_armour )          // Save up for armor if we need it
			) {
				moneyToSave += save_money_for[ i ];
				wanted      += 1.;
				if ( save_money_for[ i ] > max_cost ) {
					max_cost = save_money_for[ i ];
				}
				DEBUG_LOG_FIN(
					name.c_str(),
					" ==> I%s need %d.: %s! (+ %d => %d)",
					first_look ? "" : " still",
					ROUND( wanted ),
					i < WEAPONS ? weapon[ i ].getName() : item[ j ].getName(),
					save_money_for[ i ],
					ROUND( moneyToSave )
				)
			} else {
				// nope...
				save_money_for[ i ] = 0;
				DEBUG_LOG_FIN(
					name.c_str(),
					" <== I no longer need %s ...",
					i < WEAPONS ? weapon[ i ].getName() : item[ j ].getName()
				)
			}
		}
	}

	// If anything is wanted, the most expensive item is counted twice.
	// This is done so the bots do not consider having enough money
	// too early, just like humans would.
	if ( max_cost > 1. ) {
		moneyToSave += max_cost;
		wanted      += 1.;

		// The average money to save modified by the player type is the base:
		moneyToSave = ( moneyToSave / wanted ) * ( 1. + ( static_cast< double >( LAST_PLAYER_TYPE - type ) / 10. ) );

		// If the base is lower than the most expensive item costs, raise the amount to save to the
		// average of the base and twice the items cost.
		if ( moneyToSave < max_cost ) {
			moneyToSave = ( ( 2. * max_cost ) + moneyToSave ) / 2.;
		}
	}

	/* Results for Armageddon only @ 100k credits:
	 * (wanted is 1 in this test case)
	 * Useless: 100,000 * (1 + ( (6 - 1) / 10)) = 100,000 * 1.5 = 150,000
	 * Deadly : 100,000 * (1 + ( (6 - 5) / 10)) = 100,000 * 1.1 = 110,000
	 */

	// Whenever moneyToSave is less than the money owned, boost_bought is reset
	if ( first_look && ( money > ROUND( moneyToSave ) ) ) {
		boost_bought  = 0; // Let's go!
		shield_bought = 0;
	}

	return ROUND( moneyToSave );
}

// return the player name
char const* CPlayer::get_name() const {
	return name.c_str();
}

// This function checks for incoming data from a client.
// If data is coming in, we put the incoming data in the net_command
// variable. If the socket connection is broken, then we will
// close the socket and hand control over to the AI.
bool CPlayer::get_net_cmd() {
#ifdef NETWORK
	if ( Check_For_Incoming_Data( server_socket ) ) {
		// we have something coming down the pipe
		memset( net_command, '\0', NET_COMMAND_SIZE ); // clear buffer
		size_t status = read( server_socket, net_command, NET_COMMAND_SIZE );
		if ( !status ) {
			// connection is broken
			close( server_socket );
			type = DEADLY_PLAYER;
			printf( "%s lost network connection. Returning control to AI.\n", name.c_str() );
			return false;
		} else {
			// we got data
			net_command[ NET_COMMAND_SIZE - 1 ] = '\0';
			Trim_Newline( net_command );
		}
	}
#endif // NETWORK
	return true;
}

/** @brief Get one entry of the opponent memory or the last one attacked
 * @param[in] idx Index of the opponent memory to get, or -1 to get the last attacked.
 **/
sOpponent* CPlayer::get_opp_mem( int32_t idx ) {
	// regular memory
	if ( ( idx > -1 ) && ( idx < opp_count ) ) {
		return &opponents[ idx ];
	}

	// or the last attacked
	else if ( -1 == idx ) {
		return last_opponent;
	}

	// or invalid.
	return nullptr;
}

// returns a static string to the player's team name
char const* CPlayer::get_team_name() const {
	static char team_name[ 9 ] = { 0 };

	switch ( team ) {
		case TEAM_JEDI:
			snprintf( team_name, 8, "%s", "Jedi" );
			break;
		case TEAM_NEUTRAL:
			snprintf( team_name, 8, "%s", "Neutral" );
			break;
		case TEAM_SITH:
			snprintf( team_name, 8, "%s", "Sith" );
			break;
		case TEAM_COUNT:
		default:
			snprintf( team_name, 8, "%s", "* N/A *" );
			break;
	}

	return team_name;
}

/// @brief return the weapon preference of weapon @a idx or -1 if @a idx is out
/// of range.
/// Note: This uses the static weap_pref instead of the adapted curr_pref,
///       because it is used by CAICore for point calculation.
int32_t CPlayer::get_weap_pref( int32_t idx ) {
	if ( ( idx > -1 ) && ( idx < WEAPONS ) ) {
		return weap_pref[ idx ];
	}
	return -1;
}

EControl CPlayer::human_controls( CAICore* aicore ) {
	bool     moved  = false;
	EControl status = CONTROL_NONE;

	// Keyboard control in aim stage
	if ( ( global.stage == STAGE_AIM ) && tank ) {
		if ( ( key[ KEY_LEFT ] || key[ KEY_A ] ) && !ctrlUsedUp && ( tank->a < 270 ) ) {
			if ( has_shift_pressed ) {
				tank->a = std::min( tank->a + 5, 270 );
			} else {
				tank->a++;
			}
			global.update_menu = true;
			if ( has_ctrl_pressed ) {
				ctrlUsedUp = true;
			}
		}

		if ( ( key[ KEY_RIGHT ] || key[ KEY_D ] ) && !ctrlUsedUp && ( tank->a > 90 ) ) {
			if ( has_shift_pressed ) {
				tank->a = std::max( tank->a - 5, 90 );
			} else {
				tank->a--;
			}
			global.update_menu = true;
			if ( has_ctrl_pressed ) {
				ctrlUsedUp = true;
			}
		}

		if ( ( key[ KEY_DOWN ] || key[ KEY_S ] ) && !ctrlUsedUp && ( tank->p > 0 ) ) {
			if ( has_shift_pressed ) {
				tank->p = std::max( tank->p - 25, 0 );
			} else {
				tank->p -= 5;
			}
			global.update_menu = true;
			if ( has_ctrl_pressed ) {
				ctrlUsedUp = true;
			}
		}

		if ( ( key[ KEY_UP ] || key[ KEY_W ] ) && !ctrlUsedUp && ( tank->p < MAX_POWER ) ) {
			if ( has_shift_pressed ) {
				tank->p = std::min( tank->p + 25, MAX_POWER );
			} else {
				tank->p += 5;
			}
			global.update_menu = true;
			if ( has_ctrl_pressed ) {
				ctrlUsedUp = true;
			}
		}

		if ( ( key[ KEY_PGUP ] || key[ KEY_R ] ) && !ctrlUsedUp && ( tank->p < MAX_POWER ) ) {
			tank->p += 100;
			if ( tank->p > MAX_POWER ) {
				tank->p = MAX_POWER;
			}
			global.update_menu = true;
			if ( has_ctrl_pressed ) {
				ctrlUsedUp = true;
			}
		}

		if ( ( key[ KEY_PGDN ] || key[ KEY_F ] ) && !ctrlUsedUp && ( tank->p > 0 ) ) {
			tank->p -= 100;
			if ( tank->p < 0 ) {
				tank->p = 0;
			}
			global.update_menu = true;
			if ( has_ctrl_pressed ) {
				ctrlUsedUp = true;
			}
		}
	}

	// See whether there is a new key press
	if ( !k ) {
		if ( keypressed() ) {
			k = readkey();
			K = k >> 8;
		}
	}

	// If anything is newly there, make it happen
	if ( K ) {
		status = CONTROL_OTHER;

		if ( ( global.stage == STAGE_AIM ) && tank ) {
			if ( K == KEY_N ) {
				tank->a           = 180;
				global.update_menu = true;
				K                 = 0;
			}

			if ( ( K == KEY_TAB ) || ( K == KEY_C ) ) {
				global.update_menu = true;
				bool done         = false;
				while ( !done ) {
					if ( ++tank->cw >= THINGS ) {
						tank->cw = 0;
					}

					if ( ( ( tank->cw < WEAPONS ) && tank->player->nm[ tank->cw ] )
					     || ( ( tank->cw >= WEAPONS ) && item[ tank->cw - WEAPONS ].selectable
					          && tank->player->ni[ tank->cw - WEAPONS ] ) ) {
						done = true;
					}
				}
				changed_weapon = false;
				K              = 0;
			}

			if ( ( K == KEY_BACKSPACE ) || ( K == KEY_Z ) ) {
				global.update_menu = true;
				bool done         = false;
				while ( !done ) {
					if ( --tank->cw < 0 ) {
						tank->cw = THINGS - 1;
					}

					if ( ( ( tank->cw < WEAPONS ) && tank->player->nm[ tank->cw ] )
					     || ( ( tank->cw >= WEAPONS ) && item[ tank->cw - WEAPONS ].selectable
					          && tank->player->ni[ tank->cw - WEAPONS ] ) ) {
						done = true;
					}
				}
				changed_weapon = false;
				K              = 0;
			}

			// put the tank under computer control
			if ( K == KEY_F10 ) {
				type = PART_TIME_BOT;
				K    = 0;
				return ( computer_controls( aicore, false ) );
			}

			// move the tank
			if ( ( K == KEY_COMMA ) || ( K == KEY_H ) ) {
				moved = tank->move_tank( DIR_LEFT );
			}
			if ( ( K == KEY_STOP ) || ( K == KEY_J ) ) {
				moved = tank->move_tank( DIR_RIGHT );
			}

			if ( moved ) {
				global.update_menu = true;
				K                 = 0;
			}

			// Fire Weapon
			if ( ( K == KEY_SPACE )
			     && ( ( ( tank->cw < WEAPONS ) && ( tank->player->nm[ tank->cw ] ) )
			          || ( ( tank->cw >= WEAPONS ) && ( tank->player->ni[ tank->cw - WEAPONS ] ) ) ) ) {

				gloating       = false;
				status         = CONTROL_FIRE;
				changed_weapon = false;
				K              = 0;
			}
		} // end of being in aim satge and having a tank
	}         // End of havig a key

	return status;
}

void CPlayer::initialise( bool loaded_game ) {
	// Initialize basic values if this is not loaded
	if ( !loaded_game ) {
		memset( nm, 0, sizeof( int32_t ) * WEAPONS );
		memset( ni, 0, sizeof( int32_t ) * ITEMS );

		ni[ ITEM_FUEL ] = 100; // Supply some initial fuel

		kills           = 0;
		killed          = 0;
		score           = 0;
	}

	last_opponent = nullptr;
	tank          = nullptr;
}

/// @brief read player data from a dump file.
bool CPlayer::load_from_file( FILE* file ) {
	if ( !file ) {
		return false;
	}

	char  line[ MAX_CONFIG_LINE + 1 ]  = { 0 };
	char  field[ MAX_CONFIG_LINE + 1 ] = { 0 };
	char  value[ MAX_CONFIG_LINE + 1 ] = { 0 };
	char* result                       = nullptr;

	setlocale( LC_NUMERIC, "C" );

	// read until we hit line "*CPlayer*" or "***" or EOF
	do {
		result = fgets( line, MAX_CONFIG_LINE, file );
		if ( !result || !strncmp( line, "***", 3 ) ) {
			// eof OR end of record
			return false;
		}
	} while ( 0 != strncmp( line, "*CPlayer*", 8 ) );

	bool done = false;

	while ( result && !done ) {
		// read a line
		memset( line, '\0', MAX_CONFIG_LINE );
		if ( ( result = fgets( line, MAX_CONFIG_LINE, file ) ) ) {

			// if we hit end of the record, stop
			if ( !strncmp( line, "***", 3 ) ) {
				done = true;
				continue; // This exits the loop as well
			}

			// strip newline character
			size_t line_length = strlen( line );
			while ( line[ line_length - 1 ] == '\n' ) {
				line[ line_length - 1 ] = '\0';
				line_length--;
			}

			// find equal sign
			size_t equal_position = 1;
			while ( ( equal_position < line_length ) && ( line[ equal_position ] != '=' ) ) {
				equal_position++;
			}

			// make sure the equal sign position is valid
			if ( line[ equal_position ] != '=' ) {
				continue; // Go to next line
			}

			// separate field from value
			memset( field, '\0', MAX_CONFIG_LINE );
			memset( value, '\0', MAX_CONFIG_LINE );
			strncpy( field, line, equal_position );
			strncpy( value, &( line[ equal_position + 1 ] ), MAX_CONFIG_LINE );

			// check which field we have and process value
			if ( !strcasecmp( field, "NAME" ) ) {
				name.assign( value );
				if ( name.length() > NAME_LEN ) {
					name.erase( NAME_LEN );
				}
			} else if ( !strcasecmp( field, "COLOR" ) ) {
				SAFE_STOI( color, value );
			} else if ( !strcasecmp( field, "DEFENSIVE" ) ) {
				SAFE_STOD( defensive, value );
			} else if ( !strcasecmp( field, "PAINSENSITIVITY" ) ) {
				SAFE_STOD( pain_sensitivity, value );
			} else if ( !strcasecmp( field, "PLAYED" ) ) {
				SAFE_STOUL( played, value );
			} else if ( !strcasecmp( field, "PREFTYPE" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				if ( ( val >= 0 ) && ( val <= ALWAYS_PREF ) ) {
					pref_type = static_cast< EPlayerPrefType >( val );
				}
			} else if ( !strcasecmp( field, "SELFPRESERVATION" ) ) {
				SAFE_STOD( self_preservation, value );
			} else if ( !strcasecmp( field, "TANK_BITMAP" ) ) {
				SAFE_STOI( tank_bitmap, value );
			} else if ( !strcasecmp( field, "TEAM" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				if ( ( val >= 0 ) && ( val <= TEAM_JEDI ) ) {
					team = static_cast< ETeamTypes >( val );
				}
			} else if ( !strcasecmp( field, "TYPE" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );

				if ( ( val >= HUMAN_PLAYER ) && ( val <= LAST_PLAYER_TYPE ) ) {
					type = static_cast< EPlayerType >( val );
				}

				// make sure previous human players are restored as humans
				if ( type == PART_TIME_BOT ) {
					type = HUMAN_PLAYER;
				}

			} else if ( !strcasecmp( field, "TYPESAVED" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				if ( ( val >= HUMAN_PLAYER ) && ( val <= LAST_PLAYER_TYPE ) ) {
					type_saved = static_cast< EPlayerType >( val );
					if ( type_saved > HUMAN_PLAYER ) {
						type = type_saved;
					}
				}
			} else if ( !strcasecmp( field, "VENGEANCETHRESHOLD" ) ) {
				SAFE_STOD( vengeance_threshold, value );
				// fix old configs
				if ( vengeance_threshold < 0.05 ) {
					vengeance_threshold =
						0.05 + ( static_cast< double >( get_rand() % 901 ) / 1000. ); // [0.05;0.95]
				}
				if ( vengeance_threshold > 0.95 ) {
					vengeance_threshold = 0.95;
				}
			} else if ( !strcasecmp( field, "VENGEFUL" ) ) {
				SAFE_STOI( vengeful, value );
				// fix old configs
				if ( vengeful < 1 ) {
					vengeful = 1 + ( get_rand() % 100 ); // [1;100]
				}
				if ( vengeful > 100 ) {
					vengeful = 100;
				}
			} else if ( !strcasecmp( field, "WON" ) ) {
				SAFE_STOUL( won, value );
			} else if ( !strcasecmp( field, "WEAPONPREFERENCES" ) ) {
				int32_t            wp_index = -1;
				int32_t            wp_value = -1;
				std::istringstream iss( value );
				iss >> wp_index >> wp_value;
				if ( ( wp_index < THINGS ) && ( wp_index >= 0 ) ) {
					weap_pref[ wp_index ] = wp_value;
				}
			} // end of valid data line
		}         // end of if we read a line properly
	}                 // end of while not done

	return true;
}

/** @brief Load player data from @a file which has the @a file_version.
 *
 * Version additions that are not found in earlier versions:
 * <ul>
 * <li>Version 65 : THEFT_BOMB
 * </ul>
 *
 * Version changes from earlier versions:
 * <ul>
 * <li>Version 65 : FUEL needs preferences
 * </ul>
 *
 **/
void CPlayer::load_game_data( FILE* file, int32_t file_version ) {
	if ( !file ) {
		return;
	}

	char  line[ MAX_CONFIG_LINE + 1 ]  = { 0 };
	char  field[ MAX_CONFIG_LINE + 1 ] = { 0 };
	char  value[ MAX_CONFIG_LINE + 1 ] = { 0 };
	char* result                       = nullptr;
	bool  done                         = false;
	bool  has_pref_loaded              = false;

	setlocale( LC_NUMERIC, "C" );


	do {
		// read a line
		memset( line, '\0', MAX_CONFIG_LINE );
		if ( ( result = fgets( line, MAX_CONFIG_LINE, file ) ) ) {

			// if we hit end of the record, stop
			if ( !strncmp( line, "***", 3 ) ) {
				done = true;
				continue; // This exits the loop as well
			}

			// strip newline character
			size_t line_length = strlen( line );
			while ( line[ line_length - 1 ] == '\n' ) {
				line[ line_length - 1 ] = '\0';
				line_length--;
			}

			// find equal sign
			size_t equal_position = 1;
			while ( ( equal_position < line_length ) && ( line[ equal_position ] != '=' ) ) {
				equal_position++;
			}

			// make sure the equal sign position is valid
			if ( line[ equal_position ] != '=' ) {
				continue; // Go to next line
			}

			// separate field from value
			memset( field, '\0', MAX_CONFIG_LINE );
			memset( value, '\0', MAX_CONFIG_LINE );
			strncpy( field, line, equal_position );
			strncpy( value, &( line[ equal_position + 1 ] ), MAX_CONFIG_LINE );

			// check which field we have and process value
			if ( !strcasecmp( field, "DEFENSIVE" ) ) {
				SAFE_STOD( defensive, value );
			} else if ( !strcasecmp( field, "PAINSENSITIVITY" ) ) {
				SAFE_STOD( pain_sensitivity, value );
			} else if ( !strcasecmp( field, "KILLED" ) ) {
				SAFE_STOI( killed, value );
			} else if ( !strcasecmp( field, "KILLS" ) ) {
				SAFE_STOI( kills, value );
			} else if ( !strcasecmp( field, "MONEY" ) ) {
				SAFE_STOI( money, value );
			} else if ( !strcasecmp( field, "SCORE" ) ) {
				SAFE_STOI( score, value );
			} else if ( !strcasecmp( field, "SELFPRESERVATION" ) ) {
				SAFE_STOD( self_preservation, value );
			} else if ( !strcasecmp( field, "TYPE" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				if ( ( val >= HUMAN_PLAYER ) && ( val < LAST_PLAYER_TYPE ) ) {
					type = static_cast< EPlayerType >( val );
				}
			} else if ( !strcasecmp( field, "TYPESAVED" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				if ( ( val >= HUMAN_PLAYER ) && ( val < LAST_PLAYER_TYPE ) ) {
					type_saved = static_cast< EPlayerType >( val );
				}
			} else if ( !strcasecmp( field, "VENGEANCETHRESHOLD" ) ) {
				SAFE_STOD( vengeance_threshold, value );
				// fix old configs
				if ( vengeance_threshold < 0.05 ) {
					vengeance_threshold =
						0.05 + ( static_cast< double >( get_rand() % 901 ) / 1000. ); // [0.05;0.95]
				}
				if ( vengeance_threshold > 0.95 ) {
					vengeance_threshold = 0.95;
				}
			} else if ( !strcasecmp( field, "VENGEFUL" ) ) {
				SAFE_STOI( vengeful, value );
				// fix old configs
				if ( vengeful < 1 ) {
					vengeful = 1 + ( get_rand() % 100 ); // [1;100]
				}
				if ( vengeful > 100 ) {
					vengeful = 100;
				}
			}

			// Preferences - saved if "PERPLAY_PREF" - type player.
			else if ( !strcasecmp( field, "WEAPONPREFERENCES" ) ) {
				int32_t            prf_idx = -1;
				int32_t            prf_val = -1;
				std::istringstream iss( value );
				iss >> prf_idx >> prf_val;
				if ( ( prf_idx > -1 ) && ( prf_idx < THINGS ) ) {

					/* === Version Checks for new weapons / items === */

					if ( ( file_version < 65 ) && ( prf_idx >= THEFT_BOMB ) ) {
						if ( THEFT_BOMB == prf_idx ) {
							// Generate a value
							weap_pref[ THEFT_BOMB ] = ROUND(
								( 150. + vengeful ) * ( static_cast< double >( type ) / 2. + .5 )
								* ( ( self_preservation + 2. ) / 2. )
								* ( std::abs( defensive ) + 1. )
							);
							DEBUG_LOG_EMO(
								name.c_str(),
								"New preference for %s : %5d",
								weapon[ THEFT_BOMB ].getName(),
								weap_pref[ THEFT_BOMB ]
							)
						}
						++prf_idx; // Skip new index value
					}                  // End of version 65 THEFT_BOMB

					/* === Version Checks for changed weapons / items === */

					if ( ( file_version < 65 ) ) {
						if ( ( ITEM_FUEL == ( prf_idx - WEAPONS ) ) && ( prf_val < 1 ) ) {
							// Generate a value
							prf_val = ROUND( MAX_WEAP_PROBABILITY / 60. * type / 2. + .5 );

							DEBUG_LOG_EMO(
								name.c_str(),
								"Changed preference for %s : %5d",
								item[ ITEM_FUEL ].getName(),
								prf_val
							)
						}
					} // End of version 65 ITEM_FUEL

					/* === Store data === */
					// (If someone edited the save game, the index might
					//  be too larger now, so check again to be safe!)
					if ( prf_idx < THINGS ) {
						weap_pref[ prf_idx ] = prf_val;
					}

					// separate very old from new save games
					has_pref_loaded = true;
				}
			}

			// Inventory of the weapons
			else if ( !strcasecmp( field, "CWeapon" ) ) {
				int32_t            weap_idx = -1;
				int32_t            weap_val = -1;
				std::istringstream iss( value );
				iss >> weap_idx >> weap_val;
				if ( ( weap_idx > -1 ) && ( weap_idx < WEAPONS ) ) {

					/* === Version Checks for new weapons === */

					if ( ( file_version < 65 ) && ( weap_idx >= THEFT_BOMB ) ) {
						++weap_idx; // Skip new index value
					}

					/* === Store data === */
					// (If someone edited the save game, the index might
					//  be too larger now, so check again to be safe!)
					if ( weap_idx < WEAPONS ) {
						nm[ weap_idx ] = weap_val;
					}
				}
			}

			// Inventory of the items
			else if ( !strcasecmp( field, "CItem" ) ) {
				int32_t            item_idx = -1;
				int32_t            item_val = -1;
				std::istringstream iss( value );
				iss >> item_idx >> item_val;
				if ( ( item_idx > -1 ) && ( item_idx < ITEMS ) ) {

					/* === Version Checks for new weapons === */
					// Currently there are no new items.

					ni[ item_idx ] = item_val;
				}
			}

			// Opponents Memory
			else if ( !strcasecmp( field, "OPPCOUNT" ) ) {
				int32_t safed_count = 0;
				SAFE_STOI( safed_count, value );

				// prepare the memory
				if ( opponents ) {
					delete[] opponents;
					opponents = nullptr;
				}

				if ( safed_count ) {
					try {
						opp_count  = safed_count;
						opponents = new opp_t[ opp_count ];
					} catch ( std::exception& e ) {
						cerr << "ERROR: Unable to allocate ";
						cerr << ( sizeof( opp_t ) * opp_count );
						cerr << " bytes for opponents array!" << endl;
						cerr << "ERROR: " << e.what() << endl;
						opp_count = 0;
					}
				} else {
					opp_count = 0;
				}
			} // end of oppcount handling
			else if ( !strcasecmp( field, "OPPMEM_INDX" ) ) {
				int32_t            opp_idx = -1;
				int32_t            opp_val = -1;
				std::istringstream iss( value );
				iss >> opp_idx >> opp_val;
				if ( ( opp_idx > -1 ) && ( opp_idx < opp_count ) ) {
					opponents[ opp_idx ].index    = opp_val;
					opponents[ opp_idx ].opponent = env.all_players[ opp_val ];
				}
			} else if ( !strcasecmp( field, "OPPMEM_DDEA" ) ) {
				int32_t            opp_idx = -1;
				int32_t            opp_val = -1;
				std::istringstream iss( value );
				iss >> opp_idx >> opp_val;
				if ( ( opp_idx > -1 ) && ( opp_idx < opp_count ) ) {
					opponents[ opp_idx ].damage_from = opp_val;
				}
			} else if ( !strcasecmp( field, "OPPMEM_DDON" ) ) {
				int32_t            opp_idx = -1;
				int32_t            opp_val = -1;
				std::istringstream iss( value );
				iss >> opp_idx >> opp_val;
				if ( ( opp_idx > -1 ) && ( opp_idx < opp_count ) ) {
					opponents[ opp_idx ].damage_to = opp_val;
				}
			} else if ( !strcasecmp( field, "OPPMEM_FEAR" ) ) {
				int32_t            opp_idx = -1;
				double             opp_val = 0.;
				std::istringstream iss( value );
				iss >> opp_idx >> opp_val;
				if ( ( opp_idx > -1 ) && ( opp_idx < opp_count ) ) {
					opponents[ opp_idx ].fear = opp_val;
				}
			} else if ( !strcasecmp( field, "OPPMEM_KIME" ) ) {
				int32_t            opp_idx = -1;
				int32_t            opp_val = -1;
				std::istringstream iss( value );
				iss >> opp_idx >> opp_val;
				if ( ( opp_idx > -1 ) && ( opp_idx < opp_count ) ) {
					opponents[ opp_idx ].killed_me = opp_val;
				}
			} else if ( !strcasecmp( field, "OPPMEM_KITH" ) ) {
				int32_t            opp_idx = -1;
				int32_t            opp_val = -1;
				std::istringstream iss( value );
				iss >> opp_idx >> opp_val;
				if ( ( opp_idx > -1 ) && ( opp_idx < opp_count ) ) {
					opponents[ opp_idx ].killed_them = opp_val;
				}
			}


		} // End of having a line
	} while ( result && !done );
	// End of reading player section


	// For backwards compatibility the preferences must be generated
	// if this is a PERPLAY type player but no preferences got saved.
	// This might be the case for very old save games.
	if ( !has_pref_loaded && ( PERPLAY_PREF == pref_type ) && ( type != HUMAN_PLAYER ) ) {
		generate_preferences();
	}
}

/// @brief reserve memory for the opponents array and fill it
void CPlayer::new_game() {
	if ( env.num_game_players ) {

		if ( opponents ) {
			delete[] opponents;
			opponents = nullptr;
		}

		try {
			opp_count  = env.num_game_players;
			opponents = new opp_t[ opp_count ];
		} catch ( std::exception& e ) {
			cerr << "ERROR: Unable to allocate " << ( sizeof( opp_t ) * opp_count );
			cerr << " bytes for opponents array!" << endl;
			cerr << "ERROR: " << e.what() << endl;
			opp_count = 0;
		}
	}

	if ( opp_count ) {
		for ( int32_t i = 0; i < opp_count; ++i ) {
			opponents[ i ].opponent = env.players[ i ];
			opponents[ i ].index    = env.players[ i ]->index;
		}
	}
}

// run this at the beginning of each turn
void CPlayer::new_round() {
	// if the player is under computer control, give it back to the player
	if ( type == PART_TIME_BOT ) {
		type = HUMAN_PLAYER;
	}

	if ( !tank ) {
		try {
			tank         = new CTank();
			tank->player = this;
		} catch ( std::exception& e ) {
			cerr << "FATAL: Error allocating memory for CTank in player.cpp:";
			cerr << __LINE__ << " : " << e.what() << endl;
			global.set_command( GLOBAL_COMMAND_QUIT );
		}
	}
	// tank->new_round() doesn't need to be called, because
	// the game loop will do that on tank placement.

	// if we are playing in a campaign, raise the AI level for every 20% played
	// rounds, so that useless players become deadly at 80% played rounds
	if ( env.campaign_mode && ( global.current_round < env.next_campaign_round ) && ( type > HUMAN_PLAYER )
	     && ( type < DEADLY_PLAYER ) ) {
		++type;
	}

	// reset some basic values
	changed_weapon    = false;
	time_left_to_fire = env.max_fire_time;
	skip_me           = false;
	last_shield_used  = 0;

	// Save damage from opponents if there was some not processed.
	// Although this would be done automatically once the AI takes
	// this player over the next time, lingering damage from the
	// last round can lead to panic actions and/or revenge actions
	// against players, who haven't fired, yet.
	// Noting the damage will raise the probability, but only once
	// the opponent had their first shot.
	for ( int32_t i = 0; i < opp_count; ++i ) {
		if ( opponents[ i ].damage_last > 0 ) {
			opponents[ i ].damage_from += opponents[ i ].damage_last;
			opponents[ i ].damage_last  = 0;
		}
	}
}

void CPlayer::note_damage_from( CPlayer* opponent, int32_t damage, bool destroyed ) {
	if ( opponent ) {
		int32_t idx       = opp_count;
		int32_t max_score = 0;

		for ( int32_t i = 0; i < opp_count; ++i ) {
			if ( opponents[ i ].revenge_dmg > max_score ) {
				max_score = opponents[ i ].revenge_dmg;
			}
			if ( opponents[ i ].opponent == opponent ) {
				idx = i;
			}
		}

		if ( idx < opp_count ) {
			opponents[ idx ].damage_last += damage;
			if ( destroyed ) {
				opponents[ idx ].killed_me++;
			}

			// If this one has the new top score and is not
			// the current revenge player, get a message out
			int32_t rev_dmg = opponents[ idx ].damage_last + opponents[ idx ].revenge_dmg;

			if ( ( opponents[ idx ].opponent != this ) && ( opponents[ idx ].opponent != revenge )
			     && ( rev_dmg > ( vengeance_threshold * tank->get_max_life() ) ) && ( rev_dmg > max_score ) ) {

				revenge = opponents[ idx ].opponent;

				if ( !global.skipping_computer_play ) {
					try {
						new CFloatText(
							select_revenge_phrase(),
							tank->x,
							tank->y - 30,
							.0,
							-.4,
							color,
							CENTRE,
							TS_NO_SWAY,
							300,
							false
						);
					} catch ( std::exception& e ) {
						std::cerr << __func__ << " new CFloatText: " << e.what() << std::endl;
					}
				}
			}
		} // end of having the opponent
	}         // end of having any opponent
}

void CPlayer::note_damage_to( CPlayer* opponent, int32_t damage, bool destroyed ) {
	if ( opponent ) {
		int32_t idx = 0;

		while ( ( idx < opp_count ) && ( opponent != opponents[ idx ].opponent ) ) {
			++idx;
		}

		if ( idx < opp_count ) {
			opponents[ idx ].damage_to += damage;
			if ( destroyed ) {
				opponents[ idx ].killed_them++;
			}
		}
	}
}

// if we have some shield strength at the end of the round, then
// reclaim this shield back into our inventory
void CPlayer::reclaim_shield() {
	if ( tank && last_shield_used && ( tank->sh > 0 ) ) {
		ni[ last_shield_used ] += 1;
	}
	last_shield_used = 0;
}

// This function takes one off the player's time to fire.
// If the player runs out of time, the function returns true.
// If the player has time left, or no time clock is being used,
// then the function returns false.
bool CPlayer::reduce_clock() {
	if ( !time_left_to_fire ) {
		// not using clock
		return false;
	}

	if ( 0 == --time_left_to_fire ) {
		time_left_to_fire = env.max_fire_time;
		return true;
	}

	return false;
}

/// @brief save game relevant data to @a file
void CPlayer::save_game_data( FILE* file ) {
	fprintf( file, "KILLED=%d\n", killed );
	fprintf( file, "KILLS=%d\n", kills );
	fprintf( file, "MONEY=%d\n", money );
	fprintf( file, "SCORE=%d\n", score );
	fprintf( file, "TYPE=%d\n", type );
	fprintf( file, "TYPESAVED=%d\n", type_saved );

	// Preferences, needed for "PERPLAY_PREF" - players
	if ( ( PERPLAY_PREF == pref_type ) && ( HUMAN_PLAYER != type ) ) {
		// Note: "ALWAYS_PREF" - players do not need this here, but in
		// save_to_file(), as the preferences are generated only once.
		fprintf( file, "DEFENSIVE=%lf\n", defensive );
		fprintf( file, "PAINSENSITIVITY=%lf\n", pain_sensitivity );
		fprintf( file, "SELFPRESERVATION=%lf\n", self_preservation );
		fprintf( file, "VENGEANCETHRESHOLD=%lf\n", vengeance_threshold );
		fprintf( file, "VENGEFUL=%d\n", vengeful );
		for ( int32_t i = 0; i < THINGS; ++i ) {
			fprintf( file, "WEAPONPREFERENCES=%d %d\n", i, weap_pref[ i ] );
		}
	}

	// Inventory of the weapons
	for ( int32_t i = 0; i < WEAPONS; ++i ) {
		fprintf( file, "CWeapon=%d %d\n", i, nm[ i ] );
	}

	// Inventory of the items
	for ( int32_t i = 0; i < ITEMS; ++i ) {
		fprintf( file, "CItem=%d %d\n", i, ni[ i ] );
	}

	// Opponents memory
	fprintf( file, "OPPCOUNT=%d\n", opp_count );
	for ( int32_t i = 0; i < opp_count; ++i ) {
		int32_t idx = opponents[ i ].index; // Just a shortcut

		// Save damage from last turn if any is still there:
		if ( opponents[ i ].damage_last > 0 ) {
			opponents[ i ].damage_from += opponents[ i ].damage_last;
			opponents[ i ].damage_last  = 0;
		}
		fprintf( file, "OPPMEM_INDX=%d %d\n", i, idx );
		fprintf( file, "OPPMEM_DDEA=%d %d\n", i, opponents[ i ].damage_from );
		fprintf( file, "OPPMEM_DDON=%d %d\n", i, opponents[ i ].damage_to );
		fprintf( file, "OPPMEM_FEAR=%d %lf\n", i, opponents[ i ].fear );
		fprintf( file, "OPPMEM_KIME=%d %d\n", i, opponents[ i ].killed_me );
		fprintf( file, "OPPMEM_KITH=%d %d\n", i, opponents[ i ].killed_them );
	}

	fprintf( file, "***\n" );
}

/// @brief dump full player data to @a file
void CPlayer::save_to_file( FILE* file ) {
	if ( !file ) {
		return;
	}

	// start section with "*CPlayer*"
	fprintf( file, "*CPlayer*\n" );
	fprintf( file, "NAME=%s\n", name.c_str() ); // Set first for easier debugging
	fprintf( file, "COLOR=%d\n", color );
	fprintf( file, "DEFENSIVE=%lf\n", defensive );
	fprintf( file, "PAINSENSITIVITY=%lf\n", pain_sensitivity );
	fprintf( file, "PLAYED=%u\n", played );
	fprintf( file, "PREFTYPE=%d\n", pref_type );
	fprintf( file, "SELFPRESERVATION=%lf\n", self_preservation );
	fprintf( file, "TANK_BITMAP=%d\n", tank_bitmap );
	fprintf( file, "TEAM=%d\n", team );
	fprintf( file, "TYPE=%d\n", type );
	fprintf( file, "TYPESAVED=%d\n", type_saved );
	fprintf( file, "VENGEANCETHRESHOLD=%lf\n", vengeance_threshold );
	fprintf( file, "VENGEFUL=%d\n", vengeful );
	fprintf( file, "WON=%u\n", won );

	// Preferences, needed for "ALWAYS_PREF" - players
	if ( ALWAYS_PREF == pref_type ) {
		// Note: "PERPLAY_PREF" - players do not need this here, but in
		// save_game_data(), as the preferences are different in each game.
		for ( int32_t i = 0; i < THINGS; ++i ) {
			fprintf( file, "WEAPONPREFERENCES=%d %d\n", i, weap_pref[ i ] );
		}
	}

	fprintf( file, "***\n" );
}

char const* CPlayer::select_gloat_phrase() {
	return env.gloat->Get_Random_Line();
}

/// @return a constructed panic phrase which must be freed!
char const* CPlayer::select_panic_phrase( CPlayer* shocker ) {
	if ( !shocker ) {
		return nullptr;
	}

	char const* line  = env.panic->Get_Random_Line();
	size_t      tLen  = strlen( shocker->get_name() ) + strlen( line );
	char*       pText = (char*)calloc( tLen + 1, sizeof( char ) );

	if ( !pText ) {
		return nullptr;
	}

	snprintf( pText, tLen, line, shocker->get_name() );

	return pText;
}

char const* CPlayer::select_kamikaze_phrase() {
	return env.kamikaze->Get_Random_Line();
}

/// @return a constructed retaliation phrase which must be freed!
char const* CPlayer::select_retaliation_phrase() const {
	if ( !revenge ) {
		return nullptr;
	}

	char const* line  = env.retaliation->Get_Random_Line();
	char const* rname = revenge->get_name();
	size_t      tLen  = strlen( rname ) + 4 + strlen( line );
	char*       pText = (char*)calloc( tLen + 1, sizeof( char ) );

	if ( pText ) {
		atanks_snprintf( pText, tLen, "%s%s !!!", line, rname );
	}

	return pText;
}

char const* CPlayer::select_revenge_phrase() {
	return env.revenge->Get_Random_Line();
}

char const* CPlayer::select_suicide_phrase() {
	return env.suicide->Get_Random_Line();
}

/// @brief store @a last_opp to be remembered as the current/last target
void CPlayer::set_last_opponent( sOpponent* last_opp ) {
	last_opponent = last_opp;
}

void CPlayer::set_name( char const* name_ ) {
	if ( !name_ || ( ( name != name_ ) ) ) {
		name.assign( name_ ? name_ : "" );
	}
}

/// @brief fill in the list of desired items and update their preferences
void CPlayer::update_preferences( int32_t max_boost, int32_t max_score ) {
	// 1.: Fill cart and preference array.
	// The preferences are copied, as they might get boosted this round
	int32_t weapons_in_stock = generate_desired_list();
	auto    ai_level         = static_cast< int32_t >( type );

	// 2.: Amplify wish list by current boost and score situation
	need_amp    = false;
	need_armour = false;
	need_damage = false;

	// Check whether boosting armour / amps is wanted:
	if ( get_boost_value() < ( max_boost / ai_level ) ) {
		// Yes. which?
		double amp_val = get_amp_value();
		double arm_val = get_armour_value();

		// Amplifier rating for offensive bot: 3:1, and for defensive bot: 3:5
		double amp_want = defensive < 0 ? arm_val - ( amp_val / 3. ) : ( arm_val / 5. ) - ( amp_val / 3. );

		// Armour rating for offensive bot: 3:5, and for defensive bot: 3:1
		double arm_want = defensive < 0 ? ( amp_val / 5. ) - ( arm_val / 3. ) : amp_val - ( arm_val / 3. );

		if ( amp_want > arm_want ) {
			DEBUG_LOG_FIN( name.c_str(), "updPref: Need to boost amps    (%d / %d)", get_boost_value(), max_boost / ai_level )
			need_amp = true; // Try to come back with more damage output
		} else {
			DEBUG_LOG_FIN( name.c_str(), "updPref: Need to boost armour  (%d / %d)", get_boost_value(), max_boost / ai_level )
			need_armour = true; // Try to come back with more endurance
		}
	}

	// Fallen behind? Need more weapons?
	if ( ( score <= ( max_score / ( ai_level + 1 ) ) ) && ( weapons_in_stock < ( 2 * ai_level ) ) ) {
		DEBUG_LOG_FIN( name.c_str(), "updPref: Need to boost weapons (%d / %d)", score, max_score / ( ai_level + 1 ) )
		need_damage = true;
	}

	// Account for boosted preferences from last round
	if ( ( boost_pref[ WEAPONS + ITEM_VIOLENT_FORCE ] > 0 ) || ( boost_pref[ WEAPONS + ITEM_INTENSITY_AMP ] > 0 ) ) {
		need_amp = true;
	}
	if ( ( boost_pref[ WEAPONS + ITEM_PLASTEEL ] > 0 ) || ( boost_pref[ WEAPONS + ITEM_ARMOUR ] > 0 ) ) {
		need_armour = true;
	}


	// 3.: Boost preferences if wanted and lower weapon/item
	//     preferences if there are enough in stock already.
	//     Further note down items to sell.
	boost_prefences( need_armour, need_amp, need_damage );


	// 4.: Sort these items by preferences
	bool isSorted = false;
	while ( !isSorted ) {
		isSorted = true;

		for ( int32_t i = 1; i < THINGS; ++i ) {
			int32_t idx_l = desired[ i - 1 ];
			int32_t idx_r = desired[ i ];

			if ( ( curr_pref[ idx_l ] < curr_pref[ idx_r ] )
			     // sort SML_MIS to the back, too
			     || ( ( 0 == idx_l ) && idx_r ) ) {
				isSorted         = false;
				desired[ i ]     = idx_l;
				desired[ i - 1 ] = idx_r;
			}
		}
	}

#ifdef ATANKS_DEBUG_FINANCE
	// Get out the top twenty
	for ( int32_t i = 0; i < THINGS; ++i ) {
		DEBUG_LOG_FIN(
			name.c_str(),
			"%2d. preference: %6d - %s",
			i + 1,
			curr_pref[ desired[ i ] ],
			desired[ i ] < WEAPONS ? weapon[ desired[ i ] ].getName() : item[ desired[ i ] - WEAPONS ].getName()
		)
	}
#endif // ATANKS_DEBUG_FINANCE
}

/// @brief mini ctor to pacify Visual C++
PlayerMini::PlayerMini() = default;

/// @brief backup a players editable data
void PlayerMini::copy_from( CPlayer* source ) {
	if ( source ) {
		assert( ( source->index > -1 ) && "INDEX ERROR on CPlayer!" );
		color = source->color;
		index = source->index;
		strncpy( name, source->get_name(), NAME_LEN );
		played     = source->played;
		player     = source;
		pref_type   = source->pref_type;
		tank_bitmap = source->tank_bitmap;
		team       = source->team;
		type       = source->type;
		won        = source->won;
	}
}

/// @brief copy backed up values back to the source player
void PlayerMini::write_back( CPlayer* target ) {
	if ( target ) {
		player = target;
	}
	if ( player ) {
		player->color = color;
		player->set_name( name );
		// played is read only.
		player->pref_type   = pref_type;
		player->tank_bitmap = tank_bitmap;
		player->team       = team;
		player->type       = type;
		// won is read only.
	}
}

/// @brief action function to display the edit player screen
int32_t edit_player( CPlayer** target, int32_t ) {
	int32_t result = 0;

	assert( target && "ERROR: target must be set" );
	assert( *target && "ERROR: target must point to something valid!" );

	if ( !target || !( *target ) ) {
		return -1;
	}

	int32_t menuMid        = 300;
	int32_t itemLeft       = menuMid - 75;
	int32_t itemHeight     = env.font_height + 2;
	int32_t itemPadding    = 2;
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t itemY          = itemFullHeight * 3;
	int32_t btnHeight      = env.misc[ 7 ]->h + itemPadding;
	int32_t menuHeight     = env.menu_end_y - env.menu_begin_y; // Raw height


	// Use "Mini-Player" struct to be able to cancel player editing
	PlayerMini player_bak;
	player_bak.copy_from( *target );

	// The "Are you sure" screen when deleting a player
	CMenu areyousure( MC_AREYOUSURE, env.half_width - menuMid, env.menu_begin_y );
	areyousure.add_button(
		1,
		nullptr,
		PE_CONFIRM_DEL,
		env.misc[ 7 ],
		nullptr,
		env.misc[ 8 ],
		false,
		menuMid + 50,
		menuHeight - btnHeight - 6,
		0,
		0,
		itemPadding
	);
	areyousure.add_button(
		2,
		nullptr,
		PE_BACK,
		env.misc[ 7 ],
		nullptr,
		env.misc[ 8 ],
		false,
		menuMid - env.misc[ 7 ]->w - 50,
		menuHeight - btnHeight - 6,
		0,
		0,
		itemPadding
	);

	// The menu, but with the player name as title
	CMenu menu( MC_PLAYER, env.half_width - menuMid, env.menu_begin_y );
	menu.set_title( player_bak.name, false );

	// "Name"
	menu.add_text( player_bak.name, 1, NAME_LEN, player_bak.color, "%s", itemLeft, itemY, 150, itemHeight, itemPadding );
	itemY += itemFullHeight;

	// "Colour"
	menu.add_color( &player_bak.color, 2, itemLeft, itemY, 150, 50, 25, itemPadding );
	itemY += 50 + itemPadding;

	// "Type"
	menu.add_value(
		&player_bak.type,
		3,
		nullptr,
		BLACK,
		TC_PLAYERTYPE,
		static_cast< int32_t >( DEADLY_PLAYER ),
		itemLeft,
		itemY,
		150,
		itemHeight,
		itemPadding
	);
	itemY += itemFullHeight;

	// "Team"
	menu.add_value(
		&player_bak.team,
		4,
		nullptr,
		BLACK,
		TC_PLAYERTEAM,
		static_cast< int32_t >( TEAM_JEDI ),
		itemLeft,
		itemY,
		150,
		itemHeight,
		itemPadding
	);
	itemY += itemFullHeight;

	// "Generate Pref"
	menu.add_value(
		&player_bak.pref_type,
		5,
		nullptr,
		BLACK,
		TC_PLAYERPREF,
		static_cast< int32_t >( ALWAYS_PREF ),
		itemLeft,
		itemY,
		150,
		itemHeight,
		itemPadding
	);
	itemY += itemFullHeight;

	// "Played"
	menu.add_text( &player_bak.played, 6, BLACK, "% 8u", itemLeft, itemY, 150, itemHeight, itemPadding );
	itemY += itemFullHeight;

	// "Won"
	menu.add_text( &player_bak.won, 7, BLACK, "% 8u", itemLeft, itemY, 150, itemHeight, itemPadding );
	itemY += itemFullHeight;

	// "Tank Type"
	menu.add_value(
		&player_bak.tank_bitmap,
		8,
		nullptr,
		BLACK,
		TC_TANKTYPE,
		static_cast< int32_t >( TT_MINI ),
		itemLeft,
		itemY,
		150,
		35,
		itemPadding,
		display_tank_desc
	);
	itemY += 35 + itemPadding;

	// "Delete This Player"
	menu.add_menu( &areyousure, 9, RED, itemLeft, itemY, 150, itemFullHeight, itemPadding );

	// "Okay" and "Back"
	menu.add_button(
		10,
		nullptr,
		PE_CONFIRM_EDIT,
		env.misc[ 7 ],
		nullptr,
		env.misc[ 8 ],
		false,
		menuMid + 50,
		menuHeight - btnHeight - 6,
		0,
		0,
		itemPadding
	);
	menu.add_button(
		11,
		nullptr,
		PE_BACK,
		env.misc[ 7 ],
		nullptr,
		env.misc[ 8 ],
		false,
		menuMid - env.misc[ 7 ]->w - 50,
		menuHeight - btnHeight - 6,
		0,
		0,
		itemPadding
	);

	result = menu();

	// If the editing is confirmed, the backup must be written back
	if ( PE_CONFIRM_EDIT & result ) {
		player_bak.write_back();
	}

	// If the player shall be deleted, the player index must be added
	if ( PE_CONFIRM_DEL & result ) {
		result |= player_bak.index;
	}

	return result;
}

static PlayerMini player_new; //!< Used by new_player to keep previous settings

/// @brief action function to display the edit player screen
int32_t new_player( CPlayer** target, int32_t ) {
	int32_t result = 0;

	assert( target && "ERROR: target must be set" );
	assert( ( nullptr == *target ) && "ERROR: *target must nullptr!" );

	if ( !target || *target ) {
		return -1;
	}

	int32_t menuMid        = 300;
	int32_t itemLeft       = menuMid - 75;
	int32_t itemHeight     = env.font_height + 2;
	int32_t itemPadding    = 2;
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t itemY          = itemFullHeight * 3;
	int32_t btnHeight      = env.misc[ 7 ]->h + itemPadding;
	int32_t menuHeight     = env.menu_end_y - env.menu_begin_y; // Raw height

	// The menu, with title from the menu class
	CMenu menu( MC_PLAYER, env.half_width - menuMid, env.menu_begin_y );

	// "Name"
	menu.add_text( player_new.name, 1, NAME_LEN, player_new.color, "%s", itemLeft, itemY, 150, itemHeight, itemPadding );
	itemY += itemFullHeight;

	// "Colour"
	menu.add_color( &player_new.color, 2, itemLeft, itemY, 150, 50, 25, itemPadding );
	itemY += 50 + itemPadding;

	// "Type"
	menu.add_value(
		&player_new.type,
		3,
		nullptr,
		BLACK,
		TC_PLAYERTYPE,
		static_cast< int32_t >( DEADLY_PLAYER ),
		itemLeft,
		itemY,
		150,
		itemHeight,
		itemPadding
	);
	itemY += itemFullHeight;

	// "Team"
	menu.add_value(
		&player_new.team,
		4,
		nullptr,
		BLACK,
		TC_PLAYERTEAM,
		static_cast< int32_t >( TEAM_JEDI ),
		itemLeft,
		itemY,
		150,
		itemHeight,
		itemPadding
	);
	itemY += itemFullHeight;

	// "Generate Pref"
	menu.add_value(
		&player_new.pref_type,
		5,
		nullptr,
		BLACK,
		TC_PLAYERPREF,
		static_cast< int32_t >( ALWAYS_PREF ),
		itemLeft,
		itemY,
		150,
		itemHeight,
		itemPadding
	);
	itemY += itemFullHeight;

	// "Played" and "Won" do not make sense here

	// "Tank Type"
	menu.add_value(
		&player_new.tank_bitmap,
		8,
		nullptr,
		BLACK,
		TC_TANKTYPE,
		static_cast< int32_t >( TT_MINI ),
		itemLeft,
		itemY,
		150,
		35,
		itemPadding,
		display_tank_desc
	);
	itemY += 35 + itemPadding;

	// "Delete This Player" is surely not needed

	// "Okay" and "Back"
	menu.add_button(
		10,
		nullptr,
		PE_CONFIRM_NEW,
		env.misc[ 7 ],
		nullptr,
		env.misc[ 8 ],
		false,
		menuMid + 50,
		menuHeight - btnHeight - 6,
		0,
		0,
		itemPadding
	);
	menu.add_button(
		11,
		nullptr,
		PE_BACK,
		env.misc[ 7 ],
		nullptr,
		env.misc[ 8 ],
		false,
		menuMid - env.misc[ 7 ]->w - 50,
		menuHeight - btnHeight - 6,
		0,
		0,
		itemPadding
	);

	while ( !result ) {
		char existsMessage[ 200 ];
		result = menu();

		// If the player is to be created, two things must happen.
		// First, ensure that the name is unique
		// Second, create the real player
		if ( PE_CONFIRM_NEW & result ) {
			if ( -1 == env.get_player_by_name( player_new.name ) ) {
				*target = env.create_new_player( player_new.name );
				if ( *target ) {
					player_new.write_back( *target );
				}
			} else {
				snprintf( existsMessage, 199, "The player \"%s\" already exists!", player_new.name );
				errorMessage = existsMessage;
				errorX       = env.half_width - text_length( font, errorMessage ) / 2;
				errorY       = env.menu_begin_y + itemFullHeight;
				result       = 0;
			}
		}
	} // End of !result

	return result;
}
