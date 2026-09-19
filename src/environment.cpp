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


#include "environment.h"

#include "files.h"
#include "item.h"
#include "main.h"
#include "missile.h"
#include "player.h"
#include "random.h"
#include "tank.h"

#include <cassert>

CEnvironment::CEnvironment() {
	set_fps( 60 ); // rock solid default.

	font_height = 10; // Initial value

	strncpy( server_name, "127.0.0.1", 127 );
	strncpy( server_port, "25645", 127 );

	// Reserve space for the players array:
	// Note: The all_players array is dynamically (re-)allocated while loading
	//       stored players from the configuration.
	if ( ( players = (CPlayer**)calloc( MAXPLAYERS, sizeof( CPlayer* ) ) ) == nullptr ) {
		perror( "environment.cpp: Failed allocating memory for players" );
	}

	// sin/cos short-cuts, With only 1° granularity the arrays are always
	// faster than live calculations.
	for ( int32_t i = 0; i < 360; i++ ) {
		slope[ i ][ 0 ] = std::sin( DEG2RAD( i ) );
		slope[ i ][ 1 ] = std::cos( DEG2RAD( i ) );
	}
}

/** @brief default dtor
 * Cleanly remove created objects
 **/
CEnvironment::~CEnvironment() {
	this->destroy();
}

/// @brief add a player to the players[] array that will take part in the next game
void CEnvironment::add_game_player( CPlayer* player_ ) {
	if ( player_ && ( num_game_players < MAXPLAYERS ) ) {

		// Ensure the player isn't already there:
		for ( int32_t i = 0; i < num_game_players; ++i ) {
			if ( player_ == players[ i ] ) {
				return;
			}
		}

		players[ num_game_players++ ] = player_;

		if ( HUMAN_PLAYER == player_->type ) {
			num_human_players++;
		}
	}
}

/// @brief create a new player or return nullptr if an error occurred
CPlayer* CEnvironment::create_new_player( char const* player_name ) {
	CPlayer** reallocatedPlayers = nullptr;
	CPlayer*  player             = nullptr;

	assert( player_name && "ERROR: player_name is nullptr!" );

	if ( nullptr == player_name ) {
		return nullptr;
	}

	if ( get_player_by_name( player_name ) > -1 ) {
		return nullptr;
	}

	reallocatedPlayers = (CPlayer**)realloc( all_players, sizeof( CPlayer* ) * ( num_permanent_players + 1 ) );

	if ( reallocatedPlayers ) {
		all_players = reallocatedPlayers;
	} else {
		perror( "environment.cpp: Failed allocating memory for reallocatedPlayers in CEnvironment::create_new_player" );
	}

	try {
		player = new CPlayer();
	} catch ( std::exception& e ) {
		std::cerr << __func__ << " new CPlayer: " << e.what() << std::endl;
	}

	player->index = num_permanent_players;
	player->setName( player_name );
	all_players[ num_permanent_players++ ] = player;

	return player;
}

/// @brief This function gives credits, score and money to the winner(s).
void CEnvironment::credit_winners( int32_t winner ) const {
	if ( winner == WINNER_DRAW ) { // no winner
		return;
	}

	int32_t team_members = 0;

	if ( winner == WINNER_JEDI ) {
		for ( int32_t i = 0; i < num_game_players; ++i ) {
			if ( TEAM_JEDI == players[ i ]->team ) {
				players[ i ]->score++;
				players[ i ]->won++;
				team_members++;
			}
		}
	} else if ( winner == WINNER_SITH ) {
		for ( int32_t i = 0; i < num_game_players; ++i ) {
			if ( TEAM_SITH == players[ i ]->team ) {
				players[ i ]->score++;
				players[ i ]->won++;
				team_members++;
			}
		}
	} else if ( winner < WINNER_NO_WIN ) {
		players[ winner ]->score++;
		players[ winner ]->won++;
		players[ winner ]->money += scoreRoundWinBonus;
	}

	// team gets their money too
	if ( team_members ) {
		int32_t team_bonus = scoreRoundWinBonus / team_members;
		for ( int32_t i = 0; i < num_game_players; ++i ) {
			if ( ( ( winner == WINNER_JEDI ) && ( players[ i ]->team == TEAM_JEDI ) )
			     || ( ( winner == WINNER_SITH ) && ( players[ i ]->team == TEAM_SITH ) ) ) {
				players[ i ]->money += team_bonus;
			}
		}
	}
}

void CEnvironment::decrease_volume() {
	if ( volume_factor > 0 ) {
		--volume_factor;
	}
}

/// @brief Remove one of the players, then gone for good.
void CEnvironment::delete_perm_player( CPlayer* player_ ) {
	int32_t toCount = 0;

	for ( int32_t fromCount = 0; fromCount < num_permanent_players; fromCount++ ) {
		if ( all_players[ fromCount ] != player_ ) {
			if ( all_players[ toCount ] != all_players[ fromCount ] ) {
				all_players[ toCount ]        = all_players[ fromCount ];
				all_players[ toCount ]->index = toCount;
			}
			toCount++;
		}
	}
	num_permanent_players--;

	delete player_;
}

/** @brief Free all allocated memory.
 *
 * Important: This MUST be called *before* allegro shuts down!
 **/
void CEnvironment::destroy() {
	if ( sky ) {
		destroy_bitmap( sky );
		sky = nullptr;
	}

	if ( bitmap_filenames ) {
		for ( int32_t count = 0; count < number_of_bitmaps; ++count ) {
			if ( bitmap_filenames[ count ] ) {
				free( bitmap_filenames[ count ] );
			}
		}
		free( bitmap_filenames );
		bitmap_filenames = nullptr;
	}

	if ( saved_game_list_size && saved_game_list ) {
		for ( uint32_t i = 0; i < saved_game_list_size; ++i ) {
			if ( saved_game_list[ i ] ) {
				free( const_cast< char* >( saved_game_list[ i ] ) );
			}
			saved_game_list[ i ] = nullptr;
		}
		free( saved_game_list );
		saved_game_list      = nullptr;
		saved_game_list_size = 0;
	}

	if ( music_dir ) {
		closedir( music_dir );
		music_dir = nullptr;
	}

	if ( background_music ) {
		destroy_sample( background_music );
		background_music = nullptr;
	}

	if ( sounds ) {
		int32_t index = 0;
		while ( sounds[ index ] ) {
			destroy_sample( sounds[ index++ ] );
		}
		free( sounds );
		sounds = nullptr;
	}

	if ( title ) {
		int32_t index = 0;
		while ( title[ index ] ) {
			destroy_bitmap( title[ index++ ] );
		}
		free( title );
		title = nullptr;
	}

	if ( button ) {
		int32_t index = 0;
		while ( button[ index ] ) {
			destroy_bitmap( button[ index++ ] );
		}
		free( button );
		button = nullptr;
	}

	if ( misc ) {
		int32_t index = 0;
		while ( misc[ index ] ) {
			destroy_bitmap( misc[ index++ ] );
		}
		free( misc );
		misc = nullptr;
	}

	if ( missile ) {
		int32_t index = 0;
		while ( missile[ index ] ) {
			destroy_bitmap( missile[ index++ ] );
		}
		free( missile );
		missile = nullptr;
	}

	if ( stock ) {
		int32_t index = 0;
		while ( stock[ index ] ) {
			destroy_bitmap( stock[ index++ ] );
		}
		free( stock );
		stock = nullptr;
	}

	if ( tank ) {
		int32_t index = 0;
		while ( tank[ index ] ) {
			destroy_bitmap( tank[ index++ ] );
		}
		free( tank );
		tank = nullptr;
	}

	if ( tank_gun ) {
		int32_t index = 0;
		while ( tank_gun[ index ] ) {
			destroy_bitmap( tank_gun[ index++ ] );
		}
		free( tank_gun );
		tank_gun = nullptr;
	}

	if ( gloat ) {
		delete gloat;
		gloat = nullptr;
	}
	if ( ingame ) {
		delete ingame;
		ingame = nullptr;
	}
	if ( instructions ) {
		delete instructions;
		instructions = nullptr;
	}
	if ( panic ) {
		delete panic;
		panic = nullptr;
	}
	if ( kamikaze ) {
		delete kamikaze;
		kamikaze = nullptr;
	}
	if ( retaliation ) {
		delete retaliation;
		retaliation = nullptr;
	}
	if ( revenge ) {
		delete revenge;
		revenge = nullptr;
	}
	if ( suicide ) {
		delete suicide;
		suicide = nullptr;
	}
	if ( war_quotes ) {
		delete war_quotes;
		war_quotes = nullptr;
	}

	if ( all_players ) {
		for ( int32_t i = 0; i < num_permanent_players; ++i ) {
			if ( all_players[ i ] ) {
				delete all_players[ i ];
			}
			all_players[ i ] = nullptr;
		}
		free( all_players );
		all_players = nullptr;
	}

	if ( players ) {
		for ( int32_t i = 0; i < MAXPLAYERS; ++i ) {
			players[ i ] = nullptr;
		}
		free( players );
		players = nullptr;
	}

	if ( main_font ) {
		destroy_font( main_font );
		main_font = nullptr;
	}

	gfx_data.destroy();
}

/// @brief Sets config_dir to the path to the config directory used by atanks
void CEnvironment::find_config_dir() {
	// If no config dir was given on the command line, try to find a valid one
	if ( !config_dir[ 0 ] ) {
		// figure out file name
		char* homedir  = getenv( HOME_DIR );
		config_dir      = homedir ? homedir : ".";
		config_dir     += "/.atanks";

		// copy the file over, if we did not yet
		if ( !Copy_Config_File() ) {
			// If it did not work, look whether the directory already exists:
			DIR* pDestDir = opendir( env.config_dir.c_str() );
			if ( !pDestDir ) {
				cerr << "ERROR: An error has occurred trying to set up"
				     << " Atomic Tanks folders." << endl;
			} else {
				closedir( pDestDir );
				pDestDir = nullptr;
			}
		}
	} // end of no config file on the command line
}

/// @brief Sets data_dir to the path 'unicode.dat' can be found in.
bool CEnvironment::find_data_dir() {

	// If the datadir set by command line options, try that first
	if ( !data_dir.empty() ) {
		if ( !access( data_dir.c_str(), R_OK ) ) {
			return true;
		} else {
			cerr << "ERROR: The given datadir \"" << data_dir << "\""
			     << " is invalid!" << endl;
			data_dir.clear();
		}
	}

	// Try the set directory from the build
	if ( !access( DATA_DIR "/unicode.dat", R_OK ) ) {
		data_dir.assign( DATA_DIR );
	} else {
		// This was not successful, try the current directory if not tried, yet.

		if ( ( 0 == strncmp( DATA_DIR, ".", 1 ) ) && ( 0 == strncmp( DATA_DIR, "./", 2 ) ) ) {
			// Try again and reset if unsuccessful
			if ( !access( "./unicode.dat", R_OK ) ) {
				data_dir.assign( "." );
			}
		}
	}

	// If data_dir is set, now, this was a success.
	return !data_dir.empty();
}

/// @brief Must be called before CGlobalData::first_init() is called!
void CEnvironment::first_init() {
	// Determine maximum number of updates before doing
	// a full update:
	max_screen_updates = ROUND( std::sqrt( ROUND( screen_width / 8 ) * ROUND( screen_height / 8 ) ) );
	/* This is:
	 *  800 x  600 => sqrt(100 *  75) => sqrt( 7500) =  87
	 * 1280 x 1024 => sqrt(160 *  28) => sqrt(20480) = 143
	 * 1600 x  900 => sqrt(200 * 113) => sqrt(22600) = 150
	 * 1920 x 1080 => sqrt(240 * 135) => sqrt(32400) = 180
	 */

	// Get memory ...
	if ( !sky ) {
		sky = create_bitmap( screen_width, screen_height - MENUHEIGHT );
	}
	if ( !sky ) {
		cout << "Failed to create sky bitmap: " << allegro_error << endl;
		exit( 1 );
	}

	initialise();

	menu_begin_y = ( screen_height - 400 ) / 2;
	if ( menu_begin_y < 0 ) {
		menu_begin_y = 0;
	}
	menu_end_y = screen_height - menu_begin_y;

	gfx_data.first_init();
}

/// @brief Fill available_items array with everything buyable with current settings.
void CEnvironment::gen_items_list() {
	int32_t slot = 0;
	for ( int32_t i = 0; i < THINGS; ++i ) {
		if ( is_item_available( i ) ) {
			available_items[ slot++ ] = i;
		}
	}
	num_available = slot;
}

/// @brief return the index of the player with @a player_name or -1 if not found
int32_t CEnvironment::get_player_by_name( char const* player_name ) const {
	int32_t result = -1;

	assert( player_name && "ERROR: player_name is nullptr!" );

	if ( nullptr == player_name ) {
		return result;
	}

	for ( int32_t i = 0; ( -1 == result ) && ( i < num_permanent_players ); ++i ) {
		if ( !strcmp( player_name, all_players[ i ]->getName() ) ) {
			result = i;
		}
	}

	return result;
}

void CEnvironment::increase_volume() {
	if ( volume_factor < MAX_VOLUME_FACTOR ) {
		++volume_factor;
	}
}

int32_t CEnvironment::in_game_menu() const {
	int32_t     pressed   = -1;
	bool        need_draw = true;
	int32_t     btns[ INGAMEBUTTONS ];
	bool        updatew[ INGAMEBUTTONS ];
	char const* buttext[ INGAMEBUTTONS ] = {
		ingame->Get_Line( 69 ),
		ingame->Get_Line( 70 ),
		ingame->Get_Line( 71 ),
		ingame->Get_Line( 72 ),
	};

	// Set/calculate button size and positions
	int32_t b_width  = 150;
	int32_t b_height = 20;
	int32_t b_space  = 5;
	int32_t b_half_w = b_width / 2;
	int32_t b_left   = half_width - b_half_w;
	int32_t b_right  = half_width + b_half_w - 1;

	int32_t d_width  = 200;
	int32_t d_height = ( ( INGAMEBUTTONS + 2 ) * b_height ) + ( ( INGAMEBUTTONS + 1 ) * b_space );
	int32_t d_half_w = d_width / 2;
	int32_t d_half_h = d_height / 2;

	int32_t d_left   = half_width - d_half_w;
	int32_t d_right  = half_width + d_half_w - 1;
	int32_t d_top    = half_height - d_half_h;
	int32_t d_bottom = half_height + d_half_h - 1;

	// store last mouse coordinates for movement detection
	int32_t lastMouse_x = 0;
	int32_t lastMouse_y = 0;

	// Calculate button y values and set all button status to 0
	int32_t y = -d_half_h + b_height + b_space;

	for ( int32_t i = 0; i < INGAMEBUTTONS; ++i ) {
		updatew[ i ]  = false;
		btns[ i ]     = y;
		y            += b_height + b_space;
	}

	SHOW_MOUSE( nullptr )
	k = 0;
	K = 0;

	global.make_update( d_left, d_top, d_width, d_height );
	rectfill( global.canvas, d_left, d_top, d_right, d_bottom, GREY );
	rect( global.canvas, d_left, d_top, d_right, d_bottom, BLACK );

	while ( -1 == pressed ) {
		LINUX_REST;
		if ( keypressed() ) {
			k = readkey();
			K = k >> 8;
		}

		// look for keyboard exit
		if ( ( K == KEY_ESC ) || ( K == KEY_P ) ) {
			pressed = -2;
			continue;
		}

		// Check mouse movement
		if ( !env.os_mouse && ( ( lastMouse_x != mouse_x ) || ( lastMouse_y != mouse_y ) ) ) {
			lastMouse_x = mouse_x;
			lastMouse_y = mouse_y;
			need_draw   = true;
		}

		if ( mouse_b & 1 ) {
			bool is_hit = false;
			for ( int32_t i = 0; !is_hit && ( i < INGAMEBUTTONS ); ++i ) {
				if ( ( mouse_x >= b_left ) && ( mouse_x < b_right ) && ( mouse_y >= ( btns[ i ] + half_height ) )
				     && ( mouse_y < ( btns[ i ] + b_height + half_height ) ) ) {

					is_hit = true;

					if ( pressed > -1 ) {
						updatew[ pressed ] = true;
					}
					pressed      = i;
					updatew[ i ] = true;
				}
			}

			if ( !is_hit ) {
				if ( pressed > -1 ) {
					updatew[ pressed ] = true;
				}
				pressed = -1;
			}
		}

		// Update buttons
		for ( int32_t i = 0; i < INGAMEBUTTONS; ++i ) {
			if ( updatew[ i ] ) {
				updatew[ i ] = false;
				global.make_update( b_left, half_height + btns[ i ], b_width, b_height );
			}
		}

		// Draw buttons
		if ( need_draw ) {
			SHOW_MOUSE( nullptr )

			for ( int32_t i = 0; i < INGAMEBUTTONS; ++i ) {
				draw_sprite( global.canvas, misc[ ( pressed == i ) ? 8 : 7 ], b_left, half_height + btns[ i ] );
				textout_centre_ex(
					global.canvas,
					font,
					buttext[ i ],
					half_width,
					half_height + btns[ i ] + 1,
					WHITE,
					-1
				);
			}

			// Update non-OS mouse movements
			SHOW_MOUSE( global.canvas )

			global.do_updates();
			need_draw = false;
		}
	} // end of menu loop

	return pressed;
}

void CEnvironment::initialise() {
	campaign_rounds = static_cast< double >( rounds ) / 5.;
	if ( campaign_rounds < 1. ) {
		campaign_rounds = 1.;
	}

	next_campaign_round = static_cast< double >( rounds ) - campaign_rounds;
}

/// @return true if the items tech level is not too high and if it is not a warhead.
bool CEnvironment::is_item_available( int32_t itemNum ) const {
	if ( itemNum < WEAPONS ) {
		if ( ( weapon[ itemNum ].warhead ) || ( weapon[ itemNum ].techLevel > weapontech_level ) ) {
			return false;
		}
	} else if ( item[ itemNum - WEAPONS ].techLevel > itemtech_level ) {
		return false;
	}
	return true;
}

/*
This function loads environment settings from a text
file. The function returns true on success and false if
any erors are encountered.
-- Jesse
*/
void CEnvironment::load_from_file( FILE* file ) {
	char  line[ MAX_CONFIG_LINE + 1 ]  = { 0 };
	char  field[ MAX_CONFIG_LINE + 1 ] = { 0 };
	char  value[ MAX_CONFIG_LINE + 1 ] = { 0 };
	char* result                       = nullptr;
	bool  done                         = false;
	bool  sound_bookmark               = sound_enabled; // To disable by command line

	// read until we hit line "*ENV*" or "***" or EOF
	do {
		result = fgets( line, MAX_CONFIG_LINE, file );
		if ( !result || !strncmp( line, "***", 3 ) ) {
			// eof or end of record
			return;
		} else if ( !strncmp( line, "*GLOBAL*", 8 ) ) {
			// Old style config/save file
			rewind( file );
			CGlobalData::load_from_file( file );
		}
	} while ( 0 != strncmp( line, "*ENV*", 5 ) );
	// read until we hit new record

	while ( ( result ) && ( !done ) ) {
		// read a line
		memset( line, 0, MAX_CONFIG_LINE );
		result = fgets( line, MAX_CONFIG_LINE, file );

		// if we hit end of the record, stop
		if ( 0 == strncmp( line, "***", 3 ) ) {
			done = true;
		}

		if ( result && !done ) {

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

			// seperate field from value
			memset( field, '\0', MAX_CONFIG_LINE );
			memset( value, '\0', MAX_CONFIG_LINE );
			strncpy( field, line, equal_position );
			strncpy( value, &( line[ equal_position + 1 ] ), 127 );

			// check for fields and values
			if ( !strcasecmp( field, "acceleratedai" ) ) {
				SAFE_STOI( skip_computer_play, value );
				if ( skip_computer_play > SKIP_HUMANS_DEAD ) {
					skip_computer_play = SKIP_HUMANS_DEAD;
				}
			} else if ( !strcasecmp( field, "checkupdates" ) ) {
				int32_t check = 0;
				SAFE_STOI( check, value );
				check_for_updates = check > 0;
			} else if ( !strcasecmp( field, "colourtheme" ) ) {
				SAFE_STOI( colour_theme, value );
				if ( colour_theme < CT_REGULAR ) {
					colour_theme = CT_REGULAR;
				}
				if ( colour_theme > CT_CRISPY ) {
					colour_theme = CT_CRISPY;
				}
			} else if ( !strcasecmp( field, "debrislevel" ) ) {
				SAFE_STOI( debris_level, value );
			} else if ( !strcasecmp( field, "detailedland" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				detailed_landscape = val > 0;
			} else if ( !strcasecmp( field, "detailedsky" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				detailed_sky = val > 0;
			} else if ( !strcasecmp( field, "dither" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				dither_gradients = val > 0;
			} else if ( !strcasecmp( field, "dividemoney" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				divide_money = val > 0;
			} else if ( !strcasecmp( field, "doboxwrap" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				do_box_wrap = val > 0;
			} else if ( !strcasecmp( field, "dynamicmenubg" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				dynamic_menu_bg = val > 0;
			} else if ( !strcasecmp( field, "frames" ) ) {
				int32_t new_fps = 0;
				SAFE_STOI( new_fps, value );
				set_fps( new_fps );
			} else if ( !strcasecmp( field, "fullscreen" ) ) {
				SAFE_STOI( full_screen, value );
			} else if ( !strcasecmp( field, "interest" ) ) {
				SAFE_STOD( interest, value );
			} else if ( !strcasecmp( field, "language" ) ) {
				uint32_t lang_val = 0;
				SAFE_STOUL( lang_val, value );
				language = static_cast< ELanguages >( lang_val );
			} else if ( !strcasecmp( field, "maxfiretime" ) ) {
				SAFE_STOI( max_fire_time, value );
			} else if ( !strcasecmp( field, "networking" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				network_enabled = val > 0;
			} else if ( !strcasecmp( field, "networkport" ) ) {
				SAFE_STOI( network_port, value );
			} else if ( !strcasecmp( field, "numpermanentplayers" ) ) {
				SAFE_STOI( num_permanent_players, value );
			} else if ( !strcasecmp( field, "osmouse" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				os_mouse = val > 0;
			} else if ( !strcasecmp( field, "playmusic" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				play_music = val > 0;
			} else if ( !strcasecmp( field, "rounds" ) ) {
				SAFE_STOUL( rounds, value );
			} else if ( !strcasecmp( field, "scoreboard" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				global.show_score_board = val > 0;
			} else if ( !strcasecmp( field, "scorehitunit" ) ) {
				SAFE_STOI( scoreHitUnit, value );
			} else if ( !strcasecmp( field, "scoreroundwinbonus" ) ) {
				SAFE_STOI( scoreRoundWinBonus, value );
			} else if ( !strcasecmp( field, "scoreselfhit" ) ) {
				SAFE_STOI( scoreSelfHit, value );
			} else if ( !strcasecmp( field, "scoreteamhit" ) ) {
				SAFE_STOI( scoreTeamHit, value );
			} else if ( !strcasecmp( field, "scoreunitdestroybonus" ) ) {
				SAFE_STOI( scoreUnitDestroyBonus, value );
			} else if ( !strcasecmp( field, "scoreunitselfdestroy" ) ) {
				SAFE_STOI( scoreUnitSelfDestroy, value );
			} else if ( !strcasecmp( field, "sell_percent" ) ) {
				SAFE_STOD( sell_percent, value );
			}
#ifdef NETWORK
			else if ( !strcasecmp( field, "servername" ) ) {
				string s( value );
				size_t start = s.find_first_of( '\'' ) + 1;
				size_t end   = s.find_last_of( '\'' );

				if ( start < end ) {
					strncpy( server_name, s.substr( start, end - start ).c_str(), 128 );
				}
			} else if ( !strcasecmp( field, "serverport" ) ) {
				string s( value );
				size_t start = s.find_first_of( '\'' ) + 1;
				size_t end   = s.find_last_of( '\'' );

				if ( start < end ) {
					strncpy( server_port, s.substr( start, end - start ).c_str(), 128 );
				}
			}
#endif // NETWORK
			else if ( !strcasecmp( field, "showaifeedback" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				show_ai_feedback = val > 0;
			} else if ( !strcasecmp( field, "showfps" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				show_fps = val > 0;
			} else if ( !strcasecmp( field, "soundenabled" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				sound_enabled = val > 0;
			} else if ( !strcasecmp( field, "sounddriver" ) ) {
				SAFE_STOI( sound_driver, value );
			} else if ( !strcasecmp( field, "start_money" ) ) {
				SAFE_STOI( start_money, value );
			} else if ( !strcasecmp( field, "turn_type" ) ) {
				SAFE_STOI( turn_type, value );
			} else if ( !strcasecmp( field, "violentdeath" ) ) {
				SAFE_STOI( violent_death, value );
			} else if ( !strcasecmp( field, "wind_strength" ) ) {
				SAFE_STOI( wind_strength, value );
			} else if ( !strcasecmp( field, "wind_variation" ) ) {
				SAFE_STOI( wind_variation, value );
			} else if ( !strcasecmp( field, "viscosity" ) ) {
				SAFE_STOD( viscosity, value );
				if ( viscosity < 0.25 ) {
					viscosity = 0.5;
				}
			} else if ( !strcasecmp( field, "gravity" ) ) {
				SAFE_STOD( gravity, value );
				if ( gravity < 0.025 ) {
					gravity = 0.15;
				}
				fall_vector = gravity * fps_mod;
			} else if ( !strcasecmp( field, "techlevel" ) ) {
				SAFE_STOI( weapontech_level, value );
				itemtech_level = weapontech_level; // for backward compatibility
			} else if ( !strcasecmp( field, "weapontechlevel" ) ) {
				SAFE_STOI( weapontech_level, value );
			} else if ( !strcasecmp( field, "itemtechlevel" ) ) {
				SAFE_STOI( itemtech_level, value );
			} else if ( !strcasecmp( field, "meteors" ) ) {
				SAFE_STOI( meteors, value );
			} else if ( !strcasecmp( field, "lightning" ) ) {
				SAFE_STOI( lightning, value );
			} else if ( !strcasecmp( field, "satellite" ) ) {
				SAFE_STOI( satellite, value );
			} else if ( !strcasecmp( field, "fog" ) ) {
				SAFE_STOI( fog, value );
			} else if ( !strcasecmp( field, "landtype" ) ) {
				SAFE_STOI( land_type, value );
			} else if ( !strcasecmp( field, "landslidetype" ) ) {
				SAFE_STOI( landslide_type, value );
			} else if ( !strcasecmp( field, "walltype" ) ) {
				SAFE_STOI( wall_type, value );
			} else if ( !strcasecmp( field, "boxmode" ) ) {
				SAFE_STOI( boxed_mode, value );
			} else if ( !strcasecmp( field, "textfade" ) ) {
				int32_t res = 0;
				SAFE_STOI( res, value );
				fading_text = res != 0;
			} else if ( !strcasecmp( field, "textshadow" ) ) {
				int32_t res = 0;
				SAFE_STOI( res, value );
				shadowed_text = res != 0;
			} else if ( !strcasecmp( field, "textsway" ) ) {
				int32_t res = 0;
				SAFE_STOI( res, value );
				swaying_text = res != 0;
			} else if ( !strcasecmp( field, "landslidedelay" ) ) {
				SAFE_STOI( landslide_delay, value );
			} else if ( !strcasecmp( field, "fallingdirtballs" ) ) {
				SAFE_STOI( falling_dirt_balls, value );
				if ( falling_dirt_balls < 0 ) {
					falling_dirt_balls = 0;
				}
				if ( falling_dirt_balls > 3 ) {
					falling_dirt_balls = 3;
				}
			} else if ( !strcasecmp( field, "custombackground" ) ) {
				SAFE_STOI( custom_background, value );
			} else if ( !strcasecmp( field, "volumefactor" ) ) {
				SAFE_STOI( volume_factor, value );
			} else if ( !strcasecmp( field, "volleydelay" ) ) {
				SAFE_STOI( volley_delay, value );
			} else if ( !strcasecmp( field, "screenwidth" ) ) {
				SAFE_STOI( screen_width, value );
			} else if ( !strcasecmp( field, "screenheight" ) ) {
				SAFE_STOI( screen_height, value );
			}
		} // end of read a line properly
	}         // end of while not done

	// If values were set on the command line, override
	// configuration values:
	if ( temp_screen_height ) {
		screen_height = temp_screen_height;
	} else {
		temp_screen_height = screen_height;
	}
	if ( temp_screen_width ) {
		screen_width = temp_screen_width;
	} else {
		temp_screen_width = screen_width;
	}

	// The resolution must not be below 800x600:
	if ( screen_height < 600 ) {
		screen_height = 600;
	}
	if ( screen_width < 800 ) {
		screen_width = 800;
	}

	// The screen resolution values must be copied back into
	// the temp variables, which are then used by the menu,
	// or changing the resolution will make the menu exit crash.
	// The resolution is set only once on game start, so
	// these changes go into temp and are stored back from them.
	temp_screen_height = screen_height;
	temp_screen_width  = screen_width;

	if ( !sound_bookmark ) {
		sound_enabled = false;
	}

	half_width  = screen_width / 2;
	half_height = screen_height / 2;

	menu_begin_y = ( screen_height - 400 ) / 2;
	if ( menu_begin_y < 0 ) {
		menu_begin_y = 0;
	}
	menu_end_y = screen_height - menu_begin_y;
}

#define LOAD_TEXT_BLOCK( var, file )                                                   \
	try {                                                                          \
		string     text_file{ text_base + string( file ) + string( suffix ) }; \
		TEXTBLOCK* new_##var = new TEXTBLOCK( text_file.c_str() );             \
		delete ( var );                                                        \
		( var ) = new_##var;                                                   \
	} catch ( ... ) {}

/** @brief load text files according to set language
 * This function loads all needed text files, based on
 * language, into memory. If a previous text was loaded, it is
 * removed from memory first.
 **/
void CEnvironment::load_text_files() {
	char   suffix[ 12 ] = { 0 };
	int    r            = 0;
	string text_base{ data_dir };
	text_base += "/text/";
	string war_lines{ text_base };

	switch ( language ) {
		case EL_FRENCH:
			strncpy( suffix, "_fr.txt", 11 );
			war_lines += "war_quotes.txt";
			break;
		case EL_GERMAN:
			strncpy( suffix, "_de.txt", 11 );
			war_lines += "war_quotes.txt";
			break;
		case EL_ITALIAN:
			strncpy( suffix, "_it.txt", 11 );
			war_lines += "war_quotes_it.txt";
			break;
		case EL_PORTUGUESE:
			strncpy( suffix, ".pt_BR.txt", 11 );
			war_lines += "war_quotes.txt";
			break;
		case EL_RUSSIAN:
			strncpy( suffix, "_ru.txt", 11 );
			war_lines += "war_quotes_ru.txt";
			break;
		case EL_SLOVAK:
			strncpy( suffix, "_sk.txt", 11 );
			war_lines += "war_quotes.txt";
			break;
		case EL_SPANISH:
			strncpy( suffix, "_ES.txt", 11 );
			war_lines += "war_quotes_ES.txt";
			break;
		case EL_ENGLISH:
		default:
			strncpy( suffix, ".txt", 11 ); // default to english
			war_lines += "war_quotes.txt";
			break;
	}

	if ( r < 0 ) {
		abort();
	}

	try {
		auto new_war_quotes = new TEXTBLOCK( war_lines.c_str() );
		delete war_quotes;
		war_quotes = new_war_quotes;
	} catch ( ... ) { /* can't do anything helpful here anyway */
	}


	LOAD_TEXT_BLOCK( gloat, "gloat" )
	LOAD_TEXT_BLOCK( ingame, "ingame" )
	LOAD_TEXT_BLOCK( instructions, "instr" )
	LOAD_TEXT_BLOCK( panic, "panic" )
	LOAD_TEXT_BLOCK( kamikaze, "kamikaze" )
	LOAD_TEXT_BLOCK( retaliation, "retaliation" )
	LOAD_TEXT_BLOCK( revenge, "revenge" )
	LOAD_TEXT_BLOCK( suicide, "suicide" )
}

/** @brief load a background music file.
 *
 * This function loads a music file (if there is one available.)
 * If a current sample is set, it will be released.
 *
 * @return true if a sample is loaded, false otherwise.
 **/
bool CEnvironment::load_background_music() {
	bool isSecondTry = false;

	// see if we should bother
	if ( !play_music ) {
		return false;
	}

	SAMPLE* newStream    = nullptr;
	dirent* folder_entry = nullptr;
	string  music_path{ config_dir + "/music" };


	while ( true ) {

		// see if we have the music folder open
		if ( !music_dir ) {
			music_dir = opendir( music_path.c_str() );
			if ( !music_dir ) {
				return false;
			}
		}

		// At this point we should have an open music folder.
		// The music folder is closed by global's deconstructor.

		// Now search for files ending in .wav.
		while ( !newStream && ( nullptr != ( folder_entry = readdir( music_dir ) ) ) ) {
			// we have something, see if it is a wav file
			if ( strstr( folder_entry->d_name, ".wav" ) ) {
				newStream = load_sample( string( music_path + "/" + folder_entry->d_name ).c_str() );
			}
		}

		// If we have a stream, break off
		if ( newStream ) {
			break;
		}

		if ( !folder_entry ) {
			// hit end of folder
			closedir( music_dir );
			music_dir = nullptr;

			// If there is a current background music file loaded, then the directory is just gone through
			// completely. In that case one secound round would re-open the directory and start anew.
			if ( !isSecondTry && background_music ) {
				isSecondTry = true;
				continue;
			}
		} // end of not having a folder entry any more
	}         // end of "endless" loop


	if ( background_music ) {
		destroy_sample( background_music );
	}

	if ( newStream ) {
		background_music = newStream;
		return true;
	}

	// This is odd... end background music playing for good
	background_music = nullptr;
	play_music       = false;

	return false;
}

/*
 * This function loads all the bitmaps needed by the game.
 * Bitmaps are found in a series of sub-folders under the
 * data directory. The function returns true on success and
 * false if an error occurs.
 */
bool CEnvironment::load_bitmaps() {
	int32_t  file_group   = 0;
	BITMAP*  newbitmap    = nullptr;
	BITMAP** bitmap_array = nullptr;

	while ( file_group < 7 ) {
		// set the folder we're looking at
		string folder{ data_dir };
		switch ( file_group ) {
			case 0:
				folder += "/title/";
				break;
			case 1:
				folder += "/button/";
				break;
			case 2:
				folder += "/misc/";
				break;
			case 3:
				folder += "/missile/";
				break;
			case 4:
				folder += "/stock/";
				break;
			case 5:
				folder += "/tank/";
				break;
			case 6:
			default:
				folder += "/tank_gun/";
				break;
		}

		// set up empty array
		int32_t array_size = 10;
		bitmap_array       = (BITMAP**)calloc( 10, sizeof( BITMAP* ) );
		if ( !bitmap_array ) {
			printf( "Ran out of memory, loading bitmaps.\n" );
			return false;
		}

		// search for files
		int32_t file_count = 0;
		string  bitmap_path{ folder + std::to_string( file_count ) + ".bmp" };
		while ( !access( bitmap_path.c_str(), F_OK | R_OK ) && bitmap_array ) {
			newbitmap = load_bitmap( bitmap_path.c_str(), nullptr );
			if ( !newbitmap ) {
				printf( "An error occured loading bitmap %s\n", bitmap_path.c_str() );
			}

			// Crop tank bitmaps for unification
			if ( newbitmap && ( 5 == file_group ) ) {
				int32_t left   = 0;
				int32_t right  = newbitmap->w;
				int32_t top    = 0;
				int32_t bottom = newbitmap->h;

				// Find real left edge
				bool hasPix = false;
				while ( !hasPix && ( left < right ) ) {
					for ( int32_t y = top; !hasPix && ( y < bottom ); ++y ) {
						if ( PINK != getpixel( newbitmap, left, y ) ) {
							hasPix = true;
						}
					}
					if ( !hasPix ) {
						++left;
					}
				}

				// Find real right edge
				hasPix = false;
				while ( !hasPix && ( right > left ) ) {
					for ( int32_t y = top; !hasPix && ( y < bottom ); ++y ) {
						if ( PINK != getpixel( newbitmap, right, y ) ) {
							hasPix = true;
						}
					}
					if ( !hasPix ) {
						--right;
					}
				}

				// Find real top edge
				hasPix = false;
				while ( !hasPix && ( top < bottom ) ) {
					for ( int32_t x = left; !hasPix && ( x < right ); ++x ) {
						if ( PINK != getpixel( newbitmap, x, top ) ) {
							hasPix = true;
						}
					}
					if ( !hasPix ) {
						++top;
					}
				}

				// Find real bottom edge
				hasPix = false;
				while ( !hasPix && ( bottom > top ) ) {
					for ( int32_t x = left; !hasPix && ( x < right ); ++x ) {
						if ( PINK != getpixel( newbitmap, x, bottom ) ) {
							hasPix = true;
						}
					}
					if ( !hasPix ) {
						--bottom;
					}
				}

				// Now create the real bitmap
				bitmap_array[ file_count ] = create_bitmap( right - left, bottom - top );
				blit( newbitmap, bitmap_array[ file_count ], left, top, 0, 0, right - left, bottom - top );

				destroy_bitmap( newbitmap );
			} // End of cropping tank bitmap

			// otherwise just copy the bitmap pointer
			else {
				bitmap_array[ file_count ] = newbitmap;
			}

			file_count++;

			// make sure array is large enough
			if ( file_count >= array_size ) {
				array_size     += 10;
				auto new_array  = (BITMAP**)realloc( bitmap_array, sizeof( BITMAP* ) * ( array_size + 1 ) );
				if ( !new_array ) {
					printf( "Unable to increase array size while loading bitmaps.\n" );
					free( bitmap_array );
					return false;
				} else {
					bitmap_array = new_array;
					memset( bitmap_array + file_count, 0, sizeof( BITMAP* ) * ( array_size - file_count ) );
				}
			}

			// get next file
			bitmap_path.assign( folder + std::to_string( file_count ) + ".bmp" );
		}

		// save the new array
		switch ( file_group ) {
			case 0:
				title = bitmap_array;
				break;
			case 1:
				button = bitmap_array;
				break;
			case 2:
				misc = bitmap_array;
				break;
			case 3:
				missile = bitmap_array;
				break;
			case 4:
				stock = bitmap_array;
				break;
			case 5:
				tank = bitmap_array;
				break;
			case 6:
			default:
				tank_gun = bitmap_array;
				break;
		}

		file_group++;
	}

	return true;
}

// This file loads in extra fonts the game requires.
// Fonts should be stored in the datafolder. On
// success the function returns true. When an
// error occurs, it returns false.
bool CEnvironment::load_fonts() {
	string font_file{ data_dir + string( "/unicode.dat" ) };

	main_font = load_font( font_file.c_str(), nullptr, nullptr );

	if ( main_font ) {
		font = main_font;
	} else {
		printf( "Unable to load font %s\n", font_file.c_str() );
	}

	// Store font height
	if ( main_font ) {
		font_height = text_height( main_font );
	}

	return main_font != nullptr;
}

/// @brief collection of all game text file loadings.
bool CEnvironment::load_game_files() {
	// Before the (language specific) weapons texts can be loaded,
	// the english one must be pre-loaded to get the weapons data.
	// all other files only hold the texts.
	bool status = true;
	if ( EL_ENGLISH != language ) {
		ELanguages cur_lang = language;
		language            = EL_ENGLISH;
		status              = Load_Weapons_Text();
		language            = cur_lang;
	}

	if ( status ) {
		status = Load_Weapons_Text();
	}

	if ( !status ) {
		cerr << "ERROR: An error occurred trying to read weapons file." << endl;
		return status;
	}
	// Note: If english is chosen, the first load is not done.
	//       If english is not chosen, the first load pre-loads english
	//       Thus the second load is always necessary.


	bitmap_filenames = Find_Bitmaps( &number_of_bitmaps );

	// If no bitmaps where found, a custom background is futile.
	if ( custom_background && !bitmap_filenames ) {
		custom_background = 0;
	}

	Create_Music_Folder();
	gen_items_list();

	return status;
}

/** @brief load all needed sounds
 * This function loads all sounds from the data folder and saves them
 * in an array.
 * @return true on success or false if an error happens.
 **/
bool CEnvironment::load_sounds() {
	SAMPLE* temp_sample = nullptr;

	// allocate space for sound samples
	sounds = (SAMPLE**)calloc( SND_COUNT, sizeof( SAMPLE* ) );
	if ( !sounds ) {
		printf( "Unable to create sound array.\n" );
		return false;
	}

	// read from directory
	string sound_dir{ data_dir + string( "/sound/" ) };
	for ( int32_t i = 0; i < SND_COUNT; ++i ) {
		string sound_file{ sound_dir };
		sound_file += ( i < 10 ? "0" : "" ) + std::to_string( i ) + string( ".wav" );
		if ( !access( sound_file.c_str(), R_OK ) ) {
			temp_sample = load_sample( sound_file.c_str() );
			if ( temp_sample ) {
				sounds[ i ] = temp_sample;
			} else {
				fprintf( stderr, "An error occured loading sound file %s\n", sound_file.c_str() );
			}
		}
		// No else, because the sound enum has free slots.
	}

	return true;
}

void CEnvironment::new_round() {
	// set wall type
	if ( wall_type == WALL_RANDOM ) {
		current_wall_type = get_rand() % 4;
	} else {
		current_wall_type = wall_type;
	}

	time_to_fall = ( get_rand() & landslide_delay ) + 1;

	// Set boxed mode
	if ( BM_RANDOM == boxed_mode ) {
		if ( get_rand() % 2 ) {
			is_boxed = true;
		} else {
			is_boxed = false;
		}
	} else if ( BM_ON == boxed_mode ) {
		is_boxed = true;
	} else {
		is_boxed = false;
	}

	// Set wall colour
	switch ( current_wall_type ) {
		case WALL_RUBBER:
			wall_colour = makecol( 0, 255, 0 );
			break;
		case WALL_STEEL:
			wall_colour = makecol( 255, 0, 0 );
			break;
		case WALL_SPRING:
			wall_colour = makecol( 0, 0, 255 );
			break;
		case WALL_WRAP:
			wall_colour = makecol( 255, 255, 0 );
			break;
	}

	// Init player array
	for ( auto& i : player_order ) {
		i = nullptr;
	}
}

void CEnvironment::remove_game_player( CPlayer* player_ ) {
	int32_t fromCount = 0;
	int32_t toCount   = -1;

	if ( HUMAN_PLAYER == player_->type ) {
		num_human_players--;
	}

	while ( fromCount < num_game_players ) {
		if ( player_ != players[ fromCount ] ) {
			if ( ( toCount >= 0 ) && ( fromCount > toCount ) ) {
				players[ toCount ]   = players[ fromCount ];
				players[ fromCount ] = nullptr;
				toCount++;
			}
		} else {
			// Position found, now move the remaining players down!
			toCount = fromCount;
		}
		fromCount++;
	}
	num_game_players--;
}

/*
 * This function puts all the of the environment settings back
 * to their default values. These are settings which get written
 * to the config file.
 * -- Jesse
 *  */
void CEnvironment::reset_options() {
	boxed_mode          = BM_OFF;
	check_for_updates  = true;
	colour_theme        = CT_CRISPY;
	custom_background  = 0;
	debris_level       = 1;
	detailed_landscape  = false;
	detailed_sky        = false;
	dither_gradients    = true;
	divide_money       = false;
	fading_text         = false;
	falling_dirt_balls = 0;
	fog                = 0;
	set_fps( 60 );
	gravity               = 0.15;
	interest              = 1.25;
	itemtech_level         = 5;
	landslide_delay        = MAX_GRAVITY_DELAY;
	landslide_type         = SLIDE_GRAVITY;
	land_type              = LAND_RANDOM;
	language              = EL_ENGLISH;
	lightning             = 0;
	max_fire_time           = 0;
	meteors               = 0;
	network_enabled       = false;
	network_port          = DEFAULT_NETWORK_PORT;
	os_mouse               = true;
	play_music            = true;
	satellite             = 0;
	scoreHitUnit          = 75;
	scoreRoundWinBonus    = 10000;
	scoreSelfHit          = 25;
	scoreTeamHit          = 10;
	scoreUnitDestroyBonus = 5000;
	scoreUnitSelfDestroy  = 0;
	sell_percent           = 0.80;
	shadowed_text          = true;
	skip_computer_play      = SKIP_HUMANS_DEAD;
	sound_driver          = SD_AUTODETECT;
	sound_enabled         = true;
	start_money            = 15000;
	swaying_text           = true;
	temp_screen_height     = DEFAULT_SCREEN_HEIGHT;
	temp_screen_width      = DEFAULT_SCREEN_WIDTH;
	turn_type              = TURN_RANDOM;
	viscosity             = 0.5;
	violent_death         = 0;
	volley_delay          = 10;
	volume_factor         = MAX_VOLUME_FACTOR;
	wall_type              = WALL_RUBBER;
	weapontech_level       = 5;
	wind_strength          = 8;
	wind_variation         = 1;

	strncpy( server_name, "127.0.0.1", 127 );
	strncpy( server_port, "25645", 127 );
}

/** @brief Save environment settings to a text file
 *
 * This function saves the environment settings to a text file. Each line has
 * the format name=value.\n
 *
 * @return true on success and false on failure.
 */
bool CEnvironment::save_to_file( FILE* file ) {
	if ( !file ) {
		return false;
	}

	fprintf( file, "*ENV*\n" );

	fprintf( file, "ACCELERATEDAI=%d\n", skip_computer_play );
	fprintf( file, "BOXMODE=%d\n", boxed_mode );
	fprintf( file, "CHECKUPDATES=%d\n", check_for_updates ? 1 : 0 );
	fprintf( file, "COLOURTHEME=%d\n", colour_theme );
	fprintf( file, "CUSTOMBACKGROUND=%d\n", custom_background );
	fprintf( file, "DEBRISLEVEL=%d\n", debris_level );
	fprintf( file, "DETAILEDLAND=%d\n", detailed_landscape ? 1 : 0 );
	fprintf( file, "DETAILEDSKY=%d\n", detailed_sky ? 1 : 0 );
	fprintf( file, "DITHER=%d\n", dither_gradients ? 1 : 0 );
	fprintf( file, "DIVIDEMONEY=%d\n", divide_money );
	fprintf( file, "DOBOXWRAP=%d\n", do_box_wrap );
	fprintf( file, "DYNAMICMENUBG=%d\n", dynamic_menu_bg ? 1 : 0 );
	fprintf( file, "FALLINGDIRTBALLS=%d\n", falling_dirt_balls );
	fprintf( file, "FOG=%d\n", fog );
	fprintf( file, "FRAMES=%d\n", frames_per_second );
	fprintf( file, "FULLSCREEN=%d\n", full_screen );
	fprintf( file, "GRAVITY=%f\n", gravity );
	fprintf( file, "INTEREST=%f\n", interest );
	fprintf( file, "ITEMTECHLEVEL=%d\n", itemtech_level );
	fprintf( file, "LANDSLIDEDELAY=%d\n", landslide_delay );
	fprintf( file, "LANDSLIDETYPE=%d\n", landslide_type );
	fprintf( file, "LANDTYPE=%d\n", land_type );
	fprintf( file, "LANGUAGE=%u\n", static_cast< uint32_t >( language ) );
	fprintf( file, "LIGHTNING=%d\n", lightning );
	fprintf( file, "MAXFIRETIME=%d\n", max_fire_time );
	fprintf( file, "METEORS=%d\n", meteors );
	fprintf( file, "NETWORKING=%d\n", network_enabled ? 1 : 0 );
	fprintf( file, "NETWORKPORT=%d\n", network_port );
	fprintf( file, "NUMPERMANENTPLAYERS=%d\n", num_permanent_players );
	fprintf( file, "OSMOUSE=%d\n", os_mouse );
	fprintf( file, "PLAYMUSIC=%d\n", play_music ? 1 : 0 );
	fprintf( file, "ROUNDS=%d\n", rounds );
	fprintf( file, "SATELLITE=%d\n", satellite );
	fprintf( file, "SCOREBOARD=%d\n", global.show_score_board ? 1 : 0 );
	fprintf( file, "SCOREHITUNIT=%d\n", scoreHitUnit );
	fprintf( file, "SCOREROUNDWINBONUS=%d\n", scoreRoundWinBonus );
	fprintf( file, "SCORESELFHIT=%d\n", scoreSelfHit );
	fprintf( file, "SCORETEAMHIT=%d\n", scoreTeamHit );
	fprintf( file, "SCOREUNITDESTROYBONUS=%d\n", scoreUnitDestroyBonus );
	fprintf( file, "SCOREUNITSELFDESTROY=%d\n", scoreUnitSelfDestroy );
	fprintf( file, "SCREENHEIGHT=%d\n", temp_screen_height );
	fprintf( file, "SCREENWIDTH=%d\n", temp_screen_width );
	fprintf( file, "SELLPERCENT=%f\n", sell_percent );
	fprintf( file, "SERVERNAME='%s'\n", server_name );
	fprintf( file, "SERVERPORT='%s'\n", server_port );
	fprintf( file, "SHOWAIFEEDBACK=%d\n", show_ai_feedback ? 1 : 0 );
	fprintf( file, "SHOWFPS=%d\n", show_fps ? 1 : 0 );
	fprintf( file, "SOUNDDRIVER=%d\n", sound_driver );
	fprintf( file, "SOUNDENABLED=%d\n", sound_enabled ? 1 : 0 );
	fprintf( file, "STARTMONEY=%d\n", start_money );
	fprintf( file, "TEXTFADE=%d\n", fading_text ? 1 : 0 );
	fprintf( file, "TEXTSHADOW=%d\n", shadowed_text ? 1 : 0 );
	fprintf( file, "TEXTSWAY=%d\n", swaying_text ? 1 : 0 );
	fprintf( file, "TURNTYPE=%d\n", turn_type );
	fprintf( file, "VISCOSITY=%f\n", viscosity );
	fprintf( file, "VIOLENTDEATH=%d\n", violent_death );
	fprintf( file, "VOLLEYDELAY=%d\n", volley_delay );
	fprintf( file, "VOLUMEFACTOR=%d\n", volume_factor );
	fprintf( file, "WALLTYPE=%d\n", wall_type );
	fprintf( file, "WEAPONTECHLEVEL=%d\n", weapontech_level );
	fprintf( file, "WINDSTRENGTH=%d\n", wind_strength );
	fprintf( file, "WINDVARIATION=%d\n", wind_variation );
	fprintf( file, "***\n" );

	return true;
}

/// @brief This function sends a message to all connected game clients.
/// @return true on success or false if the message could not be sent
bool CEnvironment::send_to_clients( char const* message ) const {
	if ( !message ) {
		return false;
	}

#ifdef NETWORK
	ssize_t written        = 0;
	size_t  message_length = strlen( message );

	for ( int32_t index = 0; index < num_game_players; index++ ) {
		if ( ( players[ index ] ) && ( players[ index ]->type == NETWORK_CLIENT ) ) {
			written = write( players[ index ]->server_socket, message, message_length );
			if ( written < static_cast< ssize_t >( message_length ) ) {
				fprintf( stderr,
				         "%s:%d: Warning: Only %zd/%zu bytes sent to player %d\n",
				         __FILE__,
				         __LINE__,
				         written,
				         message_length,
				         index );
			}
		}
	} // done all players
#endif    // NETWORK
	return true;
}

/// @brief set new frames per second if valid and calculate dependent values.
void CEnvironment::set_fps( int32_t new_FPS ) {
	if ( !new_FPS || ( ( new_FPS > 0 ) && ( new_FPS != frames_per_second ) ) ) {
		if ( new_FPS ) {
			frames_per_second = new_FPS;
		}
		fps_mod     = 100. / static_cast< double >( frames_per_second );
		fall_vector = gravity * fps_mod;
		max_velocity = static_cast< double >( MAX_POWER ) * fps_mod / 100.;
	}
}

void CEnvironment::window_update( int32_t x, int32_t y, int32_t w, int32_t h ) {
	if ( x < window.x ) {
		window.x = x;
	}
	if ( y < window.y ) {
		window.y = y;
	}
	if ( x + w > window.w ) {
		window.w = ( x + w ) - 1;
	}
	if ( y + h > window.h ) {
		window.h = ( y + h ) - 1;
	}
	if ( window.x < 0 ) {
		window.x = 0;
	}
	if ( window.y < MENUHEIGHT ) {
		window.y = MENUHEIGHT;
	}
	if ( window.w > ( screen_width - 1 ) ) {
		window.w = ( screen_width - 1 );
	}
	if ( window.h > ( screen_height - 1 ) ) {
		window.h = ( screen_height - 1 );
	}
}
