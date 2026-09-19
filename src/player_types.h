#ifndef ATANKS_PLAYER_TYPES_H_INCLUDED
#define ATANKS_PLAYER_TYPES_H_INCLUDED 1

/** @file player_types.h
 * @brief used enums plus operators for players and tanks
 **/

#include "main.h"

/** @enum ePlayerStages
 * @brief AI planning stages.
 **/
enum ePlayerStages {
	PS_AI_IS_IDLE = 0, //!< AI has nothing to do and is free to get work
	PS_AI_INITIALIZE,  //!< AI is initializing its data
	PS_SELECT_TARGET,  //!< AI is selecting a target
	PS_SELECT_WEAPON,  //!< AI is selecting a weapon or item
	PS_CALCULATE,      //!< AI calculates its basic attack values
	PS_AIM,            //!< AI aims the current selection to hit its target
	PS_MOVE_LEFT,      //!< AI wants to move their tank to the left
	PS_MOVE_RIGHT,     //!< AI wants to move their tank to the right
	PS_FIRE,           //!< AI is ready to have the current weapon/item fired
	PS_CLEANUP,        //!< AI is cleaning up
	PS_STAGE_COUNT
};

/// Shift the stage by a signed offset.
ePlayerStages &operator+= ( ePlayerStages &src, int32_t val );
/// Shift the stage by a negated signed offset.
ePlayerStages &operator-= ( ePlayerStages &src, int32_t val );
/// Advance to the next stage.
ePlayerStages &operator++ ( ePlayerStages &src );
/// Advance to the next stage (postfix form).
ePlayerStages operator++ ( ePlayerStages &src, int32_t ); // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]

/** @enum playerType
 * @brief Player skill levels and special types.
 **/
enum playerType {
	HUMAN_PLAYER = 0,
	USELESS_PLAYER,
	GUESSER_PLAYER,
	RANGEFINDER_PLAYER,
	TARGETTER_PLAYER,
	DEADLY_PLAYER,
	LAST_PLAYER_TYPE,
	PART_TIME_BOT,      // normally a human, but acting as a deadly computer
	VERY_PART_TIME_BOT, // just fires one shot
	NETWORK_CLIENT,
	SDI_PREDICTOR // Used so missile mind shots from the SDI won't
	              // trigger another SDI check, trigger another SDI
	              // check, trigger another...
};


/// @brief Maximum AI Level is the highest level being lucky, thus +1.
int32_t const maxAiLevel = DEADLY_PLAYER + 1;


/// Shift the type by a signed offset.
playerType &operator+= ( playerType &src, int32_t val );
/// Shift the type by a negated signed offset.
playerType &operator-= ( playerType &src, int32_t val );
/// Advance to the next type.
playerType &operator++ ( playerType &src );
/// Advance to the next type (postfix form).
playerType operator++ ( playerType &src, int32_t ); // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]

/** @enum playerPrefType
 * @brief Weapon preference lifetime.
 **/
enum playerPrefType { PERPLAY_PREF = 0, ALWAYS_PREF, PREF_COUNT };

/// Shift the preference scope by a signed offset.
playerPrefType &operator+= ( playerPrefType &src, int32_t val );
/// Shift the preference scope by a negated signed offset.
playerPrefType &operator-= ( playerPrefType &src, int32_t val );
/// Advance to the next preference scope.
playerPrefType &operator++ ( playerPrefType &src );
/// Advance to the next preference scope (postfix form).
playerPrefType operator++ ( playerPrefType &src, int32_t ); // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]

/** @enum ePlayerEdit
 * @brief return codes used by the sub menus when editing players
 **/
enum ePlayerEdit {
	PE_BACK         = 1,        //!< User opted out. No new player, no edit and no deletion.
	PE_CONFIRM_NEW  = 0x010000, //!< Adding a new player was confirmed
	PE_CONFIRM_EDIT = 0x020000, //!< Changes to a player have been confirmed
	PE_CONFIRM_DEL  = 0x040000  //!< Deleting a player was confirmed
	                            // Note: The values allow to use the last 16 bit for key code bit masks.
};

/** @enum eTeamTypes
 * @brief determines the team a player belongs to
 **/
enum eTeamTypes { TEAM_SITH = 0, TEAM_NEUTRAL, TEAM_JEDI, TEAM_COUNT };

/// Shift the team by a signed offset.
eTeamTypes &operator+= ( eTeamTypes &src, int32_t val );
/// Shift the team by a negated signed offset.
eTeamTypes &operator-= ( eTeamTypes &src, int32_t val );
/// Advance to the next team.
eTeamTypes &operator++ ( eTeamTypes &src );
/// Advance to the next team (postfix form).
eTeamTypes operator++ ( eTeamTypes &src, int32_t ); // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]

/** @enum eTankOffsets
 * @brief Centrally store the bitmap offsets of the tank images
 **/
enum eTankOffsets { TO_TURRET = 0, TO_TANK = 7 };

/** @enum eTankTypes
 * @brief the tanks currently known, TT_TANK_COUNT is the number of tanks
 **/
enum eTankTypes {
	TT_NORMAL = 0,
	TT_CLASSIC,
	TT_BIGGREY,
	TT_T34,
	TT_HEAVY,
	TT_FUTURE,
	TT_UFO,
	TT_SPIDER,
	TT_BIGFOOT,
	TT_MINI,
	TT_TANK_COUNT
};

/// Shift the tank type by a signed offset.
eTankTypes &operator+= ( eTankTypes &src, int32_t val );
/// Shift the tank type by a negated signed offset.
eTankTypes &operator-= ( eTankTypes &src, int32_t val );
/// Advance to the next tank type.
eTankTypes &operator++ ( eTankTypes &src );
/// Advance to the next tank type (postfix form).
eTankTypes operator++ ( eTankTypes &src, int32_t ); // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]


#endif // ATANKS_PLAYER_TYPES_H_INCLUDED
