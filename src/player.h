#ifndef ATANKS_PLAYER_H_INCLUDED
#define ATANKS_PLAYER_H_INCLUDED 1

// Build configuration (CMake only): the NETWORK macro lives in the generated
// config.h, which must be visible before the first #ifdef NETWORK below
// regardless of include order. Non-CMake builds keep passing -DNETWORK= on
// the compiler command line, so this include stays conditional.
#ifdef ATANKS_HAVE_CONFIG_H
#  include "config.h"
#endif

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


#include "globaltypes.h"
#include "player_types.h"

#define MAX_WEAP_PROBABILITY 10000
#define BURIED_LEVEL         135
#define BURIED_LEVEL_HALF    68

class CTank;
class CPlayer;
class CAICore;

/// @brief minimal struct to allow AI players to keep track of friend and foe.
struct sOpponent {
	int32_t damage_from = 0;       //!< How much damage the opponent did to the player.
	int32_t damage_last = 0;       //!< How much damage the opponent did in this turn.
	int32_t damage_to   = 0;       //!< How much damage the player did to the opponent.
	double  fear        = 0.;      //!< How likely evasive manoeuvres are started.
	double  fear_shock  = 0.;      //!< The highest shock value determines the shocker.
	int32_t index       = -1;      //!< Needed for saving/loading to work.
	int32_t killed_me   = 0;       //!< How many times this opponent has killed this player.
	int32_t killed_them = 0;       //!< How many times this opponent was killed by this player.
	CPlayer* opponent    = nullptr; //!< The CPlayer memorized here.
	int32_t revenge_dmg = 0;       //!< Summed up damage to determine when it is time for revenge.
};

/** @class CPlayer
 * @brief All data concerning human and A players
 **/
class CPlayer {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Construct a player.
	explicit CPlayer();
	/// Destroy a player.
	~CPlayer();

	// no copying, no assignments
	CPlayer( const CPlayer& )             = delete;
	CPlayer& operator= ( const CPlayer& ) = delete;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void     check_opp_mem();                                           ///< Prune opponent memory.
	int32_t  choose_item_to_buy( int32_t max_boost, int32_t& last_idx ); ///< Choose the shop item to buy.
	EControl control_tank( CAICore* aicore, bool allow_fire );          ///< Run the turn control.
	void     draw_indicator( int32_t x, int32_t y, int32_t h ) const;  ///< Draw the turn indicator.
#ifdef NETWORK
	EControl execute_net_cmd( bool my_turn, CAICore* aicore );
#endif // NETWORK
	void       exit_shop();                                                         ///< Leave the shop screen.
	void       generate_preferences();                                              ///< Build AI personality.
	int32_t    get_boost_value();                                                    ///< Compute shop boost value.
	int32_t    get_item_pref( int32_t idx );                                         ///< Read item preference.
	int32_t    get_money_to_save( bool first_look );                                  ///< Compute round savings.
	bool       get_net_cmd();                                                        ///< Read the network command.
	sOpponent* get_opp_mem( int32_t idx );                                           ///< Fetch opponent memory.
	int32_t    get_weap_pref( int32_t idx );                                         ///< Read weapon preference.
	void       initialise( bool loaded_game );                                     ///< Initialize for a round.
	bool       load_from_file( FILE* file );                                       ///< Read settings.
	void       load_game_data( FILE* file, int32_t file_version );                 ///< Read savegame data.
	void       new_game();                                                          ///< Reset for a new game.
	void       new_round();                                                            ///< Reset for a new round.
	void       note_damage_from( CPlayer* opponent, int32_t damage, bool destroyed ); ///< Record received damage.
	void       note_damage_to( CPlayer* opponent, int32_t damage, bool destroyed );   ///< Record dealt damage.
	void       reclaim_shield();                                                    ///< Restore unused shield.
	bool       reduce_clock();                                                      ///< Tick the aim clock.
	void       save_game_data( FILE* file );                                       ///< Write savegame data.
	void       save_to_file( FILE* file );                                         ///< Write settings.
	void       set_last_opponent( sOpponent* last_opp );                             ///< Remember last attacker.
	void       set_name( char const* name_ );                                          ///< Set the player name.
	void       update_preferences( int32_t max_boost, int32_t max_score );          ///< Refresh AI preferences.

	/* Get the names of the player and their team */
	[[nodiscard]] char const* get_name() const;     ///< Player name.
	[[nodiscard]] char const* get_team_name() const; ///< Team name.

	/* Get a (somewhat) personalized retaliation phrase (MUST be freed!) */
	/// Select a personalized retaliation phrase.
	[[nodiscard]] char const* select_retaliation_phrase() const;

	/* Other phrase selectors are unpersonalized and can therefore be static. (Must NOT be freed!) */
	static char const* select_gloat_phrase();                  ///< Select a gloating line.
	static char const* select_panic_phrase( CPlayer* shocker ); ///< Select a panic line.
	static char const* select_kamikaze_phrase();               ///< Select a kamikaze line.
	static char const* select_revenge_phrase();                ///< Select a revenge line.
	static char const* select_suicide_phrase();                ///< Select a suicide line.


	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	int32_t        color            = BLACK;          ///< Player color.
	double         damage_multiplier = 1.;             ///< Damage multiplier.
	double         defensive        = 0.;             ///< Offense/defense balance.
	double         error_multiplier  = 0.;             ///< Aim error multiplier.
	bool           changed_weapon   = false;          ///< Weapon changed this turn.
	double         focus_rate        = 0.;             ///< Aim focus rate.
	bool           gloating         = false;          ///< Gloating now.
	int32_t        index            = -1;             ///< Index in all_players.
	int32_t        killed           = 0;              ///< Deaths.
	int32_t        kills            = 0;              ///< Kills.
	int32_t        last_shield_used = 0;              ///< Last used shield type.
	double         pain_sensitivity  = .5;             ///< Damage sensitivity.
	uint32_t       played           = 0;              ///< Rounds played.
	EPlayerPrefType pref_type         = PERPLAY_PREF;   ///< Preference scope.
	EPlayerType     previous_type    = HUMAN_PLAYER;   ///< Type kept while overridden.
	int32_t        money            = 15000;          ///< Cash.
	int32_t        ni[ ITEMS ]{};                     ///< Item inventory.
	int32_t        nm[ WEAPONS ]{};                   ///< Weapon inventory.
	CPlayer*        revenge = nullptr;                 ///< Revenge target.
	int32_t        score   = 0;                       ///< Score.
	abool_t        sdi_has_fired{ false };            ///< Only one shot per frame.
	int32_t        sdi_shots           = 0;            ///< SDI shots fired.
	bool           selected           = false;        ///< Selected in menus.
	double         self_preservation   = .5;           ///< Self-harm avoidance.
	bool           skip_me            = false;        ///< Skip this turn.
	CTank*          tank               = nullptr;      ///< Controlled tank.
	int32_t        tank_bitmap         = TT_NORMAL;    ///< Tank skin.
	ETeamTypes     team               = TEAM_NEUTRAL; ///< Team.
	int32_t        time_left_to_fire  = 0;            ///< Aim time left.
	EPlayerType     type               = HUMAN_PLAYER; ///< Player type.
	EPlayerType     type_saved         = HUMAN_PLAYER; ///< Type kept across savegames.
	double         vengeance_threshold = .5;           ///< Damage warranting revenge.
	int32_t        vengeful           = 50;           ///< Retaliation chance.
	uint32_t       won                = 0;            ///< Rounds won.
#ifdef NETWORK
	int32_t server_socket                   = 0;
	char    net_command[ NET_COMMAND_SIZE ] = { 0 };
#endif // NETWORK


private:
	typedef EPlayerStages plstage_t;
	typedef sOpponent     opp_t;


	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	/* Specialized boosters */
	[[nodiscard]] double boost_amp_pref( double old_pref, int32_t idx, int32_t ai_level ) const;
	[[nodiscard]] double boost_armour_pref( double old_pref, int32_t idx, int32_t ai_level ) const;

	/* General booster */
	void     boost_prefences( bool boost_armour, bool boost_amps, bool boost_weapons );

	bool     buy_item( int32_t item_index, int32_t max_boost );
	EControl computer_controls( CAICore* aicore, bool allow_fire );
	int32_t  computer_select_pre_buy_item( int32_t max_boost );
	int32_t  generate_desired_list();
	int32_t  get_amp_value();
	int32_t  get_armour_value();
	EControl human_controls( CAICore* aicore );


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t   boost_bought = -1;
	int32_t   boost_pref[ THINGS ]{}; // used to carry "boosts" over to the next round (see notes in generate_desired_list())
	int32_t   curr_pref[ THINGS ]{};  // current preferences, calculated for each round
	int32_t   desired[ THINGS ]{};   // Shopping wish list
	int32_t   save_money_for[ THINGS ]{}; // List of items the AI wants to buy
	opp_t*    last_opponent = nullptr;
	string    name{ "New Player" };
	bool      need_amp      = false;
	bool      need_armour   = false;
	bool      need_damage   = false;
	int32_t   opp_count     = 0;
	opp_t*    opponents    = nullptr;
	plstage_t pl_stage      = PS_SELECT_WEAPON;
	int32_t   shield_bought = -1;
	int32_t   weap_pref[ THINGS ]{}; // Static preferences, generated once
};

// For headers including player.h to know that the class is there:
#define HAS_PLAYER 1

// Note: Due to circular dependencies, some headers might need to forward
//       the player class.


/** @struct PlayerMini
 * @brief Minimum dataset of editable values.
 *
 * This minimal struct is used for the player editing menu.
 *   - When adding a new player it allows to cancel the addition without
 * atanks to first call new and then delete. Further it keeps the last
 * settings so adding many new players with the same settings but different
 * names becomes very easy.
 **/
struct PlayerMini {
	int32_t        color = GREEN;                        ///< Edited color.
	int32_t        index = -1;                           ///< Edited index.
	char           name[ NAME_LEN + 1 ]{ "New Player" }; ///< Edited name.
	uint32_t       played     = 0;                       ///< Edited rounds played.
	CPlayer*        player     = nullptr;                 ///< Edited player.
	EPlayerPrefType pref_type   = ALWAYS_PREF;             ///< Edited preference scope.
	int32_t        tank_bitmap = TT_NORMAL;               ///< Edited tank skin.
	ETeamTypes     team       = TEAM_NEUTRAL;            ///< Edited team.
	EPlayerType     type       = HUMAN_PLAYER;            ///< Edited type.
	uint32_t       won        = 0;                       ///< Edited rounds won.

	// a ctor, needed by VisualC++ for the name.
	/// Construct edit defaults.
	explicit PlayerMini();

	// "Backup a player"
	/// Copy a player for editing.
	void copy_from( CPlayer* source );

	// Write back the values
	/// Write edits back.
	void write_back( CPlayer* target = nullptr );
};

#define HAS_PLAYER_MINI 1

// Helper functions to be used as action function with ET_BUTTON entries
int32_t edit_player( CPlayer** target, int32_t );
int32_t new_player( CPlayer** target, int32_t );


#endif // ATANKS_PLAYER_H_INCLUDED
