#include "client.h"

#include "beam.h"
#include "explosion.h"
#include "floattext.h"
#include "item.h"
#include "main.h"
#include "missile.h"
#include "network.h"
#include "player.h"
#include "random.h"
#include "sky.h"
#include "tank.h"
#include "teleport.h"

// Note: Don't guard everything. Empty compilation units are invalid.
#ifdef NETWORK

// From gameloop.cpp:
void draw_top_bar();

// Here we try to match the buffer with an action. We then attempt to
// perform the action. Remember, this is a command from the server, so
// it is either giving us some info or telling us to create something.
bool Parse_Client_Data( char *buffer ) {
	char   args[ CLIENT_ARGS ][ BUFFER_SIZE ];
	char   letter;
	int    dest_string;
	size_t line_length = strlen( buffer );
	size_t sourceindex = 0, destindex = 0;

	// clear buffers
	for ( dest_string = 0; dest_string < CLIENT_ARGS; dest_string++ ) {
		memset( args[ dest_string ], '\0', BUFFER_SIZE );
	}

	dest_string = 0;
	// copy buffer into cmd and argument variables
	while ( ( sourceindex < line_length ) && ( dest_string < CLIENT_ARGS ) ) {
		letter = buffer[ sourceindex ];
		if ( letter == ' ' ) {
			letter                           = '\0';
			args[ dest_string ][ destindex ] = letter;
			destindex                        = 0;
			dest_string++;
		} else {
			args[ dest_string ][ destindex ] = letter;
			destindex++;
		}
		sourceindex++;
	}

	// let us see what we have
	if ( !strcmp( args[ 0 ], "SERVERVERSION" ) ) {
		if ( !strcmp( args[ 1 ], VERSION ) ) {
			printf( "Server version matchs us. OK.\n" );
		} else {
			printf( "Server version is %s, we are %s. This is likely to cause problems.\n", args[ 1 ], VERSION );
		}
		return true;
	} else if ( !strcmp( args[ 0 ], "CURRENTPOSITION" ) ) {
		if ( ( global.client_player ) && ( global.client_player->tank ) ) {
			SAFE_STOD( ( global.client_player->tank->x ), args[ 1 ] );
			SAFE_STOD( ( global.client_player->tank->y ), args[ 2 ] );
		}
	} else if ( !strcmp( args[ 0 ], "BEAM" ) ) {
		double my_x = 0., my_y = 0.;
		int    my_angle = 0, my_type = 0;
		SAFE_STOD( my_x, args[ 1 ] );
		SAFE_STOD( my_y, args[ 2 ] );
		SAFE_STOI( my_angle, args[ 3 ] );
		SAFE_STOI( my_type, args[ 4 ] );
		try {
			new BEAM( nullptr, my_x, my_y, my_angle, my_type, BT_WEAPON );
		} catch ( std::bad_alloc &e ) {
			printf( "Attempt to create beam failed in client code: %s\n", e.what() );
		}
	} else if ( !strcmp( args[ 0 ], "BOXED" ) ) {
		int got_box = 0;
		SAFE_STOI( got_box, args[ 1 ] );
		if ( got_box ) {
			env.isBoxed = true;
		} else {
			env.isBoxed = false;
		}
		return true;
	} else if ( !strcmp( args[ 0 ], "EXPLOSION" ) ) {
		double my_x = 0., my_y = 0.;
		int    my_type = 0;
		SAFE_STOD( my_x, args[ 1 ] );
		SAFE_STOD( my_y, args[ 2 ] );
		SAFE_STOI( my_type, args[ 3 ] );
		try {
			new EXPLOSION( nullptr, my_x, my_y, 0., 0., my_type, true );
		} catch ( std::bad_alloc &e ) {
			printf( "Attempt to create explosion failed in client code: %s\n", e.what() );
		}
		return false;
	} else if ( !strcmp( args[ 0 ], "ITEM" ) ) {
		int itemindex = 0, amount = 0;
		SAFE_STOI( itemindex, args[ 1 ] );
		SAFE_STOI( amount, args[ 2 ] );
		if ( ( itemindex >= 0 ) && ( itemindex < ITEMS ) && ( amount >= 0 ) && ( amount <= 99 ) ) {
			global.client_player->ni[ itemindex ] = amount;
		}
		if ( itemindex == ( ITEMS - 1 ) ) {
			return true;
		}
	} else if ( !strcmp( args[ 0 ], "HEALTH" ) ) {
		int  tankindex = 0;
		int  health = 0, shield = 0, shield_type = 0;
		char some_text[ 32 ];

		SAFE_STOI( tankindex, args[ 1 ] );
		if ( tankindex >= 0 ) {
			SAFE_STOI( health, args[ 2 ] );
			SAFE_STOI( shield, args[ 3 ] );
			SAFE_STOI( shield_type, args[ 4 ] );
			env.players[ tankindex ]->tank->l   = health;
			env.players[ tankindex ]->tank->sh  = shield;
			env.players[ tankindex ]->tank->sht = shield_type;
			// set the text over the tank
			sprintf( some_text, "%d", health );
			env.players[ tankindex ]->tank->healthText.set_text( some_text );
			env.players[ tankindex ]->tank->healthText.set_color( env.players[ tankindex ]->color );
			sprintf( some_text, "%d", shield );
			env.players[ tankindex ]->tank->shieldText.set_text( some_text );
			env.players[ tankindex ]->tank->healthText.set_color( env.players[ tankindex ]->color );
		}
		if ( tankindex == ( env.numGamePlayers - 1 ) ) {
			return true;
		} else {
			return false;
		}
	} else if ( !strcmp( args[ 0 ], "WIND" ) ) {
		SAFE_STOD( ( global.wind ), args[ 1 ] );
		return true;
	} else if ( !strcmp( args[ 0 ], "MISSILE" ) ) {
		int    my_type = 0;
		double my_x = 0., my_y = 0., delta_x = 0., delta_y = 0.;
		SAFE_STOD( my_x, args[ 1 ] );
		SAFE_STOD( my_y, args[ 2 ] );
		SAFE_STOD( delta_x, args[ 3 ] );
		SAFE_STOD( delta_y, args[ 4 ] );
		SAFE_STOI( my_type, args[ 5 ] );
		try {
			new MISSILE( nullptr, my_x, my_y, delta_x, delta_y, my_type, MT_WEAPON, 1, 0 );
		} catch ( std::bad_alloc &e ) {
			printf( "Attempt to create missile failed in client code: %s\n", e.what() );
		}
		return false;
	} else if ( !strcmp( args[ 0 ], "NUMPLAYERS" ) ) {
		SAFE_STOI( ( env.numGamePlayers ), args[ 1 ] );
		// create the players in question
		for ( int counter = 0; counter < env.numGamePlayers; counter++ ) {
			try {
				env.players[ counter ] = new PLAYER();
			} catch ( std::bad_alloc &e ) {
				printf( "Attempt to create PLAYER failed in client code: %s\n", e.what() );
			}
			try {
				env.players[ counter ]->tank = new TANK();
			} catch ( std::bad_alloc &e ) {
				printf( "Attempt to create TANK failed in client code: %s\n", e.what() );
			}
			env.players[ counter ]->tank->player = env.players[ counter ];
			env.players[ counter ]->tank->nameText.set_text( nullptr );
		}
		return true;
	}
	// ping is a special case where we do not do anything it is just
	// making sure we are still here because we are not talking
	else if ( !strcmp( args[ 0 ], "PING" ) ) {
		return false;
	} else if ( !strcmp( args[ 0 ], "PLAYERNAME" ) ) {
		int number = 0;
		SAFE_STOI( number, args[ 1 ] );
		if ( ( number < env.numGamePlayers ) && ( number >= 0 ) ) {
			env.players[ number ]->setName( args[ 2 ] );
		}
		if ( number == ( env.numGamePlayers - 1 ) ) {
			return true;
		}
	} else if ( !strcmp( args[ 0 ], "REMOVETANK" ) ) {
		int index = 0;
		SAFE_STOI( index, args[ 1 ] );
		if ( ( index >= 0 ) && ( index < env.numGamePlayers ) ) {
			// make sure this tank exists before we get rid of it
			if ( env.players[ index ]->tank ) {
				delete env.players[ index ]->tank;
				env.players[ index ]->tank = nullptr;
			}
		}
	} else if ( !strcmp( args[ 0 ], "ROUNDS" ) ) {
		SAFE_STOUL( env.rounds, args[ 1 ] );
		SAFE_STOUL( global.currentround, args[ 2 ] );
		return true;
	} else if ( !strcmp( args[ 0 ], "SURFACE" ) ) {
		int x = 0, y = 0;
		int index         = 0;
		int colour_change = 0;
		int green         = 150;
		int my_height     = 0;

		SAFE_STOI( x, args[ 1 ] );
		SAFE_STOI( y, args[ 2 ] );
		global.surface[ x ].store( y );
		my_height = env.screenHeight - y;
		my_height = my_height / 50; // ratio of change
		// fill in terrain...
		for ( index = y; index < env.screenHeight; index++ ) {
			putpixel( global.terrain, x, index, makecol( 0, green, 0 ) );
			colour_change++;
			if ( colour_change >= my_height ) {
				colour_change = 0;
				green--;
			}
		}
		if ( x >= ( env.screenWidth - 1 ) ) {
			return true;
		}
	} else if ( !strcmp( args[ 0 ], "SCREEN" ) ) {
		int width = 0, height = 0;

		SAFE_STOI( width, args[ 1 ] );
		SAFE_STOI( height, args[ 2 ] );
		if ( ( width == env.screenWidth ) && ( height == env.screenHeight ) ) {
			printf( "Host's screen resolution matches ours.\n" );
		} else {
			printf( "Host's screen resolution is %d by %d.\n", width, height );
			printf( "Ours is %d by %d. This is going to cause problems!\n", env.screenWidth, env.screenHeight );
		}
		return true;
	} else if ( !strcmp( args[ 0 ], "TANKPOSITION" ) ) {
		int     player_number = 0, x = 0, y = 0;
		PLAYER *my_player;

		SAFE_STOI( player_number, args[ 1 ] );
		my_player = env.players[ player_number ];
		if ( ( my_player ) && ( my_player->tank ) ) {
			SAFE_STOI( x, args[ 2 ] );
			SAFE_STOI( y, args[ 3 ] );
			my_player->tank->x = x;
			my_player->tank->y = y;
		}
		if ( player_number == ( env.numGamePlayers - 1 ) ) {
			return true;
		}
	} else if ( !strcmp( args[ 0 ], "TEAM" ) ) {
		int32_t player_number = 0;
		int32_t colour        = BLACK;
		int     the_team      = 0;
		SAFE_STOI( player_number, args[ 1 ] );
		SAFE_STOI( the_team, args[ 2 ] );
		if ( ( the_team < env.numGamePlayers ) && ( the_team >= 0 ) ) {
			env.players[ player_number ]->team = static_cast< eTeamTypes >( the_team );
			if ( the_team == TEAM_JEDI ) {
				colour = makecol( 0, 255, 0 );
			} else if ( the_team == TEAM_SITH ) {
				colour = makecol( 255, 0, 255 );
			} else if ( the_team == TEAM_NEUTRAL ) {
				colour = makecol( 0, 0, 255 );
			}
			if ( env.players[ player_number ] == global.client_player ) {
				colour = makecol( 255, 0, 0 );
			}
			env.players[ player_number ]->color = colour;
		}
		if ( player_number == ( env.numGamePlayers - 1 ) ) {
			return true;
		}
	} else if ( !strcmp( args[ 0 ], "TELEPORT" ) ) {
		int player_num = 0;
		int new_x = 0, new_y = 0;

		SAFE_STOI( player_num, args[ 1 ] );
		SAFE_STOI( new_x, args[ 2 ] );
		SAFE_STOI( new_y, args[ 3 ] );
		if ( ( player_num >= 0 ) && ( player_num < env.numGamePlayers ) && ( env.players[ player_num ]->tank ) ) {
			TANK *lt = env.players[ player_num ]->tank;
			try {
				new TELEPORT( lt, new_x, new_y, ROUND( lt->getDiameter() ), 120, ITEM_TELEPORT );
			} catch ( std::bad_alloc &e ) {
				printf( "Attempt to create teleport failed in client code: %s\n", e.what() );
			}
		}

	} else if ( !strcmp( args[ 0 ], "WALLTYPE" ) ) {
		SAFE_STOI( ( env.current_wallType ), args[ 1 ] );
		switch ( env.current_wallType ) {
			case WALL_RUBBER:
				env.wallColour = makecol( 0, 255, 0 ); // GREEN;
				break;
			case WALL_STEEL:
				env.wallColour = makecol( 255, 0, 0 ); // RED;
				break;
			case WALL_SPRING:
				env.wallColour = makecol( 0, 0, 255 ); // BLUE;
				break;
			case WALL_WRAP:
				env.wallColour = makecol( 255, 255, 0 ); // YELLOW;
				break;
		}
		return true;
	} else if ( !strcmp( args[ 0 ], "WEAPON" ) ) {
		int weaponindex = 0, amount = 0;
		SAFE_STOI( weaponindex, args[ 1 ] );
		SAFE_STOI( amount, args[ 2 ] );
		if ( ( weaponindex >= 0 ) && ( weaponindex < WEAPONS ) && ( amount >= 0 ) && ( amount <= 99 ) ) {
			global.client_player->nm[ weaponindex ] = amount;
		}
		if ( weaponindex == ( WEAPONS - 1 ) ) {
			return true;
		}
	} else if ( !strcmp( args[ 0 ], "YOUARE" ) ) {
		int index = 0;
		SAFE_STOI( index, args[ 1 ] );
		if ( ( index >= 0 ) && ( index < env.numGamePlayers ) ) {
			global.client_player = env.players[ index ];
			global.set_curr_tank( global.client_player->tank );
		}
		return true;
	}

	return false;
}

void Create_Sky() {
	if ( env.custom_background && env.bitmap_filenames ) {
		if ( env.sky ) {
			destroy_bitmap( env.sky );
		}
		env.sky = load_bitmap( env.bitmap_filenames[ get_rand() % env.number_of_bitmaps ], nullptr );
	}

	if ( !env.custom_background || !env.sky ) {
		if ( env.sky && ( ( env.sky->w != env.screenWidth ) || ( env.sky->h != ( env.screenHeight - MENUHEIGHT ) ) ) ) {
			destroy_bitmap( env.sky );
			env.sky = nullptr;
		}

		if ( !env.sky ) {
			env.sky = create_bitmap( env.screenWidth, env.screenHeight - MENUHEIGHT );
		}
		generate_sky(
			nullptr,
			sky_gradients[ global.cursky ],
			( env.ditherGradients ? GENSKY_DITHERGRAD : 0 ) | ( env.detailedSky ? GENSKY_DETAILED : 0 )
		);
	}
} // end of create sky function

// Send a shot command to the server
bool Client_Fire( PLAYER *my_player, int my_socket ) {
	if ( !my_player ) {
		return false;
	}
	if ( !my_player->tank ) {
		return false;
	}

	SAFE_WRITE( my_socket, "FIRE %d %d %d", my_player->tank->cw, my_player->tank->a, my_player->tank->p );

	return true;
}

// Adjust our power on the client side
bool Client_Power( PLAYER *my_player, int more_or_less ) {
	if ( ( my_player ) && ( my_player->tank ) ) {
		if ( ( more_or_less == CLIENT_UP ) && ( my_player->tank->p < 1996 ) ) {
			my_player->tank->p += 5;
		} else if ( ( more_or_less == CLIENT_DOWN ) && ( my_player->tank->p > 5 ) ) {
			my_player->tank->p -= 5;
		}
		return true;
	}
	return false;
}

bool Client_Angle( PLAYER *my_player, int left_or_right ) {
	if ( !my_player ) {
		return false;
	}
	if ( !my_player->tank ) {
		return false;
	}

	if ( ( left_or_right == CLIENT_LEFT ) && ( my_player->tank->a < 270 ) ) {
		my_player->tank->a++;
	} else if ( ( left_or_right == CLIENT_RIGHT ) && ( my_player->tank->a > 90 ) ) {
		my_player->tank->a--;
	}
	return true;
}

bool Client_Cycle_Weapon( PLAYER *my_player, int forward_or_back ) {
	bool found = false;

	if ( !my_player->tank ) {
		return false;
	}

	while ( !found ) {
		if ( forward_or_back == CYCLE_FORWARD ) {
			my_player->tank->cw++;
		} else {
			my_player->tank->cw--;
		}

		if ( my_player->tank->cw >= THINGS ) {
			my_player->tank->cw = 0;
		} else if ( my_player->tank->cw < 0 ) {
			my_player->tank->cw = THINGS - 1;
		}

		// check if we have found a weapon
		if ( my_player->tank->cw < WEAPONS ) {
			if ( my_player->nm[ my_player->tank->cw ] ) {
				found = true;
			}
		} else // an item
		{
			if ( ( item[ my_player->tank->cw - WEAPONS ].selectable )
			     && ( my_player->ni[ my_player->tank->cw - WEAPONS ] ) ) {
				found = true;
			}
		}
	}
	return true;
}

// This function takes an error number and returns a string
// which contains useful information about that error.
// On success, a pointer to char is returned.
// On failure, a nullptr is returned.
// The returned pointer does NOT need to be freed.
char const *Explain_Error( int32_t error_code ) {
	switch ( error_code ) {
		case CLIENT_ERROR_VERSION:
			return env.ingame->Get_Line( 77 );
		case CLIENT_ERROR_SCREENSIZE:
			return env.ingame->Get_Line( 78 );
		case CLIENT_ERROR_DISCONNECT:
			return env.ingame->Get_Line( 79 );
		default:
			break;
	}

	return nullptr;
}

// Client version of the game
// Really, this loop should do some basic things.
// 1. Find out what the landscape should look like from the server.
// 2. Place tanks on the battle field
// 3. Create missiles, beam weapons and such when the server asks us to
// 4. Get input from the player and forward it to the server.
// 5. Clean up at the end of the round.
//
// Function return true if everything went well or false
// if an error occured.
int Game_Client( int socket_number ) {
	int             surface_x = 1, tank_position = 1, team_number = 1, name_number = 1;
	int             weapon_number = 1, item_number = 1, tank_health = 1;
	int             end_of_round = false, keep_playing = false;
	int             game_stage = CLIENT_VERSION;
	char            buffer[ BUFFER_SIZE ];
	int             incoming;
	int             my_key;
	int             time_clock    = 0;
	bool            screen_update = false;
	int             count;                    // generic counter
	int             stuff_going_down = false; // explosions, missiles etc on the screen
	VIRTUAL_OBJECT *my_object, *next_obj;
	int32_t         class_ = 0;
	bool            fired  = false;


	clear_to_color( global.terrain, PINK ); // get terrain ready
	clear_to_color( global.canvas, BLACK );

	// clean up old text
	global.getHeadOfClass( CLASS_FLOATTEXT, &my_object );
	while ( my_object ) {
		my_object->getNext( &next_obj );
		dynamic_cast< FLOATTEXT * >( my_object )->newRound();
		delete my_object;
	}

	Create_Sky(); // so we have a background

	SAFE_WRITE( socket_number, "%s", "VERSION" );

	while ( !end_of_round ) {
		// check for waiting input from the server
		incoming = Check_For_Incoming_Data( socket_number );
		if ( incoming ) {
			ssize_t bytes_read;

			memset( buffer, '\0', BUFFER_SIZE );
			bytes_read = read( socket_number, buffer, BUFFER_SIZE );
			if ( bytes_read > 0 ) {
				// do something with this input
				if ( !strncmp( buffer, "CLOSE", 5 ) ) {
					end_of_round = true;
					keep_playing = false;
					printf( "Got close message.\n" );
					global.client_message = strdup( env.ingame->Get_Line( 81 ) );
				} else if ( !strncmp( buffer, "NOROOM", 6 ) ) {
					end_of_round = true;
					keep_playing = false;
					printf( "The server is full or the game has not started. Please try again later.\n" );
					global.client_message = strdup( env.ingame->Get_Line( 80 ) );
				} else if ( !strncmp( buffer, "GAMEEND", 7 ) ) {
					end_of_round = true;
					keep_playing = false;
					printf( "The game is over.\n" );
					if ( strlen( buffer ) > 7 ) {
						global.client_message = strdup( &( buffer[ 8 ] ) );
					} else {
						global.client_message = strdup( env.ingame->Get_Line( 82 ) );
					}
				} else if ( !strncmp( buffer, "ROUNDEND", 8 ) ) {
					end_of_round = true;
					keep_playing = true;
					printf( "Round is over.\n" );
				}

				else // not a special command, parse it
				{
					if ( Parse_Client_Data( buffer ) ) {
						if ( game_stage < CLIENT_PLAYING ) {
							game_stage++;
						}

						// Request more information
						if ( game_stage < CLIENT_PLAYING ) {
							switch ( game_stage ) {
								case CLIENT_SCREEN:
									strcpy( buffer, "SCREEN" );
									break;
								case CLIENT_WIND:
									strcpy( buffer, "WIND" );
									break;
								case CLIENT_NUMPLAYERS:
									strcpy( buffer, "NUMPLAYERS" );
									break;
								case CLIENT_TANK_POSITION:
									strcpy( buffer, "TANKPOSITION 0" );
									break;
								case CLIENT_SURFACE:
									strcpy( buffer, "SURFACE 0" );
									break;
								case CLIENT_WHOAMI:
									strcpy( buffer, "WHOAMI" );
									break;
								case CLIENT_WEAPONS:
									strcpy( buffer, "WEAPON 0" );
									break;
								case CLIENT_ITEMS:
									strcpy( buffer, "ITEM 0" );
									break;
								case CLIENT_ROUNDS:
									strcpy( buffer, "ROUNDS" );
									break;
								case CLIENT_TEAMS:
									strcpy( buffer, "TEAMS 0" );
									global.updateMenu = true;
									break;
								case CLIENT_WALL_TYPE:
									strcpy( buffer, "WALLTYPE" );
									break;
								case CLIENT_BOXED:
									strcpy( buffer, "BOXED" );
									break;
								case CLIENT_NAME:
									strcpy( buffer, "PLAYERNAME 0" );
									break;
								case CLIENT_TANK_HEALTH:
									strcpy( buffer, "HEALTH 0" );
									break;
								default:
									buffer[ 0 ] = '\0';
							}
							SAFE_WRITE( socket_number, "%s", buffer );
						} // end of getting more info
					}         // our game stage went up
					else      // we got data, but our game stage did not go up
					{
						if ( fired ) {
							if ( ( global.client_player ) && ( global.client_player->tank ) ) {
								fired = false;
								if ( global.client_player->tank->cw < WEAPONS ) {
									SAFE_WRITE(
										socket_number,
										"WEAPON %d",
										global.client_player->tank->cw
									);
								} else {
									SAFE_WRITE(
										socket_number,
										"ITEM %d",
										global.client_player->tank->cw - WEAPONS
									);
								}
							}
						} else if ( game_stage == CLIENT_SURFACE ) {
							SAFE_WRITE( socket_number, "SURFACE %d", surface_x );
							surface_x++;
						} else if ( game_stage == CLIENT_ITEMS ) {
							SAFE_WRITE( socket_number, "ITEM %d", item_number );
							item_number++;
						} else if ( game_stage == CLIENT_TANK_POSITION ) {
							SAFE_WRITE( socket_number, "TANKPOSITION %d", tank_position );
							tank_position++;
							if ( tank_position >= env.numGamePlayers ) {
								tank_position = 0;
							}
						} else if ( game_stage == CLIENT_TANK_HEALTH ) {
							SAFE_WRITE( socket_number, "HEALTH %d", tank_health );
							tank_health++;
							if ( tank_health >= env.numGamePlayers ) {
								tank_health = 0;
							}
						} else if ( game_stage == CLIENT_TEAMS ) {
							SAFE_WRITE( socket_number, "TEAMS %d", team_number );
							team_number++;
						} else if ( game_stage == CLIENT_NAME ) {
							SAFE_WRITE( socket_number, "PLAYERNAME %d", name_number );
							name_number++;
						} else if ( game_stage == CLIENT_WEAPONS ) {
							SAFE_WRITE( socket_number, "WEAPON %d", weapon_number );
							weapon_number++;
						} else if ( game_stage == CLIENT_PLAYING ) {
							time_clock++;
							if ( time_clock > 1 ) // check positions every few inputs
							{
								time_clock = 0;
								if ( surface_x < env.screenWidth ) {
									game_stage = CLIENT_SURFACE;
									SAFE_WRITE( socket_number, "SURFACE %d", surface_x );
									surface_x++;
								} else {
									game_stage    = CLIENT_TANK_POSITION;
									tank_position = 1;
									SAFE_WRITE( socket_number, "TANKPOSITION %d", 0 );
								} // game stage stuff
							}
						} // end of playing commands
					}

				} // end of we got something besides the close command

			} else // connection was broken
			{
				close( socket_number );
				printf( "Server closed connection.\n" );
				end_of_round = true;
			}
		}

		class_ = 0;
		while ( class_ < CLASS_COUNT ) {
			if ( CLASS_TANK == class_ ) {
				++class_;
				continue;
			}

			global.getHeadOfClass( static_cast< eClass >( class_ ), &my_object );
			while ( my_object ) {
				my_object->getNext( &next_obj );

				if ( CLASS_EXPLOSION == class_ ) {
					dynamic_cast< EXPLOSION * >( my_object )->explode();
				}

				my_object->applyPhysics();

				if ( my_object->destroy ) {
					my_object->requireUpdate();
					my_object->update();
					delete my_object;
					if ( CLASS_TELEPORT == class_ ) {
						time_clock = 2;
					}
				}

				if ( ( CLASS_BEAM == class_ ) || ( CLASS_MISSILE == class_ ) || ( CLASS_EXPLOSION == class_ )
				     || ( CLASS_TELEPORT == class_ ) ) {
					stuff_going_down = true;
				}

				my_object = next_obj;
			}
			++class_;
		}

		global.slideLand();

		// update everything on the screen
		if ( global.updateMenu ) {
			draw_top_bar();
		}

		if ( screen_update ) {
			screen_update = false;
			global.make_fullUpdate();
		}
		global.replace_canvas();

		screen_update = true;

		class_        = 0;
		while ( class_ < CLASS_COUNT ) {
			global.getHeadOfClass( static_cast< eClass >( class_ ), &my_object );
			while ( my_object ) {
				my_object->draw();
				if ( CLASS_FLOATTEXT == class_ ) {
					my_object->requireUpdate();
				}
				my_object->update();
				my_object->getNext( &my_object );
			}
			++class_;
		}

		global.do_updates();


		// check for input from the user
		if ( keypressed() ) {
			my_key = readkey();
			my_key = my_key >> 8;
			if ( my_key == KEY_SPACE ) {
				Client_Fire( global.client_player, socket_number );
				fired = true;
			} else if ( my_key == KEY_ESC ) {
				end_of_round = true;
				close( socket_number );
			} else if ( my_key == KEY_UP ) {
				Client_Power( global.client_player, CLIENT_UP );
			} else if ( my_key == KEY_DOWN ) {
				Client_Power( global.client_player, CLIENT_DOWN );
			} else if ( my_key == KEY_LEFT ) {
				Client_Angle( global.client_player, CLIENT_LEFT );
			} else if ( my_key == KEY_RIGHT ) {
				Client_Angle( global.client_player, CLIENT_RIGHT );
			} else if ( ( my_key == KEY_Z ) || ( my_key == KEY_BACKSPACE ) ) {
				Client_Cycle_Weapon( global.client_player, CYCLE_BACK );
			} else if ( ( my_key == KEY_C ) || ( my_key == KEY_TAB ) ) {
				Client_Cycle_Weapon( global.client_player, CYCLE_FORWARD );
				global.updateMenu = true;
			}

			screen_update     = false;
			global.updateMenu = true;
		}

		// pause for a moment
		// if (game_stage < CLIENT_PLAYING)
		if ( stuff_going_down ) {
			LINUX_SLEEP;
			stuff_going_down = false;
		}
	}

	// we should clean up here
	for ( count = 0; count < env.numGamePlayers; count++ ) {
		if ( env.players[ count ]->tank ) {
			delete env.players[ count ]->tank;
			env.players[ count ]->tank = nullptr;
		}
	}

	return keep_playing;
}


#endif // NETWORK
