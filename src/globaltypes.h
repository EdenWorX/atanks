#ifndef ATANKS_GLOBALTYPES_H_INCLUDED
#define ATANKS_GLOBALTYPES_H_INCLUDED 1

/*
 * atanks - obliterate each other with oversize weapons
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

#define DEFAULT_SCREEN_WIDTH  800    ///< Default window width in pixels.
#define DEFAULT_SCREEN_HEIGHT 600    ///< Default window height in pixels.
#define GAMENAMELEN           64     ///< Maximum game name length.
#define MAX_INTEREST_AMOUNT   100000 ///< Maximum bank interest payout.
#define MAX_TEAM_AMOUNT       500000 ///< Maximum team fee.
#define DEMO_WAIT_TIME        60     ///< Idle seconds before demo mode starts.
#ifdef ATANKS_IS_WINDOWS
#  define MAX_AI_TIME 30 ///< DirectDraw is too slow, allow more time
#else
#  define MAX_AI_TIME 10 ///< Standard with anything but windows
#endif                   // ATANKS_IS_WINDOWS

// Start enforcing unified integer typing
#include <cstdint>

// Use atomic types for thread safety where locks are a bad idea
#include <atomic>
typedef std::atomic_bool    abool_t; ///< Atomic boolean for lock-free thread safety.
typedef std::atomic_flag    aflag_t; ///< Atomic flag for lock-free thread safety.
typedef std::atomic_int32_t ai32_t;  ///< Atomic 32-bit integer for lock-free thread safety.

/** @file globaltypes.h
 * @brief Definitions of types relevant to global data.
 **/


/** @enum EBackgroundTypes
 * @brief types for drawing menu background
 **/
enum EBackgroundTypes {
	BACKGROUND_BLANK = 0,
	BACKGROUND_CIRCLE,
	BACKGROUND_LINE,
	BACKGROUND_SQUARE,
	BACKGROUND_COUNT,
};

/** @enum EBoxModes
 * @brief Whether boxed mode is on, off or random.
 **/
enum EBoxModes { BM_OFF = 0, BM_ON, BM_RANDOM };

/** @enum EColourTheme
 * @brief determine which colour theme to use
 **/
enum EColourTheme { CT_REGULAR = 0, CT_CRISPY };

/** @enum EControl
 * @brief control results for human and computer control
 **/
enum EControl {
	CONTROL_NONE  = 0,
	CONTROL_FIRE  = 101, // Explicitly fire a weapon/item
	CONTROL_OTHER = 102, // Something else but firing something
	CONTROL_SKIP  = 201, // Turn on skipping computer play through in game menu
	CONTROL_QUIT  = 202  // Any means to quit the game
};

/** @enum EDataStage
 * @brief The data stage of the weapons text file loading
 **/
enum EDataStage { DS_NAME = 0, DS_DESC, DS_DATA };

/// Advance to the next weapons-text loading stage.
EDataStage &operator++ ( EDataStage &ds );

/** @enum EFileStage
 * @brief The file stage of the weapons text file loading
 **/
enum EFileStage { FS_WEAPONS = 0, FS_NATURALS, FS_ITEMS };

/** @enum EFullScreen
 * @brief Whether to use full screen or not.
 **/
enum EFullScreen { FULL_SCREEN_EITHER = 0, FULL_SCREEN_TRUE, FULL_SCREEN_FALSE };

/** @enum ELandscapeTypes
 * @brief determine the types the landscape can have
 **/
enum ELandscapeTypes {
	LAND_RANDOM = 0,
	LAND_CANYONS,
	LAND_MOUNTAINS,
	LAND_VALLEYS,
	LAND_HILLS,
	LAND_FOOTHILLS,
	LAND_PLAIN,
	LAND_NONE
};

/** @enum ELandSlideTypes
 * @brief determine the kind of land sliding.
 **/
enum ELandSlideTypes {
	SLIDE_NONE = 0,  // gravity does not exist
	SLIDE_TANK_ONLY, // dirt falls, tank does not
	SLIDE_INSTANT,   // dirt falls without you seeing it
	SLIDE_GRAVITY,   // normal
	SLIDE_CARTOON    // gravity is delayed
};

/** @enum ELanguages
 * @brief Declare the list of supported languages.
 *
 * The last item EL_LANGUAGE_COUNT can be used to retrieve the
 * number of supported languages.
 *
 * This enum is sorted in the order the languages should be listed.
 **/
enum ELanguages {
	EL_ENGLISH = 0,
	EL_PORTUGUESE,
	EL_FRENCH,
	EL_GERMAN,
	EL_SLOVAK,
	EL_RUSSIAN,
	EL_SPANISH,
	EL_ITALIAN,
	EL_LANGUAGE_COUNT
};

// Helper operators to rotate languages:
/// Rotate to the next language.
ELanguages &operator++ ( ELanguages &lang );
/// Rotate to the next language (postfix form).
ELanguages operator++ ( ELanguages &lang, int ); // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]
/// Rotate to the previous language.
ELanguages &operator-- ( ELanguages &lang );
/// Rotate to the previous language (postfix form).
ELanguages operator-- ( ELanguages &lang, int ); // NOLINT(cert-dcl21-cpp)
/// Shift the language by a signed offset.
ELanguages &operator+= ( ELanguages &lang, int32_t val );
/// Shift the language by a negated signed offset.
ELanguages &operator-= ( ELanguages &lang, int32_t val );

/** @enum ERoundStages
 * @brief General stages for the game flow.
 **/
enum ERoundStages {
	STAGE_AIM = 0,
	STAGE_FIRE,
	STAGE_SCOREBOARD, // The scoreboard is displayed
	STAGE_ENDGAME     // All actions have ceased, the round has ended.
};

/** @enum ESatelliteLaser
 * @brief Size of satellite laser
 **/
enum ESatelliteLaser { SL_NONE = 0, SL_WEAK, SL_STRONG, SL_SUPER };

/** @enum ESaveGameStage
 * @brief Stages written in a saved game file
 **/
enum ESaveGameStage { SGS_NONE = 0, SGS_GLOBAL, SGS_ENVIRONMENT, SGS_PLAYERS, SGS_VERSION };

/** @enum ESkipPlayType
 * @brief How skipping computer play is managed
 **/
enum ESkipPlayType { SKIP_NONE = 0, SKIP_HUMANS_DEAD };

/** @enum ESoundDriver
 * @brief determine which sound driver to use
 **/
enum ESoundDriver {
	SD_AUTODETECT = 0,
	SD_OSS,
	SD_ESD,  // Does anybody still use that?
	SD_ARTS, // Long long deprecated
	SD_ALSA,
	SD_JACK
	// What about PulseAudio?
};

/** @enum ESounds
 * @brief enum that describes the sound array
 **/
enum ESounds {
	// === FIRE a weapon / an item ===
	SND_FIRE_MISS_SML = 0,
	SND_FIRE_MISS_MED = 1,
	SND_FIRE_MISS_LRG = 2,
	SND_FIRE_NUKE     = 3,
	SND_FIRE_DEATHEAD = 4,
	SND_FIRE_LASER    = 5,
	SND_FIRE_TELEPORT = 6,
	SND_FIRE_WIND_FAN = 7,

	// === EXPLosion of a weapon / an item ===
	SND_EXPL_MISS_SML       = 10,
	SND_EXPL_MISS_MED       = 11,
	SND_EXPL_MISS_LRG       = 12,
	SND_EXPL_NUKE           = 13,
	SND_EXPL_DEATHEAD       = 14,
	SND_EXPL_DIRT_BALL_BOMB = 15,
	SND_EXPL_SHAPED_CHARGE  = 16,
	SND_EXPL_WIDE_BOY       = 17,
	SND_EXPL_CUTTER         = 18,
	SND_EXPL_NAPALM         = 19,
	SND_EXPL_NAPALM_BURN    = 20,
	SND_EXPL_PER_CENT_BOMB  = 21,
	SND_EXPL_REDUCER        = 22,

	// === NATUral going off ===
	SND_NATU_THUNDER_SMLMED = 30,
	SND_NATU_THUNDER_LRG    = 31,
	SND_NATU_DIRT_FALL      = 32,

	// === INTErface sounds ===
	SND_INTE_BUTTON_CLICK = 40,

	// Play BackGround MUSIC
	SND_BG_MUSIC = 50,
	SND_COUNT

};

// turns
/** @enum ETurnTypes
 * @brief Turn order modes.
 **/
enum ETurnTypes { TURN_HIGH = 0, TURN_LOW, TURN_RANDOM, TURN_SIMUL };

/** @enum EViolentDeath
 * @brief Level of automatic violent death option.
 **/
enum EViolentDeath { VD_OFF = 0, VD_LIGHT, VD_MEDIUM, VD_HEAVY };

/** @enum EWallTypes
 * @brief All the types the walls can have.
 **/
enum EWallTypes { WALL_RUBBER = 0, WALL_STEEL, WALL_SPRING, WALL_WRAP, WALL_RANDOM };

/** @enum EWinner
 * @brief All possible winning sets
 **/
enum EWinner { WINNER_NO_WIN = 101, WINNER_DRAW = 102, WINNER_JEDI = 104, WINNER_SITH = 105 };


#endif // ATANKS_GLOBALTYPES_H_INCLUDED
