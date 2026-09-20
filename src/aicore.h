#pragma once
#ifndef ATANKS_AICORE_H_INCLUDED
#  define ATANKS_AICORE_H_INCLUDED ///< Include guard.

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
 *
 */

/** @file aicore.h
 * @brief Home of the CAICore class
 *
 * This class substituted the old AI code in the CPlayer class.
 *
 * While the old AI worked pretty well in most situations, it did
 * lag the game on more intense calculations.
 * Further there were some situations (like peaks in the way and
 * such things) that the old AI could not handle.
 * It could have been fixed, but would have lagged the game even more.
 *
 * By outsourcing the AI to its own class and providing an
 * operator()(), the AI calculations could be put in a background
 * thread. This resulted in a much smoother gaming experience.
 *
 * Further the AI is now easier to debug, to maintain and to extend
 * than it was possible with the old AI code.
 **/


#  include "floattext.h"
#  include "globaltypes.h"
#  include "item.h"
#  include "player_types.h"
#  include "weapon.h"

#  include <condition_variable>

// Init and check for round scores
/// Neutral round score baseline.
#  define NEUTRAL_ROUND_SCORE ( -1000000 )

#  ifndef HAS_PLAYER
class CPlayer;
struct sOpponent;
#  endif // HAS_PLAYER
#  ifndef HAS_TANK
class CTank; // forwarding if not known
#  endif    // HAS_TANK

// These are restricted to aicore.cpp, as
// they are of no use anywhere else. - sed
struct sItemListEntry;
struct sOppMemEntry;
struct sWeapListEntry;

/** @class CAICore
 * @brief Core AI, written to be used as a background thread.
 *
 * The AI operator() is the main function that does all the work.
 *
 * If you need to edit something in that work flow, please try to
 * keep the documentation up-to-date.
 *
 * The following is the work flow in CAICore::operator().
 *
 *
 * Initialization in initialize():
 * --------------------------------
 * ai_level      : (player->type) This is meant as a short cut being a widely usable int32_t.
 * ai_level_d    : (ai_level)     ai_level cast to double.
 * ai_over_mod   : (from level)   Multiplier [1.1;1.5] according to ai_level_d, used to evaluate overkills and similar.
 * ai_type_mod   : (from level)   Multiplier [1.0;3.0] according to ai_level_d, used on important decisions to
 *                                strengthen higher bot levels.
 * blast_*       : (0)            Damage values of the available normal missiles.
 * can_move       : (true)         As long as it is true, allows the AI to consider using fuel/rockets.
 * is_moved_by     : (0)            Set from the outside via has_moved() to indicate the result of movement attempts.
 * is_shocked     : (false)        Whether the bot is shocked by anothers massive damage they dealt.
 * revengee      : (nullptr)      sOpponent pointer set to a bot this one wants revenge against.
 * shocker       : (nullptr)      sOpponent pointer set to the bot that caused is_shocked to become true.
 * need_success   : (true)         This is set to true and later set to false if a full targetting round resulted in a
 *                                primary target hit, or the full score is greater than zero.
 * need_aim       : (true)         Causes aim() to be called. Set to false if the bot needs to free themselves.
 * is_blocked     : (false)        Set to true if an obstacle is detected or the bot is buried.
 * hill_detected : (false)        Set to true if the progress in aiming suggest, that a hill is in the firing path.
 * need_money     : (check done)   Set to true if get_money_to_save() returns more money than the player has.
 * tank          : (player->tank) Short cut.
 * angle         : (tank->a)      Written back current angle set on the tank.
 * power         : (tank->p)      Written back current power set on the tank.
 * weap_idx      : (tank->cw)     Written back currently chosen weapon.
 * x             : (tank->x)      Written back current x position of the tank.
 * y             : (tank->y)      Written back current y position of the tank.
 * curr_life      : (tank->l + sh) Both live and remaining shield strength.
 * buried        : (check done)   Number of true angles where the turret is directly covered by dirt.
 * buried_l      : (check done)   Same as buried, but left side only
 * buried_r      : (check done)   Same as buried, but right side only
 * max_life       : (check done)   The current max_life value of the tank for this game round.
 *
 * Information from last round:
 * last_opp      : (from player)  sOpponent pointer set to the opponent that was attacked last.
 * last_ang      : (180 / angle)  If a last_opp is set, store current tank angle, initialize with 180 otherwise.
 * last_pow      : (1000 / power) If a last_opp is set, store current tank power, initialize with 1000 otherwise.
 * last_weap     : (0 / weapon)   If a last_opp is set, store current tank weapon, initialize with 0 (Small Missile)
 *                                otherwise.
 *                                If a last weapon/item is set, it is selected for the current first evaluation, too.
 *                                This is done so the first weapon/item selection will chose something different than
 *                                was tried in the last round.
 *
 * Information for the current targetting round:
 * curr_angle      : (angle)               The angle for the current aiming round.
 * curr_power      : (power)               The power for the current aiming round.
 * best_round_score: (NEUTRAL_ROUND_SCORE) Best score achieved for the current opponent.
 *
 * Best achieved values over the full targetting cycle:
 * best_setup_angle     : (angle)
 * best_setup_damage    : (0)
 * best_setup_item      : (nullptr)
 * best_setup_mem       : (nullptr)
 * best_setup_overshoot : (MAX_OVERSHOOT)
 * best_setup_power     : (power)
 * best_setup_score     : (NEUTRAL_ROUND_SCORE)
 * best_setup_weap      : (nullptr)
 *
 * Eventually a check is made to decide whether the bot gets lucky. This temporarily raises its AI level.
 *
 *
 * Initialization in  check_opp_mem():
 * --------------------------------
 * Every opponent entry is evaluated, and a (possible) current shocker and/or revengee are determined.
 * The revengee is written back to the player if it is a new one.
 * The opponent memory list is then sorted by score in descending order.
 *
 *
 * Re-Check shocked state:
 * --------------------------------
 * At this point it is checked again whether the bot is really in terror. Although difficult,
 * it is possible for a bot to overcome their shock here. This is done so the bots won't be
 * shocked almost constantly from round 2 on.
 *
 *
 * Initialization in operator():
 * --------------------------------
 * findTgtAttempts : Targetting attempts. Number of full cycles of finding an attack setup.
 *                   Counted using tgt_attempts in the operator() loop.
 * findOppAttempts : Opponent attempts. How many opponents are tried per targetting attempt.
 *                   Counted using opp_attempts in the operator() loop.
 * findWeapAttempts: Weapon attempts. How many weapons / items are tried per opponent attempt.
 *                   Counted using weap_attempts in the operator() loop.
 * findRngAttempts : Range find attempts. How many corrections to the aiming are done per weapon attempt.
 *                   Counted using rng_attempts in aim() loop.
 * focus_rate       : This is a modifier that lessens some corrections the lower the ai level is.
 * error_multiplier : This is a modifier that lessens some errors the higher the ai level is.
 * max_bounce       : The higher the AI level the more bounces/wraps from walls it can follow before
 *                   assuming the shot has crashed.
 *
 * The results should be [if shocked]:
 * findTgtAttempts : Useless:  2   [1], Deadly + 1:  7    [1]
 * findOppAttempts : Useless   3   [2], Deadly + 1:  8    [4]
 * findWeapAttempts: Useless:  2   [1], Deadly + 1: 12    [2]
 * findRngAttempts : Useless: 10   [2], Deadly + 1: 60    [7]
 * focus_rate       : Useless:  0.166,   Deadly + 1:  1.0
 * error_multiplier : Useless:  1.2 [3], Deadly + 1:  0.02 [0.14]
 * max_bounce       : Useless:  3,       Deadly + 1: 20
 *
 * As the full number of aimings can be up to Tgt*Opp*Weap*Rng attempts, the current targetting attempt
 * is finished once a new best attack plan is found.
 *
 * The full cycle of target selection, weapon/item selection, setting up the basic combat values and
 * targeting the selected weapon might need a few attempts.
 * The higher the AI level, the more attempts the bot gets. If the maximum number of attempts is
 * reached, all used methods are forced to come up with a minimum result.
 *
 * Local counters for the operator() loop:
 * ---------------------------------------
 * tgt_attempts  : (0)     The number of attempts to set up a succesful attack the AI already had.
 * opp_attempts  : (0)     The number of attempts to find a suitable opponent the AI already had.
 * weap_attempts : (0)     The number of attempts to find a suitable weapons or item the AI already had.
 * done          : (false) Control variable set by each section method on success (true) or failure (false).
 *
 * Cycle in operator():
 * --------------------------------
 *
 *   The loop is working until the AI can no longer work, aiming is needed or the AI is not blocked, and
 *   tgt_attempts is lower than findTgtAttempts.
 *
 *   1  Cycle target and item selection if both opp_attempts and weap_attempts are both zero.
 *      Those are combined, because selecting a different target later might make the current item selection less
 *      effective or even useless. Thus the item is chosen individually.
 *      tgt_attempts : (+1)      The new target and item selections marks the beginning of a new targetting attempt.
 *      mem_curr     : (nullptr) The currently selected opponent.
 *
 *   2  done = setup_attack(bool is_last, int32_t &opp_attempt, int32_t &weap_attempt)
 *
 *        Param 1 : is_last     : is set to true if tgt_attempts equals findTgtAttempts.
 *        Param 2 : opp_attempt : Reference to opp_attempts to have the actual opponent selection counted.
 *        Param 3 : weap_attempt: Reference to weap_attempts to have the actual weapon/item selection counted.
 *
 *      Here, the basic setup is done by selecting an opponent and a suitable item
 *      or weapon to use against them.
 *
 *     2.1  If either weap_attempt is 0 or mem_curr is nullptr, a new opponent selection round is started.
 *          mem_curr can be nullptr if the previous try to find an opponent failed.
 *          pl_stage     : (PS_SELECT_TARGET)
 *          opp_attempt : (+1) Raised by one when calling select_target() below.
 *
 *       2.1.1 selectDone = select_target(bool is_last)
 *               Param 1 : is_last : This is set to true if the raised opp_attempt equals findOppAttempts and is_last
 *                                   was already set to true in setup_attack() and need_success is true.
 *         2.1.1.1 If the bot is shocked, their shocker is preselected and true is returned.
 *         2.1.1.2 If the bot has a grudge against someone, the revengee is preselected and true is returned, but only
 *                 if either no opponent was selected yet, or the last opponent was someone else.
 *                 However, if is_last is true, the revengee is selected even if it is two times in a row this way.
 *         2.1.1.3 If nothing was preselected, the ordered list of opponents is simply walked down. However, if is_last
 *                 is true, the first entry is always selected, because they are primarily wanted anyway.
 *
 *       2.1.2 If an opponent is selected and it is not the same as the last time one was selected, then the
 *             following initializations are done:
 *             item_curr   : (nullptr) The currently selected item or nullptr if weap_curr is used.
 *             item_last   : (nullptr) The item selected in the last item selection or nullptr if none was selected.
 *             weap_curr   : (nullptr) The currently selected weapon or nullptr if item_curr is used.
 *             weap_last   : (nullptr) The weapon selected in the last weapon selection or nullptr if none was
 *                                     selected. item_* and weap_* are used for weap_attempts, which are done
 *                                     findWeapAttempts times for each opp_attempts cycle.
 *             best_round_score : (NEUTRAL_ROUND_SCORE) A new target is always a complete new targetting round.
 *
 *          To make this clear:
 *            - For each targeting attempt (tgt_attempts count to findTgtAttempts) up to findOppAttempts attempts are
 *              granted to find a suitable opponent.
 *            - For each opponent selection attempt (opp_attempts count to findOppAttempts) up to findWeapAttempts
 *              attempts are granted to select a suitable item or weapon.
 *            - For each weapon/item selection (weap_attempts count to findWeapAttempts) up to findRngAttempts attempts
 *              are granted to the AI to actually aim the selected weapon/item on the selected opponent.
 *
 *     2.2  Otherwise, if both weap_attempt is greater than 0 and mem_curr is not nullptr, the current target is kept
 *          and it is recorded that no new target was selected.
 *
 *     2.3  If the target selection was successful, the next item to use or weapon to fire can be selected.
 *          pl_stage      : (PS_SELECT_WEAPON)
 *          weap_attempt : (+1) Raised by one when calling select_item() below.
 *
 *       2.3.1 If the current target is new, the score lists for items and weapons are regenerated.
 *       2.3.2 At this point selectDone is set to false.
 *       2.3.3 While selectDone is false, and the AI can work, and weap_attempt is lower than findWeapAttempts,
 *             selectDone gets the return value of select_item(bool is_last).
 *               Param 1 : is_last : This is set to true if the raised weap_attempt equals findWeapAttempts and is_last
 *                                   was already set to true in setup_attack() and need_success is true.
 *
 *         2.3.3.1 store item_curr in item_last and weap_curr in weap_last. This is needed for the regular walking
 *                 down the ordered lists of weapons and items.
 *         2.3.3.2 If the bot is shocked, or if it already has a best setup where the primary target was hit, a random
 *                 weapon is chosen. This is done to give bots a wider range of opportunities to find an even better
 *                 setup with weapons they would normally not chose in their current situation.
 *                 And, on the other hand, a shocked bot does not really think but selects something random in panic.
 *                 Reducers, Dirt weapons and weapons they do not have in stock are skipped.
 *         2.3.3.3 Otherwise advance in the list of weapons. If no weapon was found (list exhausted), the first entry
 *                 is preselected.
 *         2.3.3.4 A shocked bot now ensures no item is selected, and if no weapon was available, the first one is
 *                 chosen. Here the method ends and returns true.
 *         2.3.3.5 Not shocked bots now rotate through their list of available items. If no items are left to try,
 *                 item_curr is set to nullptr to indicate that no item selection is possible.
 *         2.3.3.6 Unless the opponent has more than ten times the live points (including shields) than the bot,
 *                 kamikaze selections are undone.
 *         2.3.3.7 Unless the bot is buried or blocked, teleporter item and riot weapon selections are undone.
 *         2.3.3.8 If both a weapon and an item was found, unselect the one with the lower score.
 *         2.3.3.9 If neither weapon nor item was selected, but is_last is true, the small missile is selected.
 *
 *         Eventually return true if either weapon or item is selected and false if both are nullptr.
 *
 *       2.3.4 If all weapon selection attempts were used, but there are opponent selections left, weap_attempt is
 *             reset to zero so another opponent can be selected.
 *
 *     2.4  If all weapon selection attempts for all opponent selection attempts were used up, breakUp is set to true
 *          to indicate that all that could be done was tried.
 *     2.5  If everything failed and this is the very last setup attempt, an emergency plan kicks in:
 *          a) Try to teleport away.
 *          b) If the bot has no teleporter in stock and the currently selected opponent is not buried, try to select
 *             a swapper.
 *          c) If no swapper was available or the currently selected opponent is buried, try the mass teleport.
 *          d) If the bot has no mass teleport, select small missile.
 *     2.6  If an item or weapon is chosen that shall be used to self destruct, check again whether this is really
 *          wanted. If so, set mem_curr to the own entry, otherwise the selection either failed, or, if this is the
 *          very last attempt and there was no successful setup, yet, revert to small missile.
 *     2.7  If breakUp was set to true and this is not the last attempt, opp_attempt and weap_attempt are both reset
 *          to zero to trigger a new full targetting cylcle.
 *
 *   3  done = calc_attack(int32_t attempt) - called if setup_attack() returned true.
 *
 *        Param 1 : Value of tgt_attempts
 *
 *      Here the basic attack values are calculated, trying to find an angle and a power value to begin with. These are
 *      then checked and adapted according to the chosen item or weapon, the selected target, the situation of the bots
 *      tank and whether the current level has a ceiling ("boxed mode") or not.
 *
 *      bool is_last   : (checked)       Set to true if attempt equals findTgtAttempts and no successful setup was
 *                                       found, yet.
 *      pl_stage        : (PS_CALCULATE)  AI enters the calculation stage.
 *      is_blocked      : (false)         Will be set to true if the aiming finds out that a hill blocks the path.
 *      has_flipped     : (false)         Set to true by calc_standard() if flipped towards a wall.
 *      need_aim        : (false)         Will be set to true if a weapon is chosen that needs aiming.
 *      curr_overshoot : (MAX_OVERSHOOT) The overshoot for the current aiming round.
 *      offset_x       : (0)             Some weapons need a horizontal offset to aim at, like the napalm weapons.
 *      offset_y       : (0)             Some weapons need a vertical offset to aim at, like the driller.
 *
 *     3.1  If an item is chosen, the currently set angle and power are written into curr_angle and curr_power. There
 *          is no need for further investigation and aiming, so the method then returns true.
 *     3.2  If the currently set opponent equals last_opp, and the currently set weapon is the same as what was used
 *          against the last opponent, copy back the angle and power used in the last round and be done.
 *          It is then tried to optimized the last attack.
 *          This is only done if the same weapon is used, as other weapons might need different approaches.
 *     3.3  If a laser is chosen and the tank is not buried enough to be evaluated as buried or blocked, return the
 *          result of calc_laser(is_last is_last).
 *
 *            Param 1 : True if is_last in calc_attack() is true and need_success is true.
 *
 *          Set the angle to point directly at the selected opponent and see whether the opponent can be hit or not.
 *
 *          int32_t old_angle : (curr_angle) Backup the currently used angle to write it back if the aiming fails.
 *          int32_t old_power : (curr_power) Backup the currently used power to write it back if the aiming fails.
 *          double  drift     : (calculated) Variation of the angle to simulate that an exact aiming needs skill.
 *
 *       3.3.1 Set curr_power to the current tanks power, so no power change is necessary if this becomes the
 *             attack that is performed.
 *       3.3.2 Set curr_angle to an angle pointing directly at the selected target and add the value of 'drift'.
 *       3.3.3 Follow the beam using a mind shot and calculate a hit score.
 *       3.3.4 Use tank->shoot_clearance() to see whether the shot is blocked or crashes.
 *             If the shot does not reach the target, and is_last is true, the hit_score (might have hit someone else)
 *             is reduced. If this is not the very last attempt, curr_angle and curr_power are written back, need_aim is
 *             set to true and false is returned.
 *
 *     3.4  If the tank is buried, return the result of calc_unbury(bool is_last).
 *
 *            Param 1 : The value of is_last is simply transported.
 *
 *          Make sure that whatever is chosen is appropriate and points into the right direction.
 *
 *       3.4.1 If either a riot bomb is chosen, or a non-shaped weapon is selected while a self destruct attempt is
 *             planned (*), the number of enemies on each side is counted to chose where to fire at.
 *             After setting curr_angle and curr_power to appropriate values, they get sanitized, and angle is set to
 *             curr_angle, power is set to curr_power, need_aim is set to false as no further aiming is needed, and
 *             is_blocked is set to true as this is the situation.
 *             After that true is returned.
 *       3.4.2 If a weapon is chosen and no self destruction is wanted, the shaped weapons are the only appropriate
 *             weapons to free the trank without damaging itself. The current angle is set to 180°, the current power
 *             is set to 10 plus some random variation according to the AI level. The current values then get
 *             sanitized, and angle is set to curr_angle, power is set to curr_power, need_aim is set to false as no
 *             further aiming is needed, and is_blocked is set to true as this is the situation.
 *             After that true is returned.
 *       3.4.3 If this all fails but is_last is set to true, use_freeing_tool() is called for an emergency selection.
 *             As a last resort, the small missile is selected if use_freeing_tool() did not succeed. The current angle
 *             is set to a value between 100° and 160° degrees to either the left or right side, according to which
 *             side is heavier buried. The current power is set to a value between 500 and 1,000. The current values
 *             then get sanitized, and angle is set to curr_angle, power is set to curr_power, need_aim is set to false
 *             as no further aiming is needed, and is_blocked is set to true as this is the situation.
 *             After that true is returned.
 *       3.4.4 If everything failed, false is returned.
 *
 *     3.5  If the currently selected target is the bot itself, this is a self destruct attempt. Return the result of
 *          calc_kamikaze(bool is_last), then.
 *
 *            Param 1 : The value of is_last is simply transported.
 *
 *          As, at this point, a weapon is chosen, it must be checked whether it can be used and determined where to
 *          fire it.
 *
 *       3.5.1 If a horizontal shaped weapon is selected, a free flat spot of the same height as where the tanks stands
 *             on either the left or right side must be found.
 *             If such a spot can be found, curr_angle and curr_power are set to values trying to hit it.
 *       3.5.2 If a napalm weapon is chosen, curr_angle is set to 135° to the left or right side, whichever side has
 *             head wind. curr_power is set to 100 plus an amount calculated using the current wind strength.
 *       3.5.3 Otherwise fire the weapon upwards with more power the higher the spread value.
 *       3.5.4 If this worked out, sanitize the current angle and power, write them into to angle and power, and return
 *             true.
 *       3.5.5 If this didn't work out but this is the last attempt, go through all valid self destruct items and
 *             weapons until one is available or the small missile is selected. This is then just fired upwards and
 *             true is returned.
 *       3.5.6 In all other cases false is returned.
 *
 *     3.6  Otherwise this is normal aiming, and calc_standard(bool is_last, bool allow_flip_shot) is used to generated
 *          the initial values to begin with.
 *
 *            Param 1 : The value of is_last is simply transported.
 *            Param 2 : true, if the pre-incremented mem_curr->attempts is an even number, false otherwise.
 *                      If set to true, the bot is allowed to shoot in the opposite direction. On steel walls, this
 *                      parameter is ignored.
 *
 *          This method does no aiming but sets needed offsets and generates an angle and a power value to begin with.
 *
 *          has_flipped : This is set to true, if the bot decides to flip through a wrap wall or towards a bounce/rubber
 *                       wall. aim() then can check whether to flip back because the wall isn't even reached.
 *       3.6.1 calculate the offsets for the x and y coordinate needed by the chosen weapon. This is done with the
 *             method calc_offset(bool is_last).
 *
 *               Param 1 : The value of is_last is simply transported.
 *
 *             This method calculate x and y offsets for weapons that need it. These offsets are stored in the CAICore
 *             members offset_x and offset_y, as they are needed in multiple places.
 *
 *             If the needed offset is off the screen, or makes no sense, the method returns false. But if is_last is
 *             set to true, insane offsets are tried to be fixed. The idea is, that the bot tries nevertheless out of
 *             pure desperation.
 *
 *         3.6.1.1 Napalm weapons need a horizontal offset where the wind blows the jellies over the opponent for
 *                 greatest effect.
 *         3.6.1.2 Shaped charges and their bigger versions need a horizontal offset that leads to a hit at either side
 *                 of the opponent where the land height is equal enough to where the opponent stands, that the blast
 *                 actually hits.
 *         3.6.1.3 The driller must be placed above a buried tank, or under it if it is clinging to a steep hill side.
 *
 *       3.6.2 Calculate the starting angle, but limit it to:
 *             - 20° to 35° if the opponent is below the bot,
 *             - 40° to 55° if the opponent is at about the same height and
 *             - 60° to 75° if the opponent is above the bot.
 *             The angle is then modified according to the focus_rate of the bot.
 *       3.6.3 If a wrap wall is in place, check whether shooting through it results in a shorter shot, and flip the
 *             angle if it is.
 *       3.6.4 If allow_flip_shot is true and the wall is something else than a steel wall, bots may flip the shot with
 *             a chance of 50% for a useless bot and 80% for a deadly bot.
 *       3.6.5 Raise the angle until there is enough shooting clearance or the ceiling is hit.
 *       3.6.6 If the angle becomes too steep, revert to the half way between the beginning angle and the currently
 *             raised one. If this means an obstacle is in the way, the bot might decide to remove it first.
 *       3.6.7 Calculate starting power as a raw estimation using the simple distance.
 *       3.6.8 If the shot is already known to be blocked, write back the current angle and power to the used angle
 *             and power and set need_aim to false.
 *
 *     3.7  In boxed mode, the situation regarding the ceiling must be checked, but only if calc_standard() succeeded,
 *          the tank is not blocked and a weapon is chosen that needs aiming.
 *          This is done by calling calc_boxed(bool is_last).
 *
 *            Param 1 : The value of is_last is simply transported.
 *
 *       3.7.1 If is_last is false, the bot might "forget" to check for ceiling hits. The chance is between 33% for the
 *             useless bot and 7% for the deadly+1 bot.
 *       3.7.2 As long as the shot is regarded to be crashed, but the tracing was finished (not too many bounces/wraps)
 *             and either angle or power can be modified, trace_shot() is used to see where the shot would end with the
 *             current angle and power.
 *
 *         3.7.2.1 If the shot ends in a steel wall or ceiling, or if the shot hits the floor bottom through a wrap
 *                 ceiling but is not a digging weapon, the shot is regarded to be crashed.
 *         3.7.2.2 If the shot crashed, either the current angle, power or both are modified by one step. The angle is
 *                 changed to flatten the shot and the power is reduced.
 *         3.7.2.3 If the angle reaches 90°/270°, it can no longer be modified.
 *         3.7.2.4 If the power reaches MIN_POWER, it can no longer be modified.
 *
 *       3.7.3 If the shot has not crashed and the tracing was finished but the shot did not reach its target, the path
 *             is considered to be blocked. If is_last is true and the map has a steel or wrap wall and there was no
 *             positive setup score already, the path is tried to be cleared.
 *
 *         3.7.3.1 Decide whether to free the tank or to remove an obstacle.
 *         3.7.3.2 Use calc_unbury() if the tank is to be freed.
 *         3.7.3.3 Flatten the current angle if an obstacle is to be removed.
 *         3.7.3.4 Set need_aim to false and is_blocked to true.
 *         3.7.3.5 Directly return true.
 *
 *       3.7.4 If no emergency freeing is possible, is_last is false, or its the wrong wall type or a positive setup
 *             has already been found, directly return false.
 *       3.7.5 In all other cases return true if the last shot did not crash or if is_last is true, and false
 *             otherwise.
 *
 *     3.8  Eventually return the value of 'result'.
 *
 *   4  done = aim(bool is_last, bool can_move) - called if calc_attack() returned true, need_aim is true and is_blocked is false.
 *
 *        Param 1 : True if tgt_attempts equals findTgtAttempts and need_success is still true.
 *        Param 2 : True if opp_attempts is at least halve of findOppAttempts.
 *
 *      Here the aiming is done for the selected weapon against the selected target.
 *
 *      pl_stage        : (PS_AIM)              The AI enters the aiming stage.
 *      hill_detected  : (false)               Some situations indicate that a hill is between the tank and its target.
 *      best_score     : (NEUTRAL_ROUND_SCORE) Used to record the best setup for the current aiming round.
 *      best_angle     : (angle)               Used to record the angle that achieved the best aiming round score.
 *      best_power     : (power)               Used to record the power that achieved the best aiming round score.
 *      best_prime_hit : (false)               Used to record whether the best aiming round settings hit the primary
 *                                             target.
 *      best_overshoot : (MAX_OVERSHOO)        Used to record the overshoot the best setup had.
 *      last_ang_mod   : (0)                   Note down the used angle modifications, so the next aiming attempt knows
 *                                             the last modification to the angle.
 *      last_pow_mod   : (0)                   Note down the used power modifications, so the next aiming attempt knows
 *                                             the last modification to the power.
 *      last_overshoot : (MAX_OVERSHOOT)       Note down the achieved overshoot, so the next aiming attempt knows
 *                                             whether the shot gets nearer to the target or not.
 *      last_reverted  : (false)               Set to true if an aiming attempt reverts the previous modifications.
 *      last_score     : (0)                   Note down the achieved score, so the next aiming attempt knows whether
 *                                             the new hit is really better or not.
 *      last_was_better: (false)               Set to true if the previous score was better than the current one.
 *      reached_x      : (x)                   Used as a short cut and memory of where the current attempt hits.
 *      reached_y      : (y)                   Used as a short cut and memory of where the current attempt hits.
 *
 *      The aiming is done as long as the number of aiming attempts have not reached findRngAttempts.
 *
 *     4.1  Generate an angle modifier in the interval [1;7], with 2 being the maximum for the useless and 7 the
 *          maximum for the deadly+1 AI, and a power modifier in the interval [10;340] with 220 being the maximum for
 *          useless and 340 being the maximum for deadly bots.
 *
 *     4.2  Use void trace_weapon(int32_t &has_crashed, int32_t &has_finished)
 *          to see where the used weapon using curr_angle and curr_power will end.
 *
 *            Param 1: has_crashed is set to the number of projectiles that crashed into a steel wall or ceiling.
 *                     If the level has a wrap wall ceiling and the projectile can not dig and thus would explode on
 *                     the very bottom of the screen due to dirt being in the way, it is considered to have crashed,
 *                     too.
 *            Param 2: has_finished is set to the number of projectiles that have been traced to the end. The bots can
 *                     only trace max_bounce wall and ceiling bounces or wraps. If the number of actual bounces exceeds
 *                     this limit, the projectile is considered unfinished and the tracing stops.
 *
 *          Basically this method uses trace_shot() for each spread projectile of the weapon. Non-spread weapons have a
 *          spread value of 1, so this can be done for every weapon.
 *          Weapons with submunition are then traced further using trace_cluster(), which will, like trace_weapon() does
 *          on weapons without submunition, use calc_hit_damage() to generate the damage values on each tank.
 *
 *          The nearest hit to the primary target is recorded in curr_overshoot, reached_x and reached_y.
 *
 *     4.3  Use int32_t calc_hit_score(bool is_last) to generate a score out of all damage dealt.
 *
 *            Param 1: is_last is set to true if is_last in aim() is true and need_success is true.
 *
 *          This method cycles through the opponents memory, and sums up the damage done with curr_weap to a total
 *          score according to a) how much damage over the opponents health (aka overkill) has been done and b) on
 *          which team they are compared to us.
 *
 *          If the primary target was not hit, the score is ensured to be negative, as collateral damage is
 *          discouraged. However, this is only done if is_last is false, as collateral damage with a total positive
 *          score is better than nothing on the very last attempt.
 *
 *     4.4  If a new best_score is achieved, and either the primary target was hit, or hasn't been hit before, the
 *          best_* values are set to the current ones:
 *
 *            best_angle     = curr_angle
 *            best_overshoot = curr_overshoot
 *            best_power     = curr_power
 *            best_prime_hit = curr_prime_hit
 *            best_score     = hit_score
 *
 *     4.5 Otherwise, if it does not seem to be possible to find a valid setup within half of the attempts the AI has,
 *          try to move the tank a little according to the following rules:
 *
 *          - If the target is very near (under two bitmap widths) then move away.
 *          Otherwise:
 *          - If the angle is steep (75° and up), assume the shot must go over a hill and move away from the target.
 *          - If the angle is flat  (15° and down), move towards the target, the way seems clear at least.
 *          Otherwise:
 *          - If the overshoot is negative (too short), move towards the target.
 *          - If the overshoot is positive (too far), move away from the target.
 *
 *     4.6  Basically there are four different situations that need different actions.
 *          If the shot, or some of the spread or cluster shots, did not finish, it must be tried to change the used
 *          angle and power so the next attempt does finish.
 *          With steel walls or a wrapped ceiling ceiling with dirt on the ground the shot can have crashed. This can
 *          be fixed by getting away from 45° and reducing power.
 *          The hit might be nearer than the last one. In this case the last modifications seem to have moved the angle
 *          and power into the right direction, which is a path to follow further.
 *          And finally the hit might be farther away. The last modifications might have been too strong or in the
 *          wrong direction.
 *
 *       4.6.1 Try to fix unfinished shots using void fix_unfinished(int32_t &ang_mod, int32_t &pow_mod).
 *
 *               Param 1 : Reference to the angle modifier to adapt.
 *               Param 2 : Reference to the power modifier to adapt.
 *
 *             If the angle is too steep, lower it, if it is too flat, raise it. The angle is, however, limited
 *             according to where the opponent is. If it is above the own tank, the angle is limited around 60°. If
 *             it is at an equal height in the range of +/- 100 pixels, the angle is limited around 40°. If it is
 *             below, the angle is limited around 20°.
 *
 *             But if hill_detected is true, the angle modification is limited to 1° per aiming round.
 *
 *             The power is reduced if it is greater than twice the x distance to the opponen, and raised if it is less
 *             then the x distance. If it is between the two, power is not changed.
 *
 *             last_reverted   : true if last_ang_mod is signed differently than the resulting ang_mod.
 *             last_was_better : false
 *
 *       4.6.2 Try to fix crashed shots using void fix_crashed(int32_t &ang_mod, int32_t &pow_mod).
 *
 *               Param 1 : Reference to the angle modifier to adapt.
 *               Param 2 : Reference to the power modifier to adapt.
 *
 *             Here the angle modifier is limited to 1° per aiming round if hill_detected is false. this is done,
 *             because if not trying to get over hill the crashes normally occur if the shot is just a bit too strong.
 *
 *             If the angle is above 60° in boxed mode, it is reduced by a random amount between [1;7]° according
 *             to ai_level. In non-boxed mode it is reduced by 1° if it is between 10° and 45°. Otherwise the angle is
 *             left alone.
 *
 *             If the power is greater than the simple x distance, it is reduced as well, even more if it is 50% and
 *             more greater than the x distance.
 *
 *             Now if the hit_score is positive, halve it for every shot that crashed. The hit_score has of course
 *             been written into best_score already if it was better, but the modified version will be used for
 *             last_score later.
 *
 *             last_reverted   : true if last_ang_mod is signed differently than the resulting ang_mod.
 *             last_was_better : false
 *
 *       4.6.3 If the shot did finish and did not crash and hit nearer to the target than the last, a few adaptations
 *             might be needed.
 *
 *             If the resulting hit_score is worse than last_score, it must be ensured, that ang_mod has the same sign
 *             as the previous one. The direction is correct, as the hit is nearer, but it hasn't been changed enough.
 *
 *             To get even nearer, pow_mod is ensured to have the opposite sign of curr_overshoot. So if curr_overshoot
 *             is negative, the shot is still too short and pw_mod must be positive and vice versa.
 *
 *             last_reverted   : true if last_ang_mod is signed differently than the resulting ang_mod.
 *             last_was_better : true if hit_score is lower than last_score, false otherwise.
 *
 *       4.6.4 If the shot did finish and did not crash but hit farther away than the last, use
 *             void fix_overshoot(int32_t& ang_mod, int32_t& pow_mod, int32_t hit_score).
 *
 *               Param 1 : Reference to the angle modifier to adapt.
 *               Param 2 : Reference to the power modifier to adapt.
 *               Param 3 : The hit_score achieved with the shot. This is local to CAICore::aim() and must be submitted.
 *
 *             bool angle_was_optimized : true  if the last modification brought the angle nearer to 45° on its side,
 *                                              and false otherwise.
 *             hill_detected            : false if both the overshoot and the hit_score are positive. Otherwise the
 *                                              current value is not changed.
 *
 *             Here are some more possible (sub) situations to consider:
 *             1) The current score is at least better than the last.
 *                This can happen if the shot does no longer hit team mates.
 *                The important situation is, if the overshoot is very small and a new best score is achieved.
 *                The bigger the weapon, the higher the probability that this might be the case.
 *
 *                If the hit_score is lower than best_score, meaning no new best score was achieved, it is ensured
 *                that both ang_mod and pow_mod have the same sign than the last modifications had. Without a new
 *                best_score the higher one might only mean less collateral damage, and no adaptation is done.
 *
 *                last_was_better : true
 *                hill_detected   : false
 *
 *             2) Both the current and the last overshoot were negative, the angle was optimized towards 45° and the
 *                power was raised.
 *                Having a worse overshoot then can happen if the gun was lowered and the shot crashes into the side of
 *                a hill or mountain.
 *                The angle must then be brought towards 180° more than the last angle modification brought it away
 *                from it.
 *
 *                last_was_better : false, no matter what, so this change won't get directly reverted again.
 *                hill_detected   : true;
 *
 *             3) The current score is worse than the last score.
 *                 a) The last score was better than the one before.
 *                    The modifications might have been too strong, try values between the two.
 *                 b) That was two worse tries in a row.
 *                    The direction was wrong, and the last modifications must be reverted and strengthened by the
 *                    current set modifications.
 *
 *                last_was_better : false
 *
 *             4) No last score or the same.
 *                Just adapt the mods according to whether the shot was too short or too long.
 *
 *                last_was_better : false
 *
 *             last_reverted   : true if last_ang_mod is signed differently than the resulting ang_mod.
 *
 *     4.7  If the current angle is 180°, so pointing straight up, ang_mod is zero and no hit_score greater than zero
 *          was achieved, a random ang_mod in the interval [2;6] towards the opponent is generated to fix this vertical
 *          shot. Vertical trick shots using wind are only accepted if the resulting hit_score is positive.
 *
 *     4.8  If the power modification according to the current overshoot and ang_mod is too low, it is strengthened
 *          using the difference of the overshoot and pow_mod divided by ang_mod and multiplied by the AI's focus_rate.
 *
 *     4.9  Sanitize ang_mod and pow_mod, both applied must not lead to invalid values. Then apply both to curr_angle
 *          and curr_power.
 *
 *     4.10 Save the current values in the last_* members:
 *
 *            last_ang_mod   = ang_mod
 *            last_overshoot = curr_overshoot
 *            last_pow_mod   = pow_mod
 *            last_score     = hit_score
 *
 *          After this, the loop ends and the work flow restarts at 4.1.
 *
 *     4.11 When all aiming attempts are used up, an emergency plan to free the tank or unblock its path might be
 *          triggered if all of the following conditions are true:
 *
 *          - is_last and need_success are both true,
 *          - there was no best setup with a positive score, yet,
 *          - the current best round score will not create a new best setup with a positive score,
 *          - the overall best score is negative,
 *          - the best achieved overshoot is negative, indicating that the target was not yet reached and
 *          - either the overshoot is greater than the weapons radius without being a ceiling crash, or fix_overshoot()
 *            detected a hill in the path.
 *
 *     4.12 Eventually, if a new best_round_score is achieved, remember the current settings:
 *
 *            best_round_score = best_score;
 *            curr_angle       = best_angle;
 *            curr_power       = best_power;
 *
 *     4.13 Return true if either best_round_score is larger than zero, or both is_last and need_success are true.
 *
 *   5  If the aiming was successful, a few more checks are made.
 *
 *     5.1  If best_round_score is greater than zero, and either more weapons have been tried than the ai_level is or
 *          all findOppAttempts have been used up, both the weap_attempts and opp_attempts are reset to zero so this
 *          targeting attempt is declared to be over.
 *
 *     5.2  If the primary target was hit, its score is added to best_round_score. The opponents score is divided by
 *          ten times the ai_level for this unless it is the revengee, in which case the opponent score is only divided
 *          by the simple ai_level. This is done to enhance scores for attacks where the primary target was hit over
 *          attempts, were only collateral damage was done, but counted positive due to last attempt behaviour.
 *          Further this makes shots against revengees more likely, even if they are imperfect, the lower the AI level
 *          is.
 *
 *     5.3  If a weapon with a single damage value over zero, so no dirt bomb or reducer, was chosen, its score divided
 *          by ten times the ai_level is added, too.
 *
 *     5.4  If a new best setup score was achieved, or the primary target was first hit, or a success must be enforced,
 *          the best setup data is remembered:
 *
 *          best_setup_angle     = curr_angle
 *          best_setup_item      = item_curr
 *          best_setup_mem       = mem_curr
 *          best_setup_overshoot = best_overshoot
 *          best_setup_power     = curr_power
 *          best_setup_prime     = best_prime_hit
 *          best_setup_weap      = weap_curr
 *          best_setup_score     = best_round_score
 *
 *          As this targeting round is then definitely over, opp_attempts and weap_attempts are reset to zero to
 *          trigger a fresh new round.
 *
 *      The main targeting loop ends here.
 *
 *   6  If a new revengee was set, or the old removed, save this in player->revenge.
 *
 *   7  Without a real setup with a positive score here, a last freeing attempt might be triggered. But only if the
 *      AI did not detect that it was blocked already, or a freeing attempt would have been setup already.
 *
 *   8  Write back the best attack setup for the tank to use.
 *
 *   9  Shout a retaliation phrase out if the revengee is attacked and the predicted damage is at least 50% of the
 *      opponents total health.
 *
 *  10  Otherwise, if a self destruct attempt is issued, shout out a good-bye-phrase.
 *
 *  11  If a different target than in the last round is attacked, add some "last-second-errors".
 **/
class CAICore {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Create the AI core.
	explicit CAICore();
	/// Destroy the AI core.
	~CAICore();

	// No copying, no assignment
	CAICore( CAICore const& )             = delete;
	CAICore& operator= ( CAICore const& ) = delete;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	// Getters
	[[nodiscard]] CPlayer* active_player() const; ///< Planned player.
	[[nodiscard]] bool    can_work() const;      ///< Planning may proceed.
	[[nodiscard]] bool    has_exited() const;     ///< Planning thread finished.

	// Setters
	/// Enable AI speech.
	void allow_text();
	/// Disable AI speech.
	void forbid_text();
	/// Report the movement result.
	void has_moved( int32_t direction );
	/// Start planning for a player.
	bool start( CPlayer* player_ );
	/// Fetch the attack setup.
	bool status( int32_t& a_item, int32_t& a_angle, int32_t& a_power, EPlayerStages& pl_stage );
	/// Stop planning.
	void stop();
	/// Note the shot was taken.
	void weapon_fired();

	// Operators
	/// Run background planning.
	void operator() ();


	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

private:
	typedef EPlayerStages               plstage_t;
	typedef sItemListEntry              itentry_t;
	typedef sOppMemEntry                opentry_t;
	typedef sWeapListEntry              weentry_t;
	typedef std::mutex                  mutex_t;
	typedef std::condition_variable     condv_t;
	typedef std::lock_guard< mutex_t >  lguard_t;
	typedef std::unique_lock< mutex_t > luniq_t;


	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	bool    aim( int32_t combo_attempt, int32_t combo_tries, bool can_move );
	bool    calc_attack( int32_t attempt, int32_t tries );
	bool    calc_boxed( bool is_last );
	void    calc_hit_damage( int32_t hit_x, int32_t hit_y, double weap_rad, double dmg, EWeaponType weap_type );
	int32_t calc_hit_score( bool is_last );
	bool    calc_kamikaze( bool is_last );
	bool    calc_laser( bool is_last );
	bool    calc_offset( bool is_last );
	bool    calc_standard( bool is_last, bool allow_flip_shot );
	bool    calc_unbury( bool is_last );
	void    check_item_mem();
	void    check_opp_mem();
	void    check_weap_mem();
	void    destroy();
	void    flatten_curr_ang();
	void    fix_crashed( int32_t& ang_mod, int32_t& pow_mod );
	void    fix_overshoot( int32_t& ang_mod, int32_t& pow_mod, int32_t hit_score );
	void    fix_unfinished( int32_t& ang_mod, int32_t& pow_mod );
	bool    get_memory();
	bool    initialize();
	bool move_tank();
	void    sanitize_curr();
	bool    select_item( bool is_last );
	bool    select_target( bool is_last );
	bool    setup_attack( bool is_last, int32_t& opp_attempt, int32_t& weap_attempt );
	void    show_feedback( char const* feedback, int32_t col, double yv, ETextSway text_sway, int32_t dur );
	void    trace_cluster( int32_t sub_type, int32_t sub_count, int32_t sub_x, int32_t sub_y, double inh_xv, double inh_yv );
	bool    trace_shot(
		   int32_t  trace_angle,
		   int32_t  delay_idx,
		   bool&    finished,
		   bool&    top_wrapped,
		   int32_t& reached_x_,
		   int32_t& reached_y_,
		   double&  end_xv,
		   double&  end_yv
	   );
	void trace_weapon( int32_t& has_crashed, int32_t& has_finished );
	void update_item_score( itentry_t* pItem );
	void update_opp_score( opentry_t* pOpp );
	void update_weap_score( weentry_t* pWeap );
	bool use_freeing_tool( bool free_tank, bool is_last );
	bool use_item( EItemType item_type );
	bool use_item( int32_t item_index );
	bool use_weapon( EWeaponType weap_type );
	bool use_weapon( int32_t weap_index );


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	// Internal values
	mutex_t    action_mutex;
	condv_t    action_condition;
	EPlayerType best_type      = USELESS_PLAYER; // What the AI considers humans to be.
	abool_t    can_move       = ATOMIC_VAR_INIT( true );
	bool volatile allow_work    = true;
	int32_t curr_angle       = 90;    //!< The angle that is currently tested
	int32_t curr_overshoot   = 0;     //!< Current calculated distance of hit versus opponent
	int32_t curr_power       = 0;     //!< The power that is currently tested
	bool    curr_prime_hit   = false; //!< Whether the primary target was hit.
	double  error_multiplier  = 0.;    //!< Default error reduction according to AI level
	int32_t findOppAttempts  = 0;     //!< Number of attempts to select a suitable opponent
	int32_t findRngAttempts  = 0;     //!< Number of attempts to aim the current selection
	int32_t findTgtAttempts  = 0;     //!< Number of attempts to come up with an attack plan
	int32_t findWeapAttempts = 0;     //!< Number of attempts to find a suitable item/weapon
	double  focus_rate        = 0.;    //!< How good a bot can focus on a specific task
	bool    is_blocked        = false; //!< Set to true if a shot can't get through
	bool volatile is_finished = false; //!< Set to true when operator() ends
	ai32_t is_moved_by         = ATOMIC_VAR_INIT( 0 );
	bool   is_shocked         = false;
	bool volatile is_stopped  = false;
	bool volatile is_working  = false;
	int32_t    max_bounce     = 0;     //!< How many wall bounces/wraps can be calculated
	bool       need_aim       = false; //!< true if this is a standard shot
	bool       need_success   = true;  //!< true unless a best score is achieved
	int32_t    offset_x      = 0;
	int32_t    offset_y      = 0;
	plstage_t  pl_stage       = PS_AI_IS_IDLE;
	sOpponent* revengee      = nullptr;                 //!< If set, it is tried first as a target
	sOpponent* shocker       = nullptr;                 //!< The current fear shock winner
	abool_t    text_allowed   = ATOMIC_VAR_INIT( true ); //!< Is new CFloatText allowed?
	int32_t    weap_idx      = SML_MIS;

	// Values taken from the player and their tank
	int32_t    ai_level    = 0;  //!< To not having to cast from player type.
	double     ai_level_d  = 0.; //!< To not having to cast from ai_level.
	double     ai_over_mod = 0.; //!< modifier for overkills and similar
	double     ai_type_mod = 0.; //!< modifier for important decisions
	int32_t    angle       = 90; //!< The currently determined best angle
	double     blast_min   = 0.; //!< Damage done by small missile
	double     blast_med   = 0.; //!< Damage done by medium or large missile
	double     blast_big   = 0.; //!< Damage done by small nuke or nuke
	double     blast_max   = 0.; //!< Damage done by death head
	int32_t    buried      = 0;  //!< Full buried level
	int32_t    buried_l    = 0;  //!< left side buried level
	int32_t    buried_r    = 0;  //!< right side buried level
	double     curr_life    = 0.;
	bool       has_flipped  = false;   //!< Used by calc_standard() and aim() to check for flipping errors.
	itentry_t* item_curr   = nullptr; //!< Currently selected entry
	itentry_t* item_head   = nullptr; //!< Last selected entry
	itentry_t* item_last   = nullptr; //!< Entry with highest score
	int32_t    last_ang    = 0;       //!< Angle used in last round
	sOpponent* last_opp    = nullptr; //!< The opponent attacked in the last round
	int32_t    last_pow    = 0;       //!< Power used in last round
	int32_t    last_weap   = 0;       //!< weapon used in the last round
	int32_t    max_life     = 100;
	opentry_t* mem_curr    = nullptr; //!< Currently selected entry
	opentry_t* mem_head    = nullptr; //!< Last selected entry
	opentry_t* mem_last    = nullptr; //!< Entry with highest score
	bool       need_money   = false;   //!< Might alter some decisions
	CPlayer*    player      = nullptr;
	int32_t    power       = 0; //!< The currently determined best power
	CTank*      tank        = nullptr;
	weentry_t* weap_curr   = nullptr; //!< Currently selected entry
	weentry_t* weap_head   = nullptr; //!< Last selected entry
	weentry_t* weap_last   = nullptr; //!< Entry with highest score
	double     x           = 0.;
	double     y           = 0.;

	// Values to remember settings and situations over aiming rounds
	int32_t best_angle      = 0;
	int32_t best_overshoot  = MAX_OVERSHOOT; //!< Overshoot value of currently best angle and power
	int32_t best_power      = 0;
	bool    best_prime_hit  = false; //!< Whether the bes aiming round values hit the primary target.
	int32_t best_score      = NEUTRAL_ROUND_SCORE;
	bool    hill_detected   = false;
	int32_t last_ang_mod    = 0;
	int32_t last_overshoot  = MAX_OVERSHOOT;
	int32_t last_pow_mod    = 0;
	bool    last_reverted   = false;
	int32_t last_score      = 0;
	bool    last_was_better = false;
	int32_t reached_x       = 0;
	int32_t reached_y       = 0;

	// Values used to memorize the best setup in a round
	int32_t    best_round_score     = NEUTRAL_ROUND_SCORE;
	int32_t    best_setup_angle     = 0;
	int32_t    best_setup_damage    = 0;
	itentry_t* best_setup_item      = nullptr;
	opentry_t* best_setup_mem       = nullptr;
	int32_t    best_setup_overshoot = MAX_OVERSHOOT;
	int32_t    best_setup_power     = 0;
	bool       best_setup_prime     = false;
	int32_t    best_setup_score     = 0;
	weentry_t* best_setup_weap      = nullptr;
};

#endif // ATANKS_AICORE_H_INCLUDED
