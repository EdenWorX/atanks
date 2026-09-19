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
#include <iterator>
#include <sstream>
#include <vector>


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
bool Save_Game() {
	string save_path{ env.configDir + string( "/" ).append( env.game_name ).append( ".sav" ) };

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
	fprintf( game_file, "CURRENTROUND=%d\n", global.currentround + 1 );
	// Note: When the game is saved, the round already has been decreased. Thus,
	// to not lose rounds when saving a game, returning to main and load it
	// again, this has to be increased here.
	fprintf( game_file, "SCOREBOARD=%d\n", global.showScoreBoard ? 1 : 0 );
	fprintf( game_file, "***\n" );

	// write environment data
	fprintf( game_file, "CEnvironment\n" );
	fprintf( game_file, "CAMPAIGNMODE=%d\n", env.campaign_mode ? 1 : 0 );
	fprintf( game_file, "CAMPAIGNROUNDS=%lf\n", env.campaign_rounds );
	fprintf( game_file, "NEXTCAMPROUND=%lf\n", env.nextCampaignRound );
	fprintf( game_file, "ROUNDS=%u\n", env.rounds );
	fprintf( game_file, "***\n" );

	// write player data
	fprintf( game_file, "PLAYERS\n" );
	for ( int32_t i = 0; i < env.numGamePlayers; ++i ) {
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
This function attempts to load a saved
game.
The function returns true on success and
false if an error occurs.
-- Jesse
*/
bool Load_Game() {
	char    line[ MAX_CONFIG_LINE + 1 ]  = { 0 };
	char    field[ MAX_CONFIG_LINE + 1 ] = { 0 };
	char    value[ MAX_CONFIG_LINE + 1 ] = { 0 };
	int32_t player_count                 = 0;
	int32_t line_num                     = 0;
	int32_t player_idx                   = -1;
	bool    done                         = false;
	int32_t file_version                 = 0;
	char*   result;

	// Be sure that numbers are understood right:
	char const* cur_lc_numeric = setlocale( LC_NUMERIC, "C" );

	// Ensure backward compatibility:
	env.nextCampaignRound = -1.;
	env.campaign_rounds   = -1.;

	// Open game file
	string save_path{ env.configDir + string( "/" ).append( env.game_name ).append( ".sav" ) };
	FILE*  game_file = fopen( save_path.c_str(), "r" );
	if ( !game_file ) {
		return false;
	}

	// Now read until the file is finished loading
	eSaveGameStage stage = SGS_NONE;
	do {
		// read a line
		memset( line, 0, MAX_CONFIG_LINE );
		if ( ( result = fgets( line, MAX_CONFIG_LINE, game_file ) ) ) {
			++line_num;

			// if we hit end of the file, stop
			if ( !strncmp( line, "***EOF***", 9 ) ) {
				done = true;
				continue; // This exits the loop as well
			}

			// strip newline character
			size_t line_length = strlen( line );
			while ( line[ line_length - 1 ] == '\n' ) {
				line[ line_length - 1 ] = '\0';
				line_length--;
			}

			// check to see if we found a new stage
			if ( !strcasecmp( line, "GLOBAL" ) ) {
				stage = SGS_GLOBAL;
			} else if ( !strcasecmp( line, "CEnvironment" ) ) {
				stage = SGS_ENVIRONMENT;
			} else if ( !strcasecmp( line, "PLAYERS" ) ) {
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
			} else if ( !strcasecmp( line, "VERSION" ) ) {
				stage = SGS_VERSION;
			} else {
				// Not a new stage, keep loading.

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

				switch ( stage ) {
					case SGS_ENVIRONMENT:
						if ( !strcasecmp( field, "CAMPAIGNMODE" ) ) {
							int32_t cm = 0;
							SAFE_STOI( cm, value );
							env.campaign_mode = 0 != cm;
						} else if ( !strcasecmp( field, "CAMPAIGNROUNDS" ) ) {
							SAFE_STOD( env.campaign_rounds, value );
						} else if ( !strcasecmp( field, "ROUNDS" ) ) {
							SAFE_STOUL( env.rounds, value );
						} else if ( !strcasecmp( field, "NEXTCAMPROUND" ) ) {
							SAFE_STOD( env.nextCampaignRound, value );
						} else {
							cerr << save_path << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, it does not belong to ENV" << endl;
						}

						break;
					case SGS_GLOBAL:
						if ( !strcasecmp( field, "CURRENTROUND" ) ) {
							SAFE_STOUL( global.currentround, value );
						}

						// The following are kept for backwards compatibility:

						else if ( !strcasecmp( field, "NEXTCAMPROUND" ) ) {
							SAFE_STOD( env.nextCampaignRound, value );
						} else if ( !strcasecmp( field, "CAMPAIGNMODE" ) ) {
							int32_t cm = 0;
							SAFE_STOI( cm, value );
							env.campaign_mode = 0 != cm;
						} else if ( !strcasecmp( field, "ROUNDS" ) ) {
							SAFE_STOUL( env.rounds, value );
						} else if ( !strcasecmp( field, "SCOREBOARD" ) ) {
							int32_t enabled = 1;
							SAFE_STOI( enabled, value );
							global.showScoreBoard = 0 != enabled;
						} else {
							cerr << save_path << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, it does not belong to GLOBAL" << endl;
						}

						break;
					case SGS_PLAYERS:
						if ( !strcasecmp( field, "PLAYERNUMBER" ) ) {
							SAFE_STOI( player_idx, value );
							if ( ( player_idx > -1 ) && ( player_idx < env.numPermanentPlayers )
							     && ( player_count < MAXPLAYERS ) ) {
								env.addGamePlayer( env.allPlayers[ player_idx ] );
								env.players[ player_count++ ]->initialise( true );
							} else {
								player_idx = -1;
							}
						}

						// Now let the player load itself
						if ( player_idx > -1 ) {
							env.allPlayers[ player_idx ]->load_game_data( game_file, file_version );
						} else {
							cerr << save_path << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, as player idx is " << player_idx << endl;
						}

						break;
					case SGS_VERSION:
						if ( !strcasecmp( field, "FILE_VERSION" ) ) {
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
	if ( env.nextCampaignRound < 0. ) {
		env.nextCampaignRound = static_cast< double >( env.rounds ) - env.campaign_rounds;
		while ( global.currentround < env.nextCampaignRound ) {
			env.nextCampaignRound -= env.campaign_rounds;
		}
	}

	// To ensure backwards compatibility, all players have to check
	// their opponent memory. Old save files do not provide any.
	for ( int32_t i = 0; i < env.numGamePlayers; ++i ) {
		env.players[ i ]->checkOppMem();
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
bool Check_For_Saved_Game() {
	string save_path{ env.configDir + string( "/" ).append( env.game_name ).append( ".sav" ) };

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
bool Copy_Config_File() {
	static char xHere[ 2 ]             = ".";
	char        buffer[ PATH_MAX + 1 ] = { 0 };

	// check to see if the config file has already been copied
	string dest_path{ env.configDir + string( "/atanks-config.txt" ) };
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
bool Create_Music_Folder() {
	string music_dir{ env.configDir + string( "/music" ) };
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
void scrollTextList( TEXTBLOCK* lines ) {
	int32_t          spacing  = 2;
	int32_t          tOffset  = ( RAND_MAX / 4 ) + ( get_rand() % ( RAND_MAX / 4 ) );
	int32_t          numItems = ( get_rand() % 100 ) + 20;
	bool             done     = false;
	bool             moving   = true;

	eBackgroundTypes bgType =
		env.dynamicMenuBg ? static_cast< eBackgroundTypes >( get_rand() % BACKGROUND_COUNT ) : BACKGROUND_BLANK;

	drawMenuBackground( bgType, tOffset, numItems );
	quickChange( true );

	int32_t clip_l = env.halfWidth - 299;
	int32_t clip_t = env.menuBeginY + 1;
	int32_t clip_b = env.menuEndY - 1;
	int32_t clip_r = env.halfWidth + 299;
	set_clip_rect( global.canvas, clip_l, clip_t, clip_r, clip_b );

	int32_t scrollOffset = clip_b - env.halfHeight - 14;
	flush_inputs();
	SHOW_MOUSE( nullptr )

	do {
		if ( global.isCloseBtnPressed() ) {
			done = true;
		}

		if ( ++tOffset >= INT_MAX ) {
			tOffset = 0;
		}
		if ( moving ) {
			scrollOffset--;
		}

		if ( scrollOffset < -( env.halfHeight - 100 + lines->Lines() * 30 ) ) {
			scrollOffset = env.halfHeight - 100;
		} else if ( scrollOffset > ( clip_b - env.halfHeight - 14 ) ) {
			scrollOffset = clip_b - env.halfHeight - 14;
		}

		drawMenuBackground( bgType, tOffset, numItems );
		lines->Render_Lines( scrollOffset, spacing, clip_t, clip_b );
		global.make_update( env.halfWidth - 300, env.menuBeginY, 601, env.screenHeight - 2 * env.menuBeginY );
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
	set_clip_rect( global.canvas, 0, 0, ( env.screenWidth - 1 ), ( env.screenHeight - 1 ) );
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
// from a text file
// Returns true on success and false on failure
bool Load_Weapons_Text() {
	// Be sure that numbers are understood right:
	char const* cur_lc_numeric = setlocale( LC_NUMERIC, "C" );
	string      weap_file{ env.dataDir };

	// get path name
	if ( env.language == EL_ENGLISH ) {
		weap_file += "/text/weapons.txt";
	} else if ( env.language == EL_PORTUGUESE ) {
		weap_file += "/text/weapons.pt_BR.txt";
	} else if ( env.language == EL_FRENCH ) {
		weap_file += "/text/weapons_fr.txt";
	} else if ( env.language == EL_GERMAN ) {
		weap_file += "/text/weapons_de.txt";
	} else if ( env.language == EL_SLOVAK ) {
		weap_file += "/text/weapons_sk.txt";
	} else if ( env.language == EL_RUSSIAN ) {
		weap_file += "/text/weapons_ru.txt";
	} else if ( env.language == EL_SPANISH ) {
		weap_file += "/text/weapons_ES.txt";
	} else if ( env.language == EL_ITALIAN ) {
		weap_file += "/text/weapons_it.txt";
	}

	// open file
	FILE* wfile = fopen( weap_file.c_str(), "r" );

	if ( !wfile ) {
		printf( "Unable to open weapons file. (%s)\n", weap_file.c_str() );
		return false;
	}

	// read line
	char       line[ 512 ]   = { 0 };
	char*      status        = fgets( line, 512, wfile );
	eFileStage file_stage    = FS_WEAPONS; // weapons, naturals, items
	eDataStage data_stage    = DS_NAME;    // name, description, data
	int32_t    item_count    = 0;
	int32_t    weapon_count  = 0;
	int32_t    natural_count = 0;

	while ( status ) {
		// clear end of line
		if ( strchr( line, '\n' ) ) {
			strchr( line, '\n' )[ 0 ] = '\0';
		}
		if ( strchr( line, '\r' ) ) {
			strchr( line, '\r' )[ 0 ] = '\0';
		}

		// skip # and empty lines
		if ( ( line[ 0 ] != '#' ) && ( strlen( line ) > 2 ) ) {

			// check for header
			if ( !strcasecmp( line, "*WEAPONS*" ) ) {
				file_stage = FS_WEAPONS;
				data_stage = DS_NAME;
			} else if ( !strcasecmp( line, "*NATURALS*" ) ) {
				file_stage = FS_NATURALS;
				data_stage = DS_NAME;
			} else if ( !strcasecmp( line, "*ITEMS*" ) ) {
				file_stage = FS_ITEMS;
				data_stage = DS_NAME;
			}

			// not a special line, let's read some data
			else {
				// =============
				// == Weapons ==
				// =============
				if ( ( FS_WEAPONS == file_stage ) && ( weapon_count < WEAPONS ) ) {
					if ( DS_NAME == data_stage ) {
						weapon[ weapon_count ].setName( line );
					} else if ( DS_DESC == data_stage ) {
						weapon[ weapon_count ].setDesc( line );
					} else if ( DS_DATA == data_stage ) {
						std::istringstream    iss( line );
						std::vector< string > values(
							std::istream_iterator< string >{ iss },
							std::istream_iterator< string >()
						);

						if ( values.size() >= 22 ) { // assuming there are 22 elements to parse
							auto value = values.begin();
							SAFE_STOI( weapon[ weapon_count ].cost, *value++ );
							SAFE_STOI( weapon[ weapon_count ].amt, *value++ );
							SAFE_STOD( weapon[ weapon_count ].mass, *value++ );
							SAFE_STOD( weapon[ weapon_count ].drag, *value++ );
							SAFE_STOI( weapon[ weapon_count ].radius, *value++ );
							SAFE_STOI( weapon[ weapon_count ].sound, *value++ );
							SAFE_STOI( weapon[ weapon_count ].etime, *value++ );
							SAFE_STOI( weapon[ weapon_count ].damage, *value++ );
							SAFE_STOI( weapon[ weapon_count ].picpoint, *value++ );
							SAFE_STOI( weapon[ weapon_count ].spread, *value++ );
							SAFE_STOI( weapon[ weapon_count ].delay, *value++ );
							SAFE_STOI( weapon[ weapon_count ].noimpact, *value++ );
							SAFE_STOI( weapon[ weapon_count ].techLevel, *value++ );
							SAFE_STOI( weapon[ weapon_count ].warhead, *value++ );
							SAFE_STOI( weapon[ weapon_count ].numSubmunitions, *value++ );
							SAFE_STOI( weapon[ weapon_count ].submunition, *value++ );
							SAFE_STOD( weapon[ weapon_count ].impartVelocity, *value++ );
							SAFE_STOI( weapon[ weapon_count ].divergence, *value++ );
							SAFE_STOD( weapon[ weapon_count ].spreadVariation, *value++ );
							SAFE_STOD( weapon[ weapon_count ].launchSpeed, *value++ );
							SAFE_STOD( weapon[ weapon_count ].speedVariation, *value++ );
							SAFE_STOI( weapon[ weapon_count ].countdown, *value++ );
							SAFE_STOD( weapon[ weapon_count ].countVariation, *value++ );
						} else {
							cerr << "Weapon " << weapon_count << " \""
							     << weapon[ weapon_count ].getName() << " has only "
							     << values.size() << "entries but needs 22!" << endl;
						}
					}

					// Advance data stage
					++data_stage;
					if ( ( DS_NAME == data_stage ) // flipped
					     || ( ( DS_DATA == data_stage ) && ( EL_ENGLISH != env.language ) ) ) {
						data_stage = DS_NAME;
						weapon_count++;
					}
				} // end of a weapon section

				// ==============
				// == Naturals ==
				// ==============
				else if ( ( FS_NATURALS == file_stage ) && ( natural_count < NATURALS ) ) {
					if ( DS_NAME == data_stage ) {
						naturals[ natural_count ].setName( line );
					} else if ( DS_DESC == data_stage ) {
						naturals[ natural_count ].setDesc( line );
					} else if ( DS_DATA == data_stage ) {
						std::istringstream    iss( line );
						std::vector< string > values(
							std::istream_iterator< string >{ iss },
							std::istream_iterator< string >()
						);

						if ( values.size() >= 23 ) { // assuming there are 23 elements to parse
							auto value = values.begin();
							SAFE_STOI( naturals[ natural_count ].cost, *value++ );
							SAFE_STOI( naturals[ natural_count ].amt, *value++ );
							SAFE_STOD( naturals[ natural_count ].mass, *value++ );
							SAFE_STOD( naturals[ natural_count ].drag, *value++ );
							SAFE_STOI( naturals[ natural_count ].radius, *value++ );
							SAFE_STOI( naturals[ natural_count ].sound, *value++ );
							SAFE_STOI( naturals[ natural_count ].etime, *value++ );
							SAFE_STOI( naturals[ natural_count ].damage, *value++ );
							SAFE_STOI( naturals[ natural_count ].picpoint, *value++ );
							SAFE_STOI( naturals[ natural_count ].spread, *value++ );
							SAFE_STOI( naturals[ natural_count ].delay, *value++ );
							SAFE_STOI( naturals[ natural_count ].noimpact, *value++ );
							SAFE_STOI( naturals[ natural_count ].techLevel, *value++ );
							SAFE_STOI( naturals[ natural_count ].warhead, *value++ );
							SAFE_STOI( naturals[ natural_count ].numSubmunitions, *value++ );
							SAFE_STOI( naturals[ natural_count ].submunition, *value++ );
							SAFE_STOD( naturals[ natural_count ].impartVelocity, *value++ );
							SAFE_STOI( naturals[ natural_count ].divergence, *value++ );
							SAFE_STOD( naturals[ natural_count ].spreadVariation, *value++ );
							SAFE_STOD( naturals[ natural_count ].launchSpeed, *value++ );
							SAFE_STOD( naturals[ natural_count ].speedVariation, *value++ );
							SAFE_STOI( naturals[ natural_count ].countdown, *value++ );
							SAFE_STOD( naturals[ natural_count ].countVariation, *value++ );
						} else {
							cerr << "Natural " << natural_count << " \""
							     << naturals[ natural_count ].getName() << " has only "
							     << values.size() << "entries but needs 23!" << endl;
						}
					}

					// Advance data stage
					++data_stage;
					if ( ( DS_NAME == data_stage ) // flipped
					     || ( ( DS_DATA == data_stage ) && ( EL_ENGLISH != env.language ) ) ) {
						data_stage = DS_NAME;
						natural_count++;
					}

				} // end of naturals

				// ==============
				// == Items ==
				// ==============
				else if ( ( FS_ITEMS == file_stage ) && ( item_count < ITEMS ) ) {
					if ( DS_NAME == data_stage ) {
						item[ item_count ].setName( line );
					} else if ( DS_DESC == data_stage ) {
						item[ item_count ].setDesc( line );
					} else if ( DS_DATA == data_stage ) {
						std::istringstream    iss( line );
						std::vector< string > values(
							std::istream_iterator< string >{ iss },
							std::istream_iterator< string >()
						);

						if ( values.size() >= 5 ) { // teleporters and the fan have 5
							auto value = values.begin();
							SAFE_STOI( item[ item_count ].cost, *value++ );
							SAFE_STOI( item[ item_count ].amt, *value++ );
							SAFE_STOI( item[ item_count ].selectable, *value++ );
							SAFE_STOI( item[ item_count ].techLevel, *value++ );
							SAFE_STOI( item[ item_count ].sound, *value++ );
							if ( values.size() >= 6 ) { // amps/armors have 6
								SAFE_STOD( item[ item_count ].vals[ 0 ], *value++ );
							}
							if ( values.size() >= 7 ) { // vengeance, parachute and repair have 7
								SAFE_STOD( item[ item_count ].vals[ 1 ], *value++ );
							}
							if ( values.size() >= 11 ) { // The rest has 11
								SAFE_STOD( item[ item_count ].vals[ 2 ], *value++ );
								SAFE_STOD( item[ item_count ].vals[ 3 ], *value++ );
								SAFE_STOD( item[ item_count ].vals[ 4 ], *value++ );
								SAFE_STOD( item[ item_count ].vals[ 5 ], *value++ );
							} else if ( values.size() > 7 ) {
								cerr << "Item " << item_count << " \""
								     << item[ item_count ].getName() << " has only "
								     << values.size() << " entries but needs 11 if more than 7!"
								     << endl;
							}
						} else {
							cerr << "Item " << item_count << " \"" << item[ item_count ].getName()
							     << " has only " << values.size() << "entries but needs at least 5!"
							     << endl;
						}
					}

					// Advance data stage
					++data_stage;
					if ( ( DS_NAME == data_stage ) // flipped
					     || ( ( DS_DATA == data_stage ) && ( EL_ENGLISH != env.language ) ) ) {
						data_stage = DS_NAME;
						item_count++;
					}
				} // end of items
			}         // end of reading data from a valid line
		}                 // end of valid line

		// read in data
		status = fgets( line, 512, wfile );
	} // end while(status)


	// close file
	fclose( wfile );


	// Revert locale settings
	if ( cur_lc_numeric ) {
		setlocale( LC_NUMERIC, cur_lc_numeric );
	} else {
		setlocale( LC_NUMERIC, "" );
	}

	return true;
}

/*
Filter out files that do not have .sav in the name.
*/
#ifdef MACOSX
int Filter_File( struct dirent* my_file )
#else
int Filter_File( const struct dirent* my_file )
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
dirent** Find_Saved_Games( uint32_t& num_files_found ) {
	dirent** my_list = nullptr;

	int32_t  status  = scandir( env.configDir.c_str(), &my_list, Filter_File, alphasort );
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
dirent** Find_Saved_Games( uint32_t& num_files_found ) {
	dirent** my_list    = (dirent**)calloc( 256, sizeof( dirent* ) );
	dirent*  one_file   = nullptr;
	uint32_t file_count = 0;
	DIR*     game_dir   = nullptr;

	if ( !my_list ) {
		return nullptr;
	}

	game_dir = opendir( env.configDir );
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
char** Find_Bitmaps( int32_t* bitmaps_found ) {
	char**         my_list;
	struct dirent* one_file;
	int32_t        file_count = 0;
	DIR*           game_dir;

	my_list = (char**)calloc( 256, sizeof( char* ) );
	if ( !my_list ) {
		return nullptr;
	}


	game_dir = opendir( env.configDir.c_str() );
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
			size_t nLen           = env.configDir.size() + strlen( one_file->d_name ) + 16;
			my_list[ file_count ] = (char*)calloc( nLen + 1, sizeof( char ) );
			if ( my_list[ file_count ] ) {
				snprintf( my_list[ file_count ], nLen, "%s/%s", env.configDir.c_str(), one_file->d_name );
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
