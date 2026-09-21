#include "main.h"

// basically, all UNIX-like OSes should use stat
#ifndef WIN32
#  include <sys/stat.h>
#endif

#include "files.h"
#include "item.h"
#include "optionscreens.h"
#include "player.h"
#include "random.h"
#include "text.h"
#include "weapon.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <toml++/toml.hpp>


// They are filled here, declaring them here prevents the linker
// from 'optimizing' them away.
CWeapon weapon[ WEAPONS ];
CWeapon naturals[ NATURALS ];
CItem   item[ ITEMS ];

/** @brief Save the current game in progress
 * This function saves the game in progress.
 * All data is saved in a text file for flexibility.
 * @return true on success and false on failure.
 **/
bool save_game() {
	string save_path{ env.config_dir + string( "/" ).append( env.game_name ).append( ".sav" ) };

	FILE*  game_file = fopen( save_path.c_str(), "w" );
	if ( !game_file ) {
		return false;
	}

	// write file version information
	fprintf( game_file, "VERSION\n" );
	fprintf( game_file, "FILE_VERSION=%d\n", game_version );
	fprintf( game_file, "***\n" );

	// write global data
	fprintf( game_file, "GLOBAL\n" );
	fprintf( game_file, "CURRENTROUND=%d\n", global.current_round + 1 );
	// Note: When the game is saved, the round already has been decreased. Thus,
	// to not lose rounds when saving a game, returning to main and load it
	// again, this has to be increased here.
	fprintf( game_file, "SCOREBOARD=%d\n", global.show_score_board ? 1 : 0 );
	fprintf( game_file, "***\n" );

	// write environment data
	fprintf( game_file, "CEnvironment\n" );
	fprintf( game_file, "CAMPAIGNMODE=%d\n", env.campaign_mode ? 1 : 0 );
	fprintf( game_file, "CAMPAIGNROUNDS=%lf\n", env.campaign_rounds );
	fprintf( game_file, "NEXTCAMPROUND=%lf\n", env.next_campaign_round );
	fprintf( game_file, "ROUNDS=%u\n", env.rounds );
	fprintf( game_file, "***\n" );

	// write player data
	fprintf( game_file, "PLAYERS\n" );
	for ( int32_t i = 0; i < env.num_game_players; ++i ) {
		CPlayer* my_player = env.players[ i ];

		if ( my_player->index > -1 ) {
			// Note: This line is needed to know which player to load
			fprintf( game_file, "PLAYERNUMBER=%d\n", my_player->index );
			my_player->save_game_data( game_file );
		}
	}

	fprintf( game_file, "***EOF***\n" );

	fclose( game_file );

	/* Atanks always saved the current configuration in a file
	 * alongside the save game, but this environment file was
	 * never read. It makes no sense anyway, as it would change
	 * the current settings without re-loading the main config
	 * file.
	 * However, I find this exceptional for testing reasons, and
	 * if a game becomes boring, why not allow the player to
	 * enable some weather effects before loading a game?
	 * So it won't be added that the environment file is loaded,
	 * but if an old one is lying around, we should delete our
	 * own garbage:
	 */
	save_path.replace( save_path.size() - 3, 3, "txt" );
	if ( !access( save_path.c_str(), F_OK ) && !access( save_path.c_str(), W_OK ) ) {
		unlink( save_path.c_str() );
	}

	return true;
}

/*
This function reads one full line from a config/savegame file.
Unlike fgets() into a fixed buffer, it grows past any length, so long lines
are no longer split into broken pieces. Trailing newlines are stripped.
Returns false on end of file (nothing read), true otherwise.
-- EdenWorX
*/
bool read_config_line( FILE* file, string& line ) {
	line.clear();

	char chunk[ 256 ] = { 0 };
	bool got_data     = false;

	while ( fgets( chunk, sizeof( chunk ), file ) ) {
		got_data = true;
		line.append( chunk );
		if ( '\n' == line.back() ) {
			break; // full line read
		}
	}

	if ( !got_data ) {
		return false;
	}

	// strip newline characters like the old parsing did
	while ( !line.empty() && ( '\n' == line.back() ) ) {
		line.pop_back();
	}

	return true;
}

/*
This function splits a config line into field and value at the first '=',
searching from index 1 like the old parsing did.
Returns false if there is no '=' at position >= 1, true otherwise.
-- EdenWorX
*/
bool split_config_field( const string& line, string& field, string& value ) {
	size_t const equal_position = line.find( '=', 1 );
	if ( string::npos == equal_position ) {
		return false;
	}

	field = line.substr( 0, equal_position );
	value = line.substr( equal_position + 1 );
	return true;
}

/*
This function attempts to load a saved
game.
The function returns true on success and
false if an error occurs.
-- Jesse
*/
bool load_game() {
	string  line;
	string  field;
	string  value;
	int32_t player_count                 = 0;
	int32_t line_num                     = 0;
	int32_t player_idx                   = -1;
	bool    done                         = false;
	int32_t file_version                 = 0;
	bool    result;

	// Be sure that numbers are understood right:
	char const* cur_lc_numeric = setlocale( LC_NUMERIC, "C" );

	// Ensure backward compatibility:
	env.next_campaign_round = -1.;
	env.campaign_rounds   = -1.;

	// Open game file
	string save_path{ env.config_dir + string( "/" ).append( env.game_name ).append( ".sav" ) };
	FILE*  game_file = fopen( save_path.c_str(), "r" );
	if ( !game_file ) {
		return false;
	}

	// Now read until the file is finished loading
	ESaveGameStage stage = SGS_NONE;
	do {
		// read a line
		if ( ( result = read_config_line( game_file, line ) ) ) {
			++line_num;

			// if we hit end of the file, stop
			if ( !strncmp( line.c_str(), "***EOF***", 9 ) ) {
				done = true;
				continue; // This exits the loop as well
			}

			// check to see if we found a new stage
			if ( !strcasecmp( line.c_str(), "GLOBAL" ) ) {
				stage = SGS_GLOBAL;
			} else if ( !strcasecmp( line.c_str(), "CEnvironment" ) ) {
				stage = SGS_ENVIRONMENT;
			} else if ( !strcasecmp( line.c_str(), "PLAYERS" ) ) {
				// Here the file version must be known, or it is not set.
				// Inform the user if an upgrade is needed
				if ( game_version > file_version ) {
					fprintf( stdout,
					         "Game \"%s\" needs to be upgraded"
					         " from version %2.1f to version %2.1f\n",
					         env.game_name.c_str(),
					         static_cast< float >( file_version ) / 10.0,
					         static_cast< float >( game_version ) / 10.0 );
				}

				stage = SGS_PLAYERS;
			} else if ( !strcasecmp( line.c_str(), "VERSION" ) ) {
				stage = SGS_VERSION;
			} else {
				// Not a new stage, keep loading: separate field from value.
				if ( !split_config_field( line, field, value ) ) {
					continue; // Go to next line
				}

				switch ( stage ) {
					case SGS_ENVIRONMENT:
						if ( !strcasecmp( field.c_str(), "CAMPAIGNMODE" ) ) {
							int32_t cm = 0;
							SAFE_STOI( cm, value );
							env.campaign_mode = 0 != cm;
						} else if ( !strcasecmp( field.c_str(), "CAMPAIGNROUNDS" ) ) {
							SAFE_STOD( env.campaign_rounds, value );
						} else if ( !strcasecmp( field.c_str(), "ROUNDS" ) ) {
							SAFE_STOUL( env.rounds, value );
						} else if ( !strcasecmp( field.c_str(), "NEXTCAMPROUND" ) ) {
							SAFE_STOD( env.next_campaign_round, value );
						} else {
							cerr << save_path << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, it does not belong to ENV" << endl;
						}

						break;
					case SGS_GLOBAL:
						if ( !strcasecmp( field.c_str(), "CURRENTROUND" ) ) {
							SAFE_STOUL( global.current_round, value );
						}

						// The following are kept for backwards compatibility:

						else if ( !strcasecmp( field.c_str(), "NEXTCAMPROUND" ) ) {
							SAFE_STOD( env.next_campaign_round, value );
						} else if ( !strcasecmp( field.c_str(), "CAMPAIGNMODE" ) ) {
							int32_t cm = 0;
							SAFE_STOI( cm, value );
							env.campaign_mode = 0 != cm;
						} else if ( !strcasecmp( field.c_str(), "ROUNDS" ) ) {
							SAFE_STOUL( env.rounds, value );
						} else if ( !strcasecmp( field.c_str(), "SCOREBOARD" ) ) {
							int32_t enabled = 1;
							SAFE_STOI( enabled, value );
							global.show_score_board = 0 != enabled;
						} else {
							cerr << save_path << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, it does not belong to GLOBAL" << endl;
						}

						break;
					case SGS_PLAYERS:
						if ( !strcasecmp( field.c_str(), "PLAYERNUMBER" ) ) {
							SAFE_STOI( player_idx, value );
							if ( ( player_idx > -1 ) && ( player_idx < env.num_permanent_players )
							     && ( player_count < MAXPLAYERS ) ) {
								env.add_game_player( env.all_players[ player_idx ] );
								env.players[ player_count++ ]->initialise( true );
							} else {
								player_idx = -1;
							}
						}

						// Now let the player load itself
						if ( player_idx > -1 ) {
							env.all_players[ player_idx ]->load_game_data( game_file, file_version );
						} else {
							cerr << save_path << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, as player idx is " << player_idx << endl;
						}

						break;
					case SGS_VERSION:
						if ( !strcasecmp( field.c_str(), "FILE_VERSION" ) ) {
							SAFE_STOI( file_version, value );
						} else {
							cerr << save_path << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, it does not belong to VERSION" << endl;
						}

						break;
					case SGS_NONE:
					default:
						cerr << save_path << ":" << line_num << " : Wrong line\n";
						cerr << "The line \"" << line << "\"";
						cerr << " does not belong  to any stage!" << endl;
						break;
				} // end of switching stage
			}         // End of loading line / player record
		}                 // end of having read a line
	} while ( result && !done );
	// End of being not done

	fclose( game_file );

	// See if the campaign state values have to be recalculated
	// because an old save game was loaded.
	if ( env.campaign_rounds < 0. ) {
		env.campaign_rounds = static_cast< double >( env.rounds ) / 5.;
	}
	if ( env.next_campaign_round < 0. ) {
		env.next_campaign_round = static_cast< double >( env.rounds ) - env.campaign_rounds;
		while ( global.current_round < env.next_campaign_round ) {
			env.next_campaign_round -= env.campaign_rounds;
		}
	}

	// To ensure backwards compatibility, all players have to check
	// their opponent memory. Old save files do not provide any.
	for ( int32_t i = 0; i < env.num_game_players; ++i ) {
		env.players[ i ]->check_opp_mem();
	}

	// Revert locale settings
	if ( cur_lc_numeric ) {
		setlocale( LC_NUMERIC, cur_lc_numeric );
	} else {
		setlocale( LC_NUMERIC, "" );
	}

	return true;
}

/*
Check to see if a saved game exists with the given name.
*/
bool check_for_saved_game() {
	string save_path{ env.config_dir + string( "/" ).append( env.game_name ).append( ".sav" ) };

	if ( !access( save_path.c_str(), R_OK ) ) {
		return true;
	}

	return false;
}

/** @brief Copy atanks config to safe location.
 *
 * This function copies the atanks config file from the HOME_DIR folder to
 * HOME_DIR/.atanks
 * If the .atanks folder does not exist, this function will create it.
 *
 * @return true on success, false otherwise
 */
bool copy_config_file() {
	static char xHere[ 2 ]             = ".";
	char        buffer[ PATH_MAX + 1 ] = { 0 };

	// check to see if the config file has already been copied
	string dest_path{ env.config_dir + string( "/atanks-config.txt" ) };
	if ( !access( dest_path.c_str(), R_OK | W_OK ) ) {
		return true;
	}

	char* my_home_folder = getenv( HOME_DIR );

	// figure out where home is
	if ( !my_home_folder ) {
		my_home_folder = xHere;
	}


	// file not copied yet, create the required directory
	snprintf( buffer, PATH_MAX, "%s/.atanks", my_home_folder );
#ifdef ATANKS_IS_WINDOWS
	int32_t mkdir_status = mkdir( buffer );
#else
	int32_t mkdir_status = mkdir( buffer, 0700 );
#endif // ATANKS_IS_WINDOWS

	if ( mkdir_status == -1 ) {
		printf( "Error occured. Unable to create sub directory.\n" );
		return false;
	}

	// check to make sure we have a source file
	string source_path{ string( my_home_folder ) + string( "/.atanks-config.txt" ) };
	FILE*  source_file = fopen( source_path.c_str(), "r" );
	if ( !source_file ) {
		return true;
	}

	// we already have an open source file, create destination file
	FILE* dest_file = fopen( dest_path.c_str(), "wb" );
	if ( !dest_file ) {
		printf( "Unable to create destination file.\n" );
		fclose( source_file );
		return false;
	}

	// we have open files, let's copy
	size_t file_status = fread( buffer, 1, PATH_MAX, source_file );
	while ( file_status ) {
		fwrite( buffer, 1, PATH_MAX, dest_file );
		file_status = fread( buffer, 1, PATH_MAX, source_file );
	}

	fclose( source_file );
	fclose( dest_file );
	return true;
}

/** @brief Make sure we have a music folder
 * @return true on success or false if an error occures.
 **/
bool create_music_folder() {
	string music_dir{ env.config_dir + string( "/music" ) };
	DIR*   music_folder = opendir( music_dir.c_str() );

	if ( !music_folder ) {
#ifdef ATANKS_IS_WINDOWS
		if ( mkdir( music_dir.c_str() ) )
#else
		if ( mkdir( music_dir.c_str(), 0700 ) )
#endif // ATANKS_IS_WINDOWS
			return false;
	} else {
		// it already exists
		closedir( music_folder );
	}

	return true;
}

/*
Scroll text in a box
*/
void scroll_text_list( TEXTBLOCK* lines ) {
	int32_t          spacing  = 2;
	int32_t          tOffset  = ( RAND_MAX / 4 ) + ( get_rand() % ( RAND_MAX / 4 ) );
	int32_t          numItems = ( get_rand() % 100 ) + 20;
	bool             done     = false;
	bool             moving   = true;

	EBackgroundTypes bg_type =
		env.dynamic_menu_bg ? static_cast< EBackgroundTypes >( get_rand() % BACKGROUND_COUNT ) : BACKGROUND_BLANK;

	draw_menu_background( bg_type, tOffset, numItems );
	quick_change( true );

	int32_t clip_l = env.half_width - 299;
	int32_t clip_t = env.menu_begin_y + 1;
	int32_t clip_b = env.menu_end_y - 1;
	int32_t clip_r = env.half_width + 299;
	set_clip_rect( global.canvas, clip_l, clip_t, clip_r, clip_b );

	int32_t scrollOffset = clip_b - env.half_height - 14;
	flush_inputs();
	SHOW_MOUSE( nullptr )

	do {
		if ( global.is_close_btn_pressed() ) {
			done = true;
		}

		if ( ++tOffset >= INT_MAX ) {
			tOffset = 0;
		}
		if ( moving ) {
			scrollOffset--;
		}

		if ( scrollOffset < -( env.half_height - 100 + lines->lines() * 30 ) ) {
			scrollOffset = env.half_height - 100;
		} else if ( scrollOffset > ( clip_b - env.half_height - 14 ) ) {
			scrollOffset = clip_b - env.half_height - 14;
		}

		draw_menu_background( bg_type, tOffset, numItems );
		lines->render_lines( scrollOffset, spacing, clip_t, clip_b );
		global.make_update( env.half_width - 300, env.menu_begin_y, 601, env.screen_height - 2 * env.menu_begin_y );
		global.do_updates();
		LINUX_REST;

		if ( keypressed() ) {
			k = ( readkey() ) >> 8;
			switch ( k ) {
				case KEY_ESC:
					done = true;
					break;
				case KEY_SPACE:
					moving = true;
					break;
				case KEY_UP:
					scrollOffset += 2;
					moving        = false;
					break;
				case KEY_DOWN:
					scrollOffset -= 2;
					moving        = false;
					break;
				default:
					break;
			} // end of switch
		}         // end of key pressed

		if ( mouse_b ) {
			done = true;
		}
	} while ( !done );

	SHOW_MOUSE( global.canvas )
	set_clip_rect( global.canvas, 0, 0, ( env.screen_width - 1 ), ( env.screen_height - 1 ) );
	flush_inputs();
}

/* Flush key buffer and waits for button releases */
void flush_inputs() {
	do {
		std::this_thread::yield();
	} while ( mouse_b );
	clear_keybuf();
}

// This file loads weapons, naturals and items
/// File-static TOML arsenal helpers (WP PF-1.17.3). Only used by load_weapons_text() below.

/// @brief Read a required integer key; complain with file/index context and fail otherwise.
static bool toml_require_int( toml::table const& record,
                              char const*        key,
                              int32_t&           target,
                              string const&      file,
                              int32_t            idx ) {
	auto const value = record[ key ].value< int64_t >();
	if ( !value || ( *value < INT32_MIN ) || ( *value > INT32_MAX ) ) {
		cerr << file << ": record " << idx << " misses 32-bit integer key \"" << key << "\"" << endl;
		return false;
	}
	target = static_cast< int32_t >( *value );
	return true;
}

/// @brief Read a required float key (integer TOML values accepted, per spec); complain and fail otherwise.
static bool toml_require_float( toml::table const& record,
                                char const*        key,
                                double&            target,
                                string const&      file,
                                int32_t            idx ) {
	if ( auto const value = record[ key ].value< double >() ) {
		target = *value;
		return true;
	}
	if ( auto const ivalue = record[ key ].value< int64_t >() ) {
		target = static_cast< double >( *ivalue );
		return true;
	}
	cerr << file << ": record " << idx << " misses numeric key \"" << key << "\"" << endl;
	return false;
}

/// @brief Read a required string key; complain and fail otherwise.
static bool toml_require_string( toml::table const& record,
                                 char const*        key,
                                 string&            target,
                                 string const&      file,
                                 int32_t            idx ) {
	if ( auto const value = record[ key ].value< string >() ) {
		target = *value;
		return true;
	}
	cerr << file << ": record " << idx << " misses string key \"" << key << "\"" << endl;
	return false;
}

/// @brief Fill one weapon/natural record (both are CWeapon); all keys required per spec.
static bool fill_weapon_record( toml::table const& record, CWeapon& dest, string const& file, int32_t idx ) {
	string name, desc;
	if ( !toml_require_string( record, "name", name, file, idx ) ) {
		return false;
	}
	if ( !toml_require_string( record, "desc", desc, file, idx ) ) {
		return false;
	}
	dest.set_name( name.c_str() );
	dest.set_desc( desc.c_str() );
	return toml_require_int( record, "cost", dest.cost, file, idx )
	    && toml_require_int( record, "amt", dest.amt, file, idx )
	    && toml_require_float( record, "mass", dest.mass, file, idx )
	    && toml_require_float( record, "drag", dest.drag, file, idx )
	    && toml_require_int( record, "radius", dest.radius, file, idx )
	    && toml_require_int( record, "sound", dest.sound, file, idx )
	    && toml_require_int( record, "etime", dest.etime, file, idx )
	    && toml_require_int( record, "damage", dest.damage, file, idx )
	    && toml_require_int( record, "picpoint", dest.picpoint, file, idx )
	    && toml_require_int( record, "spread", dest.spread, file, idx )
	    && toml_require_int( record, "delay", dest.delay, file, idx )
	    && toml_require_int( record, "noimpact", dest.noimpact, file, idx )
	    && toml_require_int( record, "tech_level", dest.tech_level, file, idx )
	    && toml_require_int( record, "warhead", dest.warhead, file, idx )
	    && toml_require_int( record, "num_submunitions", dest.numSubmunitions, file, idx )
	    && toml_require_int( record, "submunition", dest.submunition, file, idx )
	    && toml_require_float( record, "impart_velocity", dest.impartVelocity, file, idx )
	    && toml_require_int( record, "divergence", dest.divergence, file, idx )
	    && toml_require_float( record, "spread_variation", dest.spreadVariation, file, idx )
	    && toml_require_float( record, "launch_speed", dest.launchSpeed, file, idx )
	    && toml_require_float( record, "speed_variation", dest.speedVariation, file, idx )
	    && toml_require_int( record, "countdown", dest.countdown, file, idx )
	    && toml_require_float( record, "count_variation", dest.countVariation, file, idx );
}

/// @brief Fill one item record; the vals array holds 0-6 effect values per spec.
static bool fill_item_record( toml::table const& record, CItem& dest, string const& file, int32_t idx ) {
	string name, desc;
	if ( !toml_require_string( record, "name", name, file, idx ) ) {
		return false;
	}
	if ( !toml_require_string( record, "desc", desc, file, idx ) ) {
		return false;
	}
	dest.set_name( name.c_str() );
	dest.set_desc( desc.c_str() );
	if ( !toml_require_int( record, "cost", dest.cost, file, idx )
	     || !toml_require_int( record, "amt", dest.amt, file, idx )
	     || !toml_require_int( record, "selectable", dest.selectable, file, idx )
	     || !toml_require_int( record, "tech_level", dest.tech_level, file, idx )
	     || !toml_require_int( record, "sound", dest.sound, file, idx ) ) {
		return false;
	}
	toml::array const* const vals = record[ "vals" ].as_array();
	if ( ( nullptr == vals ) || ( vals->size() > 6 ) ) {
		cerr << file << ": record " << idx << " misses a vals array of 0-6 numbers" << endl;
		return false;
	}
	for ( size_t i = 0; i < vals->size(); ++i ) {
		toml::node const* const element = &( *vals )[ i ];
		if ( auto const value = element->value< double >() ) {
			dest.vals[ i ] = *value;
		} else if ( auto const ivalue = element->value< int64_t >() ) {
			dest.vals[ i ] = static_cast< double >( *ivalue );
		} else {
			cerr << file << ": record " << idx << " vals[" << i << "] is no number" << endl;
			return false;
		}
	}
	return true;
}

/// @brief Overwrite display strings from one translation array by index; short arrays keep base strings.
template< typename T >
static bool apply_display_strings( toml::table const& root,
                                   char const*        array_key,
                                   T*                 catalog,
                                   int32_t            count,
                                   string const&      file ) {
	toml::node const* const array_node = root[ array_key ].node();
	if ( nullptr == array_node ) {
		return true; // no translations of this kind
	}
	toml::array const* const arr = array_node->as_array();
	if ( nullptr == arr ) {
		cerr << file << ": \"" << array_key << "\" is no array" << endl;
		return false;
	}
	for ( int32_t i = 0; ( i < count ) && ( i < static_cast< int32_t >( arr->size() ) ); ++i ) {
		toml::table const* const entry = ( *arr )[ static_cast< size_t >( i ) ].as_table();
		if ( nullptr == entry ) {
			cerr << file << ": " << array_key << " entry " << i << " is no table" << endl;
			return false;
		}
		string name, desc;
		if ( !toml_require_string( *entry, "name", name, file, i )
		     || !toml_require_string( *entry, "desc", desc, file, i ) ) {
			return false;
		}
		catalog[ i ].set_name( name.c_str() );
		catalog[ i ].set_desc( desc.c_str() );
	}
	return true;
}

/// @brief Parse one TOML file; complain with its path and fail otherwise.
static bool parse_toml_file( string const& path, toml::table& table ) {
	try {
		table = toml::parse_file( path );
	} catch ( std::exception const& err ) {
		cerr << path << ": " << err.what() << endl;
		return false;
	}
	return true;
}

/// @brief Fill one record, dispatching to the weapon or item filler by catalog type.
static bool fill_record( toml::table const& record, CWeapon& dest, string const& file, int32_t idx ) {
	return fill_weapon_record( record, dest, file, idx );
}

/// @brief Fill one record, dispatching to the weapon or item filler by catalog type.
static bool fill_record( toml::table const& record, CItem& dest, string const& file, int32_t idx ) {
	return fill_item_record( record, dest, file, idx );
}

/// @brief Load one base file array into its catalog; counts are strict per spec.
template< typename T >
static bool load_base_array( string const& path, char const* array_key, T* catalog, int32_t count ) {
	toml::table table;
	if ( !parse_toml_file( path, table ) ) {
		return false;
	}
	toml::node const* const array_node = table[ array_key ].node();
	toml::array const*      arr        = ( nullptr == array_node ) ? nullptr : array_node->as_array();
	if ( ( nullptr == arr ) || ( static_cast< int32_t >( arr->size() ) != count ) ) {
		cerr << path << ": needs exactly " << count << " \"" << array_key << "\" records" << endl;
		return false;
	}
	for ( int32_t i = 0; i < count; ++i ) {
		toml::table const* const entry = ( *arr )[ static_cast< size_t >( i ) ].as_table();
		if ( nullptr == entry ) {
			cerr << path << ": " << array_key << " entry " << i << " is no table" << endl;
			return false;
		}
		if ( !fill_record( *entry, catalog[ i ], path, i ) ) {
			return false;
		}
	}
	return true;
}

// from a text file
// Returns true on success and false on failure
bool load_weapons_text() {
	// Base files first (numeric stats plus English display strings).
	if ( !load_base_array( env.data_dir + "/text/weapons.toml", "weapon", weapon, WEAPONS )
	     || !load_base_array( env.data_dir + "/text/naturals.toml", "natural", naturals, NATURALS )
	     || !load_base_array( env.data_dir + "/text/items.toml", "item", item, ITEMS ) ) {
		return false;
	}

	// A second pass overwrites only display strings for localization,
	// mirroring the legacy English-preload protocol in load_game_files().
	if ( EL_ENGLISH != env.language ) {
		string trans_file{ env.data_dir };
		if ( EL_PORTUGUESE == env.language ) {
			trans_file += "/text/weapons.pt_BR.toml";
		} else if ( EL_FRENCH == env.language ) {
			trans_file += "/text/weapons_fr.toml";
		} else if ( EL_GERMAN == env.language ) {
			trans_file += "/text/weapons_de.toml";
		} else if ( EL_SLOVAK == env.language ) {
			trans_file += "/text/weapons_sk.toml";
		} else if ( EL_RUSSIAN == env.language ) {
			trans_file += "/text/weapons_ru.toml";
		} else if ( EL_SPANISH == env.language ) {
			trans_file += "/text/weapons_ES.toml";
		} else if ( EL_ITALIAN == env.language ) {
			trans_file += "/text/weapons_it.toml";
		} else {
			return true; // unknown language: keep English strings
		}
		toml::table translation;
		if ( !parse_toml_file( trans_file, translation ) ) {
			return false;
		}
		if ( !apply_display_strings( translation, "weapon", weapon, WEAPONS, trans_file )
		     || !apply_display_strings( translation, "natural", naturals, NATURALS, trans_file )
		     || !apply_display_strings( translation, "item", item, ITEMS, trans_file ) ) {
			return false;
		}
	}

	return true;
}





/*
Filter out files that do not have .sav in the name.
*/
#ifdef MACOSX
int filter_file( struct dirent* my_file )
#else
int filter_file( const struct dirent* my_file )
#endif
{
	if ( strstr( my_file->d_name, ".sav" ) ) {
		return true;
	} else {
		return false;
	}
}

/*
This function finds a list of saved games on your profile.
On error, NULL is returned. If all goes well, a list of file names
are returned.
After use, the return value *must* be freed.
*/
#if defined( ATANKS_IS_LINUX )
dirent** find_saved_games( uint32_t& num_files_found ) {
	dirent** my_list = nullptr;

	int32_t  status  = scandir( env.config_dir.c_str(), &my_list, filter_file, alphasort );
	if ( status < 0 ) {
		printf( "Error trying to find saved games.\n" );
		return nullptr;
	}

	num_files_found = status;

	return my_list;
}
#endif // Linux


/*
This function hunts for saved games. If games are found, the
function returns an array of filenames. If an error occures
or no files are found, NULL is returned.
*/
#if defined( ATANKS_IS_WINDOWS )
dirent** find_saved_games( uint32_t& num_files_found ) {
	dirent** my_list    = (dirent**)calloc( 256, sizeof( dirent* ) );
	dirent*  one_file   = nullptr;
	uint32_t file_count = 0;
	DIR*     game_dir   = nullptr;

	if ( !my_list ) {
		return nullptr;
	}

	game_dir = opendir( env.config_dir );
	if ( !game_dir ) {
		free( my_list );
		return nullptr;
	}

	while ( ( one_file = readdir( game_dir ) ) && ( file_count < 256 ) ) {
		// check to see if this is a save game file
		if ( strstr( one_file->d_name, ".sav" ) ) {
			my_list[ file_count ] = (dirent*)calloc( 1, sizeof( dirent ) );
			if ( my_list[ file_count ] ) {
				memcpy( my_list[ file_count ], one_file, sizeof( dirent ) );
				my_list[ file_count ]->d_name = strdup( one_file->d_name );
				file_count++;
			}
		}
	}

	closedir( game_dir );
	num_files_found = file_count;
	return my_list;
}
#endif // ATANKS_IS_WINDOWS


/*
 * This function searches for bitmap files (.bmp) in the config folder.
 * The function returns an array of bitmap file names. If no files
 * are found, or an error occures, then NULL is returned.
 * */
char** find_bitmaps( int32_t* bitmaps_found ) {
	char**         my_list;
	struct dirent* one_file;
	int32_t        file_count = 0;
	DIR*           game_dir;

	my_list = (char**)calloc( 256, sizeof( char* ) );
	if ( !my_list ) {
		return nullptr;
	}


	game_dir = opendir( env.config_dir.c_str() );
	if ( !game_dir ) {
		free( my_list );
		return nullptr;
	}

	one_file = readdir( game_dir );
	while ( ( one_file ) && ( file_count < 256 ) ) {
		// check to see if this is a save game file
#ifdef ATANKS_IS_LINUX
		if ( strcasestr( one_file->d_name, ".bmp" ) ) {
#else
		if ( ( strstr( one_file->d_name, ".bmp" ) ) || ( strstr( one_file->d_name, ".BMP" ) ) ) {
#endif // ATANKS_IS_LINUX
			size_t nLen           = env.config_dir.size() + strlen( one_file->d_name ) + 16;
			my_list[ file_count ] = (char*)calloc( nLen + 1, sizeof( char ) );
			if ( my_list[ file_count ] ) {
				snprintf( my_list[ file_count ], nLen, "%s/%s", env.config_dir.c_str(), one_file->d_name );
				file_count++;
			}
		}

		one_file = readdir( game_dir );
	}

	closedir( game_dir );
	*bitmaps_found = file_count;
	if ( file_count < 1 ) {
		free( my_list );
		my_list = nullptr;
	}

	return my_list;
}
