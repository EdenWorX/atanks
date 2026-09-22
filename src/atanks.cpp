#define ATANKS_ATANKS_CPP 1

/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2002,2003  Thomas Hudson,Juraj Michalek
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

#include "box.h"
#include "button.h"
#include "clock.h"
#include "debug.h"
#include "files.h"
#include "gameloop.h"
#include "globals.h"
#include "item.h"
#include "optionscreens.h"
#include "player.h"
#include "random.h"
#include "score.h"
#include "tank.h"
#include "update.h"
#include "weapon.h"

#ifdef NETWORK
#  include "client.h"

#  include <thread>
#endif

#define HELP_REQUESTED     ( -100 )
#define SWITCH_HELP        "-h"
#define SWITCH_FULL_SCREEN "-fs"
#define SWITCH_WINDOWED    "--windowed"
// #define SWITCH_NOSOUND     "--nosound"
// #define SWITCH_DATADIR     "--datadir"
// #define SWITCH_CONFIGDIR   "-c"
// #define SWITCH_NO_CONFIG   "--noconfig"


/*****************************
*** static local variables ***
*****************************/
static bool        allow_network = true;
static string      full_path;
static EFullScreen full_screen      = FULL_SCREEN_EITHER;
static bool        load_config_file = true;
static int32_t     screen_mode      = GFX_AUTODETECT_WINDOWED;
#ifdef NETWORK
static int32_t client_socket = -1;
#endif // NETWORK


/*************************
*** External variables ***
*************************/
extern CWeapon weapon[ WEAPONS ];    // from files.cpp
extern CWeapon naturals[ NATURALS ]; // from files.cpp
extern CItem   item[ ITEMS ];        // from files.cpp


/*****************************
*** static local functions ***
*****************************/
static void        change_settings( bool old_sound, int32_t old_itech, int32_t old_wtech );
static void        close_button_handler();
static void        create_config();
static void        credits();
static char const* do_winner();
static void        endgame_cleanup();
void               init_mouse_cursor();
static void        init_game_settings();
static void        init_graphics_and_assets();
static void        initialise_players();
static bool        load_config();
static bool        load_players( FILE* file );
static int32_t     menu();
static void        new_game();
static int32_t     parse_args( int32_t argc, char** argv );
static void        play_demo();
static void        play_local();
static void        play_networked();
static void        print_text_help();
static void        print_text_init_msg();
static bool        save_game_settings( char const* path );
static void        show_options();
static void        title();


/*****************************
*** external functions     ***
*****************************/
void draw_simple_bg( bool drawImage ); // from shop.cpp

/*******************************
*** Function implementations ***
*******************************/

/** @brief Take care of changed settings.
 *
 * This function detects changes to some environment settings and, if a
 * change has happened, makes the required changes to the game environment.
 **/
static void change_settings( bool old_sound, int32_t old_itech, int32_t old_wtech ) {
	// first, check for a change in the sound settings
	if ( old_sound != env.sound_enabled ) {
		if ( env.sound_enabled ) {
			if ( detect_digi_driver( DIGI_AUTODETECT ) ) {
				if ( install_sound( DIGI_AUTODETECT, MIDI_NONE, nullptr ) < 0 ) {
					fprintf( stderr, "install_sound: failed turning on sound\n" );
				}
			} else {
				fprintf( stderr, "detect_digi_driver found no sound device\n" );
			}
		} else {
			remove_sound();
		}
	} // End of sound checking

	// Check for tech level changes
	if ( ( old_itech != env.itemtech_level ) || ( old_wtech != env.weapontech_level ) ) {
		env.gen_items_list();
	}
}

/** @brief Close Button Handler
 *
 * This function catches the close command, usually given by the user pressing
 * the close window button. We'll try to clean-up.
 **/
static void close_button_handler() {
	global.press_close_button();
}

/// @brief Show the credits file in a text box
static void credits() {
	string    credits_file{ env.data_dir + string( "/credits.txt" ) };

	TEXTBLOCK my_text( credits_file.c_str() );
	scroll_text_list( &my_text );
}

/// @brief create a fresh new config if loading was prohibited or failed
static void create_config() {
	env.num_permanent_players = 0;

	// Override full screen settings from command line
	if ( ( full_screen == FULL_SCREEN_TRUE ) || ( full_screen == FULL_SCREEN_FALSE ) ) {
		env.full_screen = full_screen;
	}

	// Determine basic screen settings
	env.temp_screen_width  = env.screen_width;
	env.temp_screen_height = env.screen_height;
	env.half_width         = env.screen_width / 2;
	env.half_height        = env.screen_height / 2;
	env.menu_begin_y        = ( env.screen_height - 400 ) / 2;
	if ( env.menu_begin_y < 0 ) {
		env.menu_begin_y = 0;
	}
	env.menu_end_y = env.screen_height - env.menu_begin_y;

	// Perform game initialization
	init_game_settings();
	env.load_text_files();
}

/// @brief Draw the endgame screen and return the winner name
/// or nullptr if no winner was found. The returned text is static
/// and must *NOT* be freed.
static char const* do_winner() {
	static char return_string[ 257 ] = { 0 };

	// Get the dimensions of the score board and the texts right:
	int32_t lh = env.font_height + 3; // The line height.
	int32_t pd = 10;                 // Padding. How much space to the board border.

	// Find out longest player name and score length do determine the
	// score board size and score entry positions
	char head_name[ 5 ]   = "Name";
	char head_value[ 6 ]  = "Value";
	char head_score[ 30 ] = { 0 };
	snprintf( head_score, 29, " %6s %6s %6s %6s", "Kills", "Killed", "Diff", "Won" );

	int32_t namLen = text_length( font, head_name );
	int32_t valLen = text_length( font, head_value );
	int32_t scoLen = text_length( font, head_score );

	// While checking for the winner, determine the real lengths needed
	int32_t  idx_jedi               = -1; // Jedi Player with the highest score
	int32_t  idx_neutral            = -1; // Neutral player with the highest score
	int32_t  idx_sith               = -1; // SitH Player with the highest score
	int32_t  idx_winner             = -1; // Player with the highest score
	int32_t  maxdiff                = INT32_MIN;
	int32_t  maxscore               = -1;
	int32_t  maxkills               = -1;
	int32_t  minkilled              = INT32_MAX;
	bool     multiwinner            = false;
	CPlayer** players                = env.players; // short cut
	int32_t  pl_money[ MAXPLAYERS ] = { 0 };

	for ( int32_t z = 0; z < env.num_game_players; z++ ) {

		// Check the length of the name
		int32_t curLen = text_length( font, players[ z ]->get_name() );
		if ( curLen > namLen ) {
			namLen = curLen;
		}

		// Sum up weapons worth
		for ( int32_t j = 0; j < WEAPONS; ++j ) {
			if ( weapon[ j ].amt && players[ z ]->nm[ j ] ) {
				pl_money[ z ] += ( weapon[ j ].cost / weapon[ j ].amt ) * players[ z ]->nm[ j ];
			}
		}

		// Sum up items worth
		for ( int32_t j = 0; j < ITEMS; ++j ) {
			if ( item[ j ].amt && players[ z ]->ni[ j ] ) {
				pl_money[ z ] += ( item[ j ].cost / item[ j ].amt ) * players[ z ]->ni[ j ];
			}
		}

		// Check value length
		char valTxt[ 32 ] = { 0 };
		snprintf( valTxt, 16, " %14s", add_comma( pl_money[ z ] ) );
		curLen = text_length( font, valTxt );
		if ( curLen > valLen ) {
			valLen = curLen;
		}

		// Check the length of the score
		char    scoTxt[ 30 ] = { 0 };
		int32_t kill_diff    = players[ z ]->kills - players[ z ]->killed;
		snprintf( scoTxt, 29, " %6d %6d %6d %6d", players[ z ]->kills, players[ z ]->killed, kill_diff, players[ z ]->score );
		curLen = text_length( font, scoTxt );
		if ( curLen > scoLen ) {
			scoLen = curLen;
		}

		// Determine whether a new winner or a draw situation is found
		if ( ( players[ z ]->score == maxscore ) && ( players[ z ]->kills == maxkills )
		     && ( players[ z ]->killed == minkilled ) ) {
			multiwinner = true;
			if ( TEAM_NEUTRAL == players[ z ]->team ) {
				idx_neutral = z;
			}
		} else if ( (players[z]->score > maxscore)
		         || ( (players[z]->score == maxscore)
		           && (kill_diff > maxdiff) )
		         || ( (players[z]->score == maxscore)
		           && (kill_diff == maxdiff)
		           && (players[z]->kills > maxkills) ) ) {
			// Note: killed doesn't need to be checked. the same
			// amount of kills with less killed score would mean
			// a better diff anyway.
			maxdiff     = kill_diff;
			maxkills    = players[ z ]->kills;
			maxscore    = players[ z ]->score;
			minkilled   = players[ z ]->killed;
			idx_winner  = z;
			multiwinner = false;
			if ( TEAM_NEUTRAL == players[ z ]->team ) {
				idx_neutral = z;
			}
		}

		if ( TEAM_JEDI == players[ z ]->team ) {
			idx_jedi = z;
		}
		if ( TEAM_SITH == players[ z ]->team ) {
			idx_sith = z;
		}
	} // end of checking players

	// Now calculate the dimensions of our score board.
	int32_t w  = namLen + valLen + scoLen + ( 2 * pd );
	int32_t h  = ( ( env.num_game_players + 4 ) * lh ) + ( 2 * pd );
	int32_t x  = env.half_width - ( w / 2 );
	int32_t y  = env.half_height - ( h / 2 );
	int32_t qy = y + h + pd;
	sBox     qarea( x + pd, qy, w - ( 2 * pd ), env.screen_height - pd - qy );

	// stop mouse during drawing
	SHOW_MOUSE( nullptr )

	global.make_update( x, y - pd - env.misc[ 9 ]->h, w, h + pd + env.misc[ 9 ]->h );

	// Draw the winning bitmap, the background and the border
	draw_simple_bg( false );
	draw_sprite( global.canvas, env.misc[ 9 ], env.half_width - ( env.misc[ 9 ]->w / 2 ), y - env.misc[ 9 ]->h - pd );
	rectfill( global.canvas, x, y, x + w, y + h, BLACK );
	rect( global.canvas, x, y, x + w, y + h, WHITE );
	rect( global.canvas, x + 1, y + 1, x + w - 1, y + h - 1, GREY );

	// add the padding now, or it must be summed in everywhere!
	x += pd;
	y += pd;
	w -= 2 * pd;
	h -= 2 * pd;

	// Draw winner names and info about all players
	if ( multiwinner ) {
		// check for team win
		if ( TEAM_JEDI == players[ idx_winner ]->team ) {
			if ( ( ( idx_sith >= 0 ) && ( players[ idx_sith ]->score == players[ idx_winner ]->score ) )
			     || ( ( idx_neutral >= 0 ) && ( players[ idx_neutral ]->score == players[ idx_winner ]->score ) ) ) {
				snprintf( return_string, 256, "%s", env.ingame->get_line( 48 ) );
			} else {
				snprintf( return_string, 256, "%s", env.ingame->get_line( 45 ) );
			}
		} else if ( TEAM_SITH == players[ idx_winner ]->team ) {
			if ( ( ( idx_jedi >= 0 ) && ( players[ idx_jedi ]->score == players[ idx_winner ]->score ) )
			     || ( ( idx_neutral >= 0 ) && ( players[ idx_neutral ]->score == players[ idx_winner ]->score ) ) ) {
				snprintf( return_string, 256, "%s", env.ingame->get_line( 48 ) );
			} else {
				snprintf( return_string, 256, "%s", env.ingame->get_line( 46 ) );
			}
		} else {
			snprintf( return_string, 256, "%s", env.ingame->get_line( 48 ) );
		}
	} else {
		if ( TEAM_JEDI == players[ idx_winner ]->team ) {
			snprintf( return_string, 256, "%s", env.ingame->get_line( 45 ) );
		} else if ( TEAM_SITH == players[ idx_winner ]->team ) {
			snprintf( return_string, 256, "%s", env.ingame->get_line( 46 ) );
		} else {
			snprintf( return_string, 256, "%s: %s", env.ingame->get_line( 47 ), players[ idx_winner ]->get_name() );
		}
	}

	// Print the title lines
	textprintf_centre_ex( global.canvas, font, env.half_width, y, players[ idx_winner ]->color, -1, "%s", return_string );

	// to make the following easier, skip the two used lines
	// (The title and one blank)
	y += 2 * lh;

	// Second title line, the score board header
	int32_t valStart = x + namLen;
	int32_t scoStart = valStart + valLen;
	int32_t scoWidth = scoLen / 4;

	textout_ex( global.canvas, font, "Name", x, y, WHITE, -1 );
	textprintf_right_ex( global.canvas, font, valStart + valLen, y, WHITE, -1, " %14s", "$ Value" );
	textprintf_right_ex( global.canvas, font, scoStart + ( 1 * scoWidth ), y, GREEN, -1, " %6s", "Kills" );
	textprintf_right_ex( global.canvas, font, scoStart + ( 2 * scoWidth ), y, RED, -1, " %6s", "Killed" );
	textprintf_right_ex( global.canvas, font, scoStart + ( 3 * scoWidth ), y, WHITE, -1, " %6s", "Diff" );
	textprintf_right_ex( global.canvas, font, scoStart + ( 4 * scoWidth ), y, WHITE, -1, " %6s", "Won" );

	// Now get the score list:
	sScore* score_array = sort_scores();

	// And get the head entry:
	sScore* score = score_array;
	while ( score->prev ) {
		score = score->prev;
	}


	// Eventually the player scores can be displayed:
	// (again skip the previous line for easier reading/doing below)
	y         += lh;
	int32_t z  = 0;
	while ( score ) {
		textout_ex( global.canvas, font, score->name, x, y + ( z * lh ), score->color, -1 );
		textprintf_right_ex(
			global.canvas,
			font,
			valStart + valLen,
			y + ( z * lh ),
			WHITE,
			-1,
			" %14s",
			add_comma( pl_money[ score->idx ] )
		);
		textprintf_right_ex( global.canvas, font, scoStart + ( 1 * scoWidth ), y + ( z * lh ), GREEN, -1, " %6d", score->kills );
		textprintf_right_ex( global.canvas, font, scoStart + ( 2 * scoWidth ), y + ( z * lh ), RED, -1, " %6d", score->killed );
		textprintf_right_ex(
			global.canvas,
			font,
			scoStart + ( 3 * scoWidth ),
			y + ( z * lh ),
			score->diff < 0 ? RED : GREEN,
			-1,
			" %6d",
			score->diff
		);
		textprintf_right_ex( global.canvas, font, scoStart + ( 4 * scoWidth ), y + ( z * lh ), WHITE, -1, " %6d", score->score );
		++z;
		score = score->next;
	}
	global.do_updates();

	// add a war quote:
	char const* quote = env.war_quotes->get_random_line();
	if ( quote ) {
		draw_text_in_box( &qarea, quote, false );
	}

	// Clean up
	delete[] score_array;

	return return_string;
}

static void endgame_cleanup() {
	while ( env.num_game_players > 0 ) {
		if ( env.players[ 0 ]->tank ) {
			delete env.players[ 0 ]->tank;
			env.players[ 0 ]->tank = nullptr;
		}

		// make sure networked clients say good-bye and return
		// to old AI level
		if ( env.players[ 0 ]->type >= NETWORK_CLIENT ) {
			env.players[ 0 ]->type = env.players[ 0 ]->previous_type;
		}

		env.remove_game_player( env.players[ 0 ] );
	}

	global.clear_objects();
}


#if defined( ATANKS_IS_MSVC ) && defined( ATANKS_DEBUG )
// this removes the keyboard and mouse handler on break events,
// hopefully making both operational in Visual Studio again if
// a crash was caught. It does not work on breakpoints though,
// that doesn't trigger the Handler.
BOOL WINAPI ctrlHandler( DWORD CtrlType ) {
	if ( ( CTRL_BREAK_EVENT == CtrlType ) || ( CTRL_C_EVENT == CtrlType ) ) {
		remove_mouse();
		remove_keyboard();
	}

	return 0;
}
#endif // Microsoft Visual C++ and Debug


void init_mouse_cursor() {
	if ( env.os_mouse ) {
		show_os_cursor( MOUSE_CURSOR_ARROW );
	} else {
		set_mouse_sprite( env.misc[ 0 ] );
		set_mouse_sprite_focus( 0, 0 );
	}
}

static void init_game_settings() {
	if ( env.full_screen == FULL_SCREEN_TRUE ) {
		env.os_mouse = false;
	}

	int32_t status = allegro_init();

	if ( status ) {
		fprintf( stderr, "Unable to start Allegro.\nStatus %d", status );
		exit( 1 );
	}

	// Be sure no vsync is used:
	char const* no_vsync = get_config_string( "graphics", "disable_vsync", "no" );
	if ( 0 != strcasecmp( "yes", no_vsync ) ) {
		set_config_string( "graphics", "disable_vsync", "yes" );
	}

	set_window_title( "Atomic Tanks V" VERSION );

	// Before we get started make sure, that if we are using full
	// screen mode, we have to ignore width and height settings.
	if ( env.full_screen == FULL_SCREEN_TRUE ) {
		status = get_desktop_resolution( &env.screen_width, &env.screen_height );
		if ( status < 0 ) {
			env.screen_width  = 800;
			env.screen_height = 600;
		}
		screen_mode = GFX_AUTODETECT_FULLSCREEN;
	}

	// check for X pressed on the window bar
	LOCK_FUNCTION( close_button_handler )
	set_close_button_callback( close_button_handler );

	// Ensure sane colour depth
	if ( !env.colour_depth ) {
		env.colour_depth = desktop_color_depth();
	}

	if ( ( env.colour_depth != 16 ) && ( env.colour_depth != 32 ) ) {
		env.colour_depth = 16;
	}
	set_color_depth( env.colour_depth );
}

/// @brief Set the graphics mode and load all assets needing it.
///
/// Must run after the configuration is known and the arsenal files have loaded successfully (see `main`), so that
/// load failures exit while almost nothing is allocated. Covers everything the old monolithic `init_game_settings`
/// did past Allegro core init and colour depth setup: graphics mode, `first_init`, bitmaps, sounds, fonts.
static void init_graphics_and_assets() {
	// Now the screen mode can be set
	if ( set_gfx_mode( screen_mode, env.screen_width, env.screen_height, 0, 0 ) < 0 ) {
		perror( "set_gfx_mode" );

		int32_t status = set_gfx_mode( screen_mode, 800, 600, 0, 0 );

		if ( status < 0 ) {
			exit( 1 );
		}
		env.screen_width  = 800;
		env.screen_height = 600;
	}
	enable_triple_buffer();

	env.half_width  = env.screen_width / 2;
	env.half_height = env.screen_height / 2;

#ifdef ATANKS_IS_MSVC
#  if defined( ATANKS_DEBUG )
	SetConsoleCtrlHandler( ctrlHandler, 1 );
#  endif // DEBUG
	if ( env.full_screen == FULL_SCREEN_TRUE ) {
		set_display_switch_mode( SWITCH_BACKAMNESIA );
	} else {
		set_display_switch_mode( SWITCH_BACKGROUND );
	}
#endif // ATANKS_IS_MSVC

	if ( install_keyboard() < 0 ) {
		perror( "install_keyboard failed" );
		exit( 1 );
	}

	if ( install_mouse() < 0 ) {
		perror( "install_mouse failed" );
	}

	// check to see if we want sound
	if ( env.sound_enabled ) {
		int32_t sound_type = DIGI_AUTODETECT;

#ifdef ATANKS_IS_LINUX
		switch ( env.sound_driver ) {
			case SD_OSS:
				sound_type = DIGI_OSS;
				break;
			case SD_ESD:
				sound_type = DIGI_ESD;
				break;
			case SD_ARTS:
				sound_type = DIGI_ARTS;
				break;
			case SD_ALSA:
				sound_type = DIGI_ALSA;
				break;
			case SD_JACK:
				sound_type = DIGI_JACK;
				break;
			default:
				sound_type = DIGI_AUTODETECT;
				break;
		}
#endif   // ATANKS_IS_LINUX

		int32_t channels = detect_digi_driver( sound_type );

		if ( !channels && ( DIGI_AUTODETECT != sound_type ) ) {
			sound_type = DIGI_AUTODETECT;
			channels   = detect_digi_driver( sound_type );
		}

		if ( channels ) {
			env.voices            = channels > 64 ? 32 : channels > 32 ? 16 : channels > 16 ? 8 : channels;

			int32_t snd_installed = -1;

			while ( ( env.voices > 1 ) && ( 0 > snd_installed ) ) {
				DEBUG_LOG( "Sound Init", "Reserving %d / %d voices", env.voices, channels )
				reserve_voices( env.voices, 0 );
				snd_installed = install_sound( sound_type, DIGI_NONE, nullptr );

				// Instead of failing directly, reduces voices first
				if ( -1 == snd_installed ) {
					DEBUG_LOG( "Sound Init", "Too many voices, halving...", 0 )
					env.voices /= 2;
				}
			}

			// Now display an error message if it was not possible to succeed
			if ( 0 > snd_installed ) {
				fprintf( stderr, "install_sound: failed initialising sound\n" );
				fprintf( stderr,
				         "Please try selecting a different Sound Driver"
				         " from the Options menu.\n" );
			} else {
				int32_t set_voices = get_mixer_voices();
				DEBUG_LOG( "Sound Init", "Mixer has %d voices", set_voices )

				if ( set_voices < env.voices ) {
					env.voices = set_voices;
				}

				// Set the mixer quality:
				int32_t mixq = get_mixer_quality();
				if ( mixq < 2 ) {
					DEBUG_LOG( "Sound Init", "Raising mixer quality from %d to 2", mixq )
					set_mixer_quality( 2 );
				}
			}
		} else {
			fprintf( stderr, "detect_digi_driver detected no sound device\n" );
		}
	} // End of sound initialization

	// Colour initialization, must be done here when allegro is initialized.
	BLACK       = makecol( 0x00, 0x00, 0x00 );
	BLUE        = makecol( 0x00, 0x00, 0xff );
	DARK_GREEN  = makecol( 0x00, 0x50, 0x00 );
	DARK_GREY   = makecol( 0x40, 0x40, 0x40 );
	DARK_RED    = makecol( 0x80, 0x00, 0x00 );
	GOLD        = makecol( 0xaf, 0xaf, 0x00 );
	GREY        = makecol( 0x80, 0x80, 0x80 );
	GREEN       = makecol( 0x00, 0xff, 0x00 );
	LIGHT_GREEN = makecol( 0x80, 0xff, 0x80 );
	LIME_GREEN  = makecol( 0xc8, 0xff, 0xc8 );
	ORANGE      = makecol( 0xfa, 0x96, 0x00 );
	PINK        = makecol( 0xff, 0x00, 0xff );
	PURPLE      = makecol( 0xc8, 0x00, 0xc8 );
	RED         = makecol( 0xff, 0x00, 0x00 );
	SILVER      = makecol( 0xc0, 0xc0, 0xc0 );
	TURQUOISE   = makecol( 0x96, 0xc8, 0xff );
	WHITE       = makecol( 0xff, 0xff, 0xff );
	YELLOW      = makecol( 0xff, 0xff, 0x00 );

	// Start preparing environment
	env.first_init();    // *MUST* be done before CGlobalData or
	global.first_init(); // max_screen_updates is not correct!

	// Prepare remaining environment
	clear_to_color( global.canvas, BLACK );
	env.load_bitmaps();
	title();
	env.load_sounds();

	init_mouse_cursor();

	env.load_fonts();

	env.window.x = 0;
	env.window.y = 0;
	env.window.w = 0;
	env.window.h = 0;

	for ( int32_t z = 0; z < env.max_screen_updates; z++ ) {
		global.updates[ z ].x = 0;
		global.updates[ z ].y = 0;
		global.updates[ z ].w = 0;
		global.updates[ z ].h = 0;
	}
}

static void initialise_players() {
	for ( int32_t z = 0; z < env.num_game_players; ++z ) {
		env.players[ z ]->money = env.start_money;
		env.players[ z ]->score = 0;
		if ( ( HUMAN_PLAYER != env.players[ z ]->type ) && ( PERPLAY_PREF == env.players[ z ]->pref_type ) ) {
			env.players[ z ]->generate_preferences();
		}
		env.players[ z ]->initialise( false );
		env.players[ z ]->type_saved = env.players[ z ]->type;
	}
}

static bool load_config() {
	bool result = false;

	full_path.assign( env.config_dir + string( "/atanks-config.txt" ) );

	if ( load_config_file ) {
		FILE* old_config_file = fopen( full_path.c_str(), "r" );

		if ( old_config_file ) {
			env.load_from_file( old_config_file );

			// over-ride full screen setting with command line
			if ( ( full_screen == FULL_SCREEN_TRUE ) || ( full_screen == FULL_SCREEN_FALSE ) ) {
				env.full_screen = full_screen;
			}

			// Initialize after loading
			init_game_settings();

			// Load texts first ...
			env.load_text_files();

			// ...then the players last
			result = load_players( old_config_file );

			fclose( old_config_file );
		}
	} // End of loading old config file

	return result;
}

static bool load_players( FILE* file ) {
	int32_t max_pl = env.num_permanent_players;

	if ( env.all_players ) {
		for ( int32_t i = 0; i < env.num_permanent_players; ++i ) {
			if ( env.all_players[ i ] ) {
				delete env.all_players[ i ];
			}
			env.all_players[ i ] = nullptr;
		}
		free( env.all_players );
		env.all_players = nullptr;
	}

	env.all_players = (CPlayer**)malloc( sizeof( CPlayer* ) * max_pl );

	if ( !env.all_players ) {
		fprintf( stderr, "%s:%d : Failed to allocate memory for all_players\n", __FILE__, __LINE__ );
		return false;
	}

	for ( int32_t i = 0; i < max_pl; ++i ) {
		env.all_players[ i ] = nullptr;
	}

	int32_t pl_count = 0;
	bool    status   = true;

	while ( status ) {
		CPlayer* player_new = nullptr;
		try {
			player_new = new CPlayer();
		} catch ( std::exception& e ) {
			fprintf( stderr, "%s:%d : Failed to allocate memory for player (%s)\n", __FILE__, __LINE__, e.what() );
			status = false;
		}

		if ( status ) {
			status = player_new->load_from_file( file );
		}

		if ( status ) {
			player_new->index            = pl_count;
			env.all_players[ pl_count++ ] = player_new;
			if ( pl_count == max_pl ) {
				max_pl               += 5;
				auto new_player_list  = (CPlayer**)realloc( env.all_players, sizeof( CPlayer* ) * max_pl );
				if ( new_player_list ) {
					env.all_players = new_player_list;
				}
				for ( int32_t i = pl_count; i < max_pl; ++i ) {
					env.all_players[ i ] = nullptr;
				}
			}
		} else {
			delete player_new;
		}
	} // end of while status

	env.num_permanent_players = pl_count;

	return true;
}

static int32_t menu() {
	int32_t result     = SIG_OK;
	int32_t shift_menu = env.half_height < 240 ? 240 - env.half_height : 0;
	int32_t move_btn   = env.button[ 0 ]->w / 2;
	int32_t bn         = env.language == EL_RUSSIAN ? MENUBUTTONS * 2 : 0;

	CButton  but_play(
                env.half_width - move_btn,
                env.half_height - 235 + shift_menu,
                env.button[ bn ],
                env.button[ bn ],
                env.button[ bn + 1 ]
        );
	bn += 2;
	CButton but_help(
		env.half_width - move_btn,
		env.half_height - 185 + shift_menu,
		env.button[ bn ],
		env.button[ bn ],
		env.button[ bn + 1 ]
	);
	bn += 2;
	CButton but_options(
		env.half_width - move_btn,
		env.half_height - 135 + shift_menu,
		env.button[ bn ],
		env.button[ bn ],
		env.button[ bn + 1 ]
	);
	bn += 2;
	CButton but_players(
		env.half_width - move_btn,
		env.half_height - 85 + shift_menu,
		env.button[ bn ],
		env.button[ bn ],
		env.button[ bn + 1 ]
	);
	bn += 2;
	CButton but_credits(
		env.half_width - move_btn,
		env.half_height - 35 + shift_menu,
		env.button[ bn ],
		env.button[ bn ],
		env.button[ bn + 1 ]
	);
	bn += 2;
	CButton but_quit(
		env.half_width - move_btn,
		env.half_height + 65 + shift_menu,
		env.button[ bn ],
		env.button[ bn ],
		env.button[ bn + 1 ]
	);
	bn += 2;
	CButton but_network(
		env.half_width - move_btn,
		env.half_height + 15 + shift_menu,
		env.button[ bn ],
		env.button[ bn ],
		env.button[ bn + 1 ]
	);

	CButton* button[ MENUBUTTONS ] = {
		&but_play, &but_help, &but_options, &but_players, &but_credits, &but_network, &but_quit
	};

	// Initialization of the menu
	global.stop_window    = true;
	fi                   = 1;
	lx                   = 0;
	ly                   = 0;
	k                    = 0;
	K                    = 0;

	bool    done         = false;
	int32_t seconds_idle = 0;
	int32_t btn_over     = -1;
	int32_t currentindex = 0;
	int32_t oldindex     = 0;
	int32_t maxindex     = MENUBUTTONS;
	int32_t lastmouse_x  = 0;
	int32_t lastmouse_y  = 0;

	// Clear key buffer and erase mouse button presses
	while ( keypressed() ) {
		readkey();
	}
	mouse_b = 0;

	// Enable first background drawing:
	bool need_draw = true;
	draw_simple_bg( true );
	global.make_full_update();

	while ( !done && ( SIG_OK == result ) ) {

		// Extra loop to divide the handling and the drawing
		while ( !done && !need_draw ) {
			// Count seconds for demo mode to start after its wait time
			if ( check_time_changed() ) {
				if ( ++seconds_idle > DEMO_WAIT_TIME ) {
					done = true;
					global.set_command( GLOBAL_COMMAND_DEMO );
				}
			}

			// Detect mouse movement for custom cursors
			if ( !env.os_mouse && ( ( lastmouse_x != mouse_x ) || ( lastmouse_y != mouse_y ) ) ) {
				lastmouse_x = mouse_x;
				lastmouse_y = mouse_y;
				need_draw   = true;
			}

			// See where the mouse is
			for ( int32_t z = 0; z < MENUBUTTONS; z++ ) {
				if ( button[ z ]->is_mouse_over() ) {
					if ( ( btn_over > -1 ) && ( btn_over != z ) ) {
						button[ z ]->draw();
						need_draw = true;
					}

					btn_over = z;
					break;
				}
			}

			// Handle mouse click
			if ( mouse_b & 1 ) {
				for ( int32_t z = 0; z < MENUBUTTONS; z++ ) {
					if ( button[ z ]->is_pressed() ) {
						need_draw = true;
						done      = true;
						if ( z == 0 ) {
							global.set_command( GLOBAL_COMMAND_PLAY );
						} else if ( z == 1 ) {
							global.set_command( GLOBAL_COMMAND_HELP );
						} else if ( z == 2 ) {
							global.set_command( GLOBAL_COMMAND_OPTIONS );
						} else if ( z == 3 ) {
							global.set_command( GLOBAL_COMMAND_PLAYERS );
						} else if ( z == 4 ) {
							global.set_command( GLOBAL_COMMAND_CREDITS );
						} else if ( z == 5 ) {
							global.set_command( GLOBAL_COMMAND_NETWORK );
						} else if ( z == 6 ) {
							global.set_command( GLOBAL_COMMAND_QUIT );
							result = SIG_QUIT_GAME;
						}
					}
				}
			} // End of mouse button pressed

			// check for key press
			if ( keypressed() ) {
				k  = readkey();
				K  = k >> 8;
				fi = 2;
			}

			// Move selection down
			if ( ( K == KEY_DOWN ) || ( K == KEY_S ) ) {
				if ( ++currentindex >= maxindex ) {
					currentindex = 0;
				}
				need_draw = true;
			}

			// Move selection up
			else if ( ( K == KEY_UP ) || ( K == KEY_W ) ) {
				if ( --currentindex < 0 ) {
					currentindex = maxindex - 1;
				}
				need_draw = true;
			}

			// Activate selection
			else if ( ( KEY_ENTER == K ) || ( KEY_ENTER_PAD == K ) || ( KEY_SPACE == K ) ) {
				need_draw = true;
				done      = true;
				if ( currentindex == 0 ) {
					global.set_command( GLOBAL_COMMAND_PLAY );
				} else if ( currentindex == 1 ) {
					global.set_command( GLOBAL_COMMAND_HELP );
				} else if ( currentindex == 2 ) {
					global.set_command( GLOBAL_COMMAND_OPTIONS );
				} else if ( currentindex == 3 ) {
					global.set_command( GLOBAL_COMMAND_PLAYERS );
				} else if ( currentindex == 4 ) {
					global.set_command( GLOBAL_COMMAND_CREDITS );
				} else if ( currentindex == 5 ) {
					global.set_command( GLOBAL_COMMAND_NETWORK );
				} else if ( currentindex == 6 ) {
					global.set_command( GLOBAL_COMMAND_QUIT );
				}
			}

			// Quick keys to exit and handle close button of the window
			else if ( ( KEY_Q == K ) || ( KEY_ESC == K ) ) {
				done   = true;
				result = SIG_QUIT_GAME;
			}

			// erase key presses
			K = 0;

			// Print out update info if any
			if ( ( global.update_string ) && ( global.update_string[ 0 ] ) ) {
				textout_centre_ex(
					global.canvas,
					font,
					global.update_string,
					env.half_width - 20,
					env.screen_height - 50,
					WHITE,
					-1
				);
				global.make_update( 50, 450, 300, 50 );
				need_draw = true;
			}

			// Print out client messages
			if ( global.client_message ) {
				textout_centre_ex(
					global.canvas,
					font,
					global.client_message,
					env.half_width - 20,
					env.screen_height - 25,
					WHITE,
					-1
				);
				global.make_update( 50, 450, 300, 100 );
				need_draw = true;
			}

			// Sleep a bit if nothing happened
			if ( !done && !need_draw ) {
				LINUX_SLEEP;
			}
		} // End of while not needing to draw


		// flip to front if needed
		if ( need_draw ) {

			// Draw the buttons
			SHOW_MOUSE( nullptr )
			draw_simple_bg( true );

			for ( int32_t z = 0; z < MENUBUTTONS; z++ ) {
				button[ z ]->draw();

				// draw a rectangle around the selected button
				if ( ( z == currentindex ) || ( z == oldindex ) ) {
					int32_t left   = env.half_width - move_btn - 6;
					int32_t top    = env.half_height - 241 + ( 50 * currentindex ) + shift_menu;
					int32_t right  = env.half_width + move_btn + 5;
					int32_t bottom = env.half_height - 192 + ( 50 * currentindex ) + shift_menu;
					global.make_update( left, top, right - left, bottom - top );
					if ( z == currentindex ) {
						rect( global.canvas, left, top, right, bottom, YELLOW );
					}
				}
			} // end of looping buttons

			// Show non-OS mouse
			SHOW_MOUSE( global.canvas )

			global.do_updates();
			need_draw = false;
			oldindex  = currentindex;
		} // End of if need_draw
	}         // End of menu loop

	clear_keybuf();

	return result;
}

static void new_game() {
	env.initialise();
	global.initialise();

	// if a game should be loaded, try it or deny loading of the game
	if ( ( env.load_game ) && ( !load_game() ) ) {
		env.load_game = false;
	}

	// Now check back whether to load a game
	if ( !env.load_game ) {
		initialise_players();
	}

	// There must not be any tanks!
	CTank* tank      = nullptr;
	CTank* next_tank = nullptr;
	global.get_head_of_class( CLASS_TANK, &tank );
	while ( tank ) {
		tank->get_next( &next_tank );
		tank->player = nullptr;
		delete tank;
		tank = next_tank;
	}

	// This is always true here, as a newly started game is handled like a loaded one:
	env.is_game_loaded = true;
}

/// @brief parse arguments and return EXIT_SUCCESS on success or EXIT_FAILURE
/// if anything went wrong. If all is well but help was requested, return
/// EXIT_HELP_SHOWN
static int32_t parse_args( int32_t argc, char** argv ) {
	for ( int32_t c = 1; c < argc; ++c ) {
		bool        has_value = true;
		std::string arg( argv[ c ] );

		if ( ( arg == SWITCH_HELP ) || ( arg == "--help" ) ) {
			print_text_help();
			return HELP_REQUESTED;
		} else if ( arg == SWITCH_FULL_SCREEN ) {
			screen_mode = GFX_AUTODETECT_FULLSCREEN;
			full_screen = FULL_SCREEN_TRUE;
		} else if ( arg == SWITCH_WINDOWED ) {
			screen_mode = GFX_AUTODETECT_WINDOWED;
			full_screen = FULL_SCREEN_FALSE;
		} else if ( ( arg == "-d" ) || ( arg == "--depth" ) ) {
			if ( ( c < ( argc - 1 ) ) && ( argv[ c + 1 ][ 0 ] != '-' ) ) {
				std::string next_arg( argv[ ++c ] );
				int32_t     val = 32;

				SAFE_STOI( val, next_arg );

				if ( ( 16 == val ) || ( 32 == val ) ) {
					env.colour_depth = val;
				} else {
					cerr << "ERROR: Invalid graphics depth!\n"
					     << "       Only 16 or 32 bit are supported!" << endl;
					return EXIT_FAILURE;
				}
			} else {
				has_value = false;
			}
		} else if ( ( arg == "-w" ) || ( arg == "--width" ) ) {
			if ( ( c < ( argc - 1 ) ) && ( argv[ c + 1 ][ 0 ] != '-' ) ) {
				std::string next_arg( argv[ ++c ] );
				int32_t     val = 512;

				SAFE_STOI( val, next_arg );

				if ( 512 <= val ) {
					env.screen_width      = val;
					env.half_width        = env.screen_width / 2;
					env.temp_screen_width = env.screen_width;
				} else {
					cerr << "ERROR: Width too small (minimum 512)\n" << endl;
					return EXIT_FAILURE;
				}
			} else {
				has_value = false;
			}
		} else if ( ( arg == "-t" ) || ( arg == "--tall" ) || ( arg == "--height" ) ) {
			if ( ( c < ( argc - 1 ) ) && ( argv[ c + 1 ][ 0 ] != '-' ) ) {
				std::string next_arg( argv[ ++c ] );
				int32_t     val = 320;

				SAFE_STOI( val, next_arg );

				if ( 320 <= val ) {
					env.screen_height      = val;
					env.half_height        = env.screen_height / 2;
					env.temp_screen_height = env.screen_height;
				} else {
					cerr << "ERROR: Height too small (minimum 320)\n" << endl;
					return EXIT_FAILURE;
				}
			} else {
				has_value = false;
			}
		} else if ( arg == "--datadir" ) {
			if ( ( c < ( argc - 1 ) ) && ( argv[ c + 1 ][ 0 ] != '-' ) ) {
				std::string next_arg( argv[ ++c ] );

				if ( next_arg.length() <= PATH_MAX ) {
					env.data_dir = next_arg;
				} else {
					cerr << "ERROR: Datadir path too long:\n"
					     << "\"" << next_arg << "\"\n\n"
					     << "Maximum length:" << PATH_MAX << " characters" << endl;
					return EXIT_FAILURE;
				}
			} else {
				has_value = false;
			}
		} else if ( arg == "-c" ) {
			if ( ( c < ( argc - 1 ) ) && ( argv[ c + 1 ][ 0 ] != '-' ) ) {
				std::string next_arg( argv[ ++c ] );

				if ( next_arg.length() <= PATH_MAX ) {
					env.config_dir = next_arg;
				} else {
					cerr << "ERROR: Configuration path too long:\n"
					     << "\"" << next_arg << "\"\n\n"
					     << "Maximum length:" << PATH_MAX << " characters" << endl;
					return EXIT_FAILURE;
				}
			} else {
				has_value = false;
			}
		} else if ( arg == "--noconfig" ) {
			load_config_file = false;
		} else if ( arg == "--nosound" ) {
			env.sound_enabled = false;
		} else if ( arg == "--noname" ) {
			env.name_above_tank = false;
		} else if ( arg == "--nonetwork" ) {
			allow_network = false;
		} else if ( arg == "--nobackground" ) {
			env.draw_background = false;
		} else if ( arg == "--nothread" ) {
			cout << "--nothread is deprecated and will be ignored." << endl;
		} else if ( arg == "--thread" ) {
			cout << "--thread is deprecated and will be ignored." << endl;
		}

		// If a required argument is missing, print out a message
		if ( !has_value ) {
			cerr << "ERROR: Missing argument for " << arg << endl;
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

static void play_demo() {
	int32_t  old_skip   = env.skip_computer_play;
	uint32_t old_rounds = env.rounds;
	bool     old_music  = env.play_music;

	global.demo_mode    = true;
	env.load_game        = false;
	env.play_music      = false;

	env.rounds          = ( get_rand() % 101 ) + ( get_rand() % 101 ) + 50;
	global.current_round = env.rounds - ( get_rand() % env.rounds );

	// Be sure to have at least 10 rounds left
	if ( global.current_round < 10 ) {
		global.current_round = 10;
	}

	// And at least 10 rounds must have been played, or it'll be a bit boring
	if ( global.current_round > ( env.rounds - 10 ) ) {
		global.current_round = env.rounds - 10;
	}

	env.skip_computer_play = SKIP_NONE;

	// set up a bunch of players (non-human, less than 10)
	env.num_game_players = 0;
	for ( int32_t i = 0; i < env.num_permanent_players; ++i ) {
		if ( ( env.all_players[ i ]->type > HUMAN_PLAYER ) && ( i < MAXPLAYERS ) ) {
			env.add_game_player( env.all_players[ i ] );
		}
	}

	new_game();

	for ( int32_t i = 0; i < env.num_game_players; ++i ) {
		env.players[ i ]->new_game();

		// give them money to spend:
		env.players[ i ]->money +=
			static_cast< int32_t >( env.players[ i ]->type * 25000 * ( env.rounds - global.current_round ) );
	}

	while ( ( global.current_round > 0 ) && ( !global.is_close_btn_pressed() ) ) {
		game();
		if ( ( global.get_command() == GLOBAL_COMMAND_QUIT ) || ( global.get_command() == GLOBAL_COMMAND_MENU ) ) {
			break;
		}
	}

	endgame_cleanup();
	global.demo_mode     = false;
	env.skip_computer_play = old_skip;
	env.play_music       = old_music;
	env.rounds           = old_rounds;
}

static void play_local() {
	if ( select_players() != MRC_Esc_Menu ) {

		// make sure the game has a name
		if ( env.game_name.empty() ) {
			env.game_name.assign( env.ingame->get_line( 53 ) );
		}

		new_game();

		if ( !env.load_game ) {
			global.current_round = env.rounds;
			for ( int32_t i = 0; i < env.num_game_players; ++i ) {
				env.players[ i ]->new_game();
			}
		}

		// play the game for the selected number of rounds
		while ( ( global.current_round > 0 ) && ( !global.is_close_btn_pressed() ) ) {
			game(); // play a round

			if ( env.background_music ) {
				stop_sample( env.background_music );
			}

			if ( global.is_close_btn_pressed() ) {
				global.set_command( GLOBAL_COMMAND_QUIT );
			}

			// if user selected to quit or return to main menu during game play
			if ( ( global.get_command() == GLOBAL_COMMAND_QUIT )
			     || ( global.get_command() == GLOBAL_COMMAND_MENU ) ) {
				env.send_to_clients( "CLOSE" );
				break;
			}

			if ( global.current_round != 0 ) {
				// end of the round
				env.send_to_clients( "ROUNDEND" );
			}
		}

		// only show winner if finished all rounds and not broken off the last
		// round by exiting or quitting
		if ( ( global.current_round == 0 ) && ( global.get_command() == GLOBAL_COMMAND_PLAY ) ) {
			char        buffer[ 512 ] = { 0 };
			char const* winner        = do_winner();

			if ( winner ) {
				if ( 0 > snprintf( buffer, 255, "GAMEEND The game went to %s.", winner ) ) {
					abort();
				}
			} else {
				strncpy( buffer, "GAMEEND", 255 );
			}

			env.send_to_clients( buffer );

			// Do fade and wait for user keypress
			quick_change( true );
			readkey();

			for ( int i = 0; i < env.num_game_players; i++ ) {
				env.players[ i ]->type = env.players[ i ]->type_saved;
			}
		}
		endgame_cleanup();
	} // end of start new game
}

static void play_networked() {
#ifdef NETWORK
	client_socket = setup_client_socket( env.server_name, env.server_port );
	if ( client_socket >= 0 ) {
		bool keep_playing = true;
		cout << "Ready to play networked" << endl;

		while ( keep_playing ) {
			keep_playing = Game_Client( client_socket );
		}

		clean_up_client_socket( client_socket );
	} else {
		cerr << "ERROR: Unable to connect to server " << env.server_name << ", port " << env.server_port << endl;
	}
#else
	char noNetworkMsg[ 200 ] = { 0 };
	snprintf(
		noNetworkMsg,
		199,
		"This version of Atanks is not compiled to"
		" handle network games."
	);
	errorMessage = noNetworkMsg;
	errorX       = env.half_width - text_length( font, errorMessage ) / 2;
	errorY       = env.menu_begin_y + 15;
	cerr << "ERROR: " << noNetworkMsg << endl;
#endif
}

static void print_text_help() {
	cout << "-h  --help           Show this help screen\n"
	     << "-fs                  Full screen\n"
	     << "    --windowed       Run in a window\n"
	     << "-w  --width <width>  Specify the screen width in pixels\n"
	     << "-t  --tall  <height> Specify the screen height in pixels\n"
	     << "                     Adjust the screen size at your own risk\n"
	     << "                     (default is 800x600)\n"
	     << "-d  --depth <depth>  Colour depths, currently either 16 or 32 bit\n"
	     << "    --datadir <path> Path to the data directory\n"
	     << "-c <path>            Path to config and saved game directory\n"
	     << "    --noconfig       Do not load game settings.\n"
	     << "    --nosound        Disable sound\n"
	     << "    --noname         Do not show player name above tank\n"
	     << "    --nonetwork      Disable network connections.\n"
	     << "    --nobackground   Do not display the green menu background." << endl;
}

static void print_text_init_msg() {
	cout << "Atomic Tanks Version " << VERSION << " (-h for help)\n"
	     << "Authors: Tom Hudson        (rewrite, additions, improvements)\n"
	     << "         Stevante Software (original design)\n"
	     << "         Kota543 Software  (fixes and updates)\n"
	     << "         Jesse Smith       (additions, fixes and updates)\n"
	     << "         Sven Eden         (ai rewrite, additions, fixes and updates)\n\n"
	     << endl;
}

/*
This function calls the functions which save data to a text file.
The function requires the global data, environment and the path to
the config file name.
The function returns true on success and false on failure.
-- Jesse
*/
static bool save_game_settings( char const* path ) {
	FILE* file = fopen( path, "w" );
	if ( !file ) {
		perror( "Error trying to open text file for writing.\n" );
		return false;
	}

	env.save_to_file( file );

	for ( int32_t i = 0; i < env.num_permanent_players; ++i ) {
		env.all_players[ i ]->save_to_file( file );
	}

	fclose( file );
	return true;
}

static void show_options() {
	// save old settings
	bool    temp_sound = env.sound_enabled;
	int32_t temp_itech = env.itemtech_level;
	int32_t temp_wtech = env.weapontech_level;

	options_menu();

	if ( !save_game_settings( full_path.c_str() ) ) {
		cerr << "atanks.cpp:" << __LINE__ << " Failed to save game settings from " << __FUNCTION__ << endl;
	}

	// check for changes to settings
	change_settings( temp_sound, temp_itech, temp_wtech );
}

static void title() {
	SHOW_MOUSE( nullptr )
	blit( env.title[ 0 ],
	      screen,
	      0,
	      0,
	      env.half_width - ( env.title[ 0 ]->w / 2 ),
	      env.half_height - ( env.title[ 0 ]->h / 2 ),
	      env.title[ 0 ]->w,
	      env.title[ 0 ]->h );
	clear_keybuf();
}

int32_t main( int32_t argc, char** argv ) {
	print_text_init_msg();

	// Parse arguments and exit early if needed
	int32_t result = parse_args( argc, argv );

	if ( EXIT_FAILURE == result ) {
		return EXIT_FAILURE;
	}
	if ( HELP_REQUESTED == result ) {
		return EXIT_SUCCESS;
	}

	// try to find data dir
	if ( !env.find_data_dir() ) {
		cerr << "ERROR: Could not find data dir." << endl;
		return EXIT_FAILURE;
	}

	// Set the game version global
#ifdef VERSION
	{
		double this_version = strtod( VERSION, nullptr );
		game_version        = static_cast< int32_t >( this_version * 10 );
	}
#endif // VERSION

	// try to find config dir
	env.find_config_dir();

	// load or create a configuration
	bool fresh_config = false;
	if ( !load_config() ) {
		create_config();
		fresh_config = true;
	}

	// Load game files
	if ( !env.load_game_files() ) {
		return EXIT_FAILURE; // message already out
	}

	// Initialize graphics and load all assets needing them. Runs only after the fallible loads above succeeded, so
	// load failures exit while almost nothing is allocated (see TODO-II-1).
	init_graphics_and_assets();

	if ( fresh_config ) {
		// First run: create one human player plus the default AI set. Needs the graphics assets (misc bitmaps) for
		// the player editor, so it must run after the initialization above.
		create_human_player();
		create_ai_players();
	}


#ifdef NETWORK /// new networking area
	sSendReceive* send_receive   = nullptr;
	std::thread*       network_thread = nullptr;

#if 0 // legacy SourceForge update checker disabled; a modern replacement is planned (see TODO_Xtra.md)
	// Create the update checker thread:
	UpdateData updateData( "projects.sourceforge.net", "version.txt", "atanks.sourceforge.net" );

	std::thread updateThread( std::ref( updateData ) );
	if ( env.check_for_updates ) {
		global.update_string = updateData.update_string;
	}
#endif // 0

	// Initialize network if allowed and wanted
	if ( env.network_enabled && allow_network ) {
		send_receive = (sSendReceive*)calloc( 1, sizeof( sSendReceive ) );
		if ( !send_receive ) {
			cerr << "ERROR: Could not create networking data." << endl;
		}
	}

	// If a sSendReceive instance was created, start the networking thread
	if ( send_receive ) {
		send_receive->listening_port = env.network_port;

		// quit option already cleared by calloc call
		network_thread = new std::thread( send_and_receive, send_receive );
	}
#endif // NETWORK

	/* ===============================
	 * === The real main main loop ===
	 * ===============================
	 */
	do {

		// show the main menu
		global.set_command( GLOBAL_COMMAND_MENU );
		int32_t signal = menu();

		// Ensure a clean client message
		if ( global.client_message ) {
			free( const_cast< char* >( global.client_message ) );
			global.client_message = nullptr;
		}

		// did the user signal to quit the game?
		if ( ( signal == SIG_QUIT_GAME ) || global.is_close_btn_pressed() ) {
			global.set_command( GLOBAL_COMMAND_QUIT );
		}

		// determine which menu item is selected
		switch ( global.get_command() ) {
			case GLOBAL_COMMAND_HELP:
				scroll_text_list( env.instructions );
				break;
			case GLOBAL_COMMAND_OPTIONS:
				show_options();
				break;
			case GLOBAL_COMMAND_PLAYERS:
				edit_players();
				break;
			case GLOBAL_COMMAND_CREDITS:
				credits();
				break;
			case GLOBAL_COMMAND_QUIT:
				// Already handled by while condition below
				break;
			case GLOBAL_COMMAND_NETWORK:
				play_networked();
				break;
			case GLOBAL_COMMAND_DEMO:
				play_demo();
				break;
			default:
				// must have commanded to play game
				play_local();
				break;
		} // end of menu switch
	} while ( ( global.get_command() != GLOBAL_COMMAND_QUIT ) );

	// print out if there is an update
	if ( ( global.update_string ) && ( global.update_string[ 0 ] ) ) {
		cout << global.update_string << endl;
		global.update_string = nullptr;
	}

	// Clean up network stuff
#ifdef NETWORK
	if ( send_receive ) {
		send_receive->shut_down = true;
		LINUX_REST;
		network_thread->join();
		delete network_thread;
		free( send_receive );
	}
#if 0 // legacy SourceForge update checker disabled (see TODO_Xtra.md)
	updateThread.join();
#endif // 0
#endif // NETWORK

	if ( !save_game_settings( full_path.c_str() ) ) {
		// This is a very critical issue, but as we are ending here, we just report it
		cerr << "atanks.cpp: Failed to save game settings from atanks::main()!" << endl;
		result = EXIT_FAILURE;
	}

	env.destroy();
	global.destroy();

	allegro_exit();

#if 0 // legacy SourceForge farewell URL disabled (see TODO_Xtra.md)
	cout << "See https://atanks.sourceforge.io for the latest news and downloads." << endl;
#endif // 0

	return result;
}
END_OF_MAIN()
