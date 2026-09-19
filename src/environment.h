#ifndef ENVIRONMENT_DEFINE
#define ENVIRONMENT_DEFINE 1

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


#include "box.h"
#include "gfxData.h"
#include "main.h"
#include "network.h"
#include "text.h"
#include "wrap_dirent.h"


// As everything depends on environment.h, PLAYER, TANK and VIRTUAL_OBJECT
// Must be forwarded here, and included before the ENVIRONMENT definition
class VIRTUAL_OBJECT;
class TANK;
class PLAYER;


#ifndef MAX_GRAVITY_DELAY
#  define GRAVITY_DELAY     200
#  define MAX_GRAVITY_DELAY 3
#endif

#define SPRING_CHANGE 1.25
#define BOUNCE_CHANGE 0.90

#define GET_R( x )    ( ( x & 0xff0000 ) >> 16 )
#define GET_G( x )    ( ( x & 0x00ff00 ) >> 8 )
#define GET_B( x )    ( x & 0x0000ff )


// Something from externs.h can not be used via include
// due to circular dependencies.
#ifndef EXTERNS_H_COLORS_DECLARED
extern int32_t GREY;
extern int32_t GREEN;
#endif // EXTERNS_H_COLORS_DECLARED


// Defined in sound.cpp:
extern int32_t MAX_VOLUME_FACTOR;

/** @class ENVIRONMENT
 * @brief Fixed values of the current environment the game takes place in.
 *
 * This class holds all values and the corresponding methods that define
 * the gaming environment.
 *
 * This means that all values in here must be set on game round start and
 * must not change until the game round ends.
 *
 * So basically this class consolidates everything set up with the options
 * menu and by the game round initialization.
 *
 * Everything that can change between the game round start and the game round
 * end has to be managed by GLOBALDATA.
 **/
class ENVIRONMENT {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */
	explicit ENVIRONMENT(); ///< Construct the environment.
	~ENVIRONMENT();         ///< Destroy the environment.


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */
	void    addGamePlayer( PLAYER* player_ );                            ///< Register a player for the next round.
	PLAYER* createNewPlayer( char const* player_name );                  ///< Create a player with default settings.
	void    creditWinners( int32_t winner ) const;                       ///< Pay out round winnings.
	void    decreaseVolume();                                            ///< Lower the sound volume.
	void    deletePermPlayer( PLAYER* player_ );                         ///< Delete a permanent player.
	void    destroy();                                                   ///< Free assets; call before Allegro shutdown.
	void    find_config_dir();                                           ///< Locate or create the config directory.
	bool    find_data_dir();                                             ///< Locate the data directory.
	void    first_init();                                                ///< One-time initialization after creation.
	void    genItemsList();                                              ///< Build the shop stock index.
	int32_t getPlayerByName( char const* player_name ) const;            ///< Find a player index by name.
	void    increaseVolume();                                            ///< Raise the sound volume.
	void    initialise();                                                ///< Regular per-round initialization.
	bool    loadBackgroundMusic();                                       ///< Load background music.
	bool    loadBitmaps();                                               ///< Load bitmaps.
	bool    loadFonts();                                                 ///< Load fonts.
	bool    loadGameFiles();                                             ///< Load game files.
	void    load_from_file( FILE* file );                                ///< Read settings.
	bool    loadSounds();                                                ///< Load sounds.
	void    load_text_files();                                           ///< Load localized text.
	void    newRound();                                                  ///< Reset per-round options.
	void    removeGamePlayer( PLAYER* player_ );                         ///< Unregister a round player.
	void    Reset_Options();                                             ///< Restore default options.
	bool    save_to_file( FILE* file );                                  ///< Write settings.
	bool    sendToClients( char const* message ) const;                  ///< Send a short message to all network clients.
	void    set_fps( int32_t new_FPS );                                  ///< Set the target frame rate.
	void    window_update( int32_t x, int32_t y, int32_t w, int32_t h ); ///< Extend the window dirty rectangle.

	/* Special questioning getters */
	[[nodiscard]] int32_t ingamemenu() const;                       ///< Run the in-game menu.
	[[nodiscard]] bool    isItemAvailable( int32_t itemNum ) const; ///< Test whether an item is sold this round.


	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	PLAYER**     allPlayers               = nullptr;                    ///< All known players.
	int32_t      availableItems[ THINGS ] = { 0x0 };                    ///< Shop stock index.
	SAMPLE*      background_music         = nullptr;                    ///< Menu background music.
	char**       bitmap_filenames         = nullptr;                    ///< Custom bitmap names.
	int32_t      boxedMode                = BM_OFF;                     ///< Boxed playfield mode.
	BITMAP**     button                   = nullptr;                    ///< Button bitmaps.
	bool         campaign_mode            = false;                      ///< Campaign mode active.
	double       campaign_rounds          = 0.;                         ///< Campaign round count.
	bool         check_for_updates        = true;                       ///< Update checker enabled.
	int32_t      colourDepth              = 0;                          ///< Video color depth.
	int32_t      colourTheme              = CT_CRISPY;                  ///< Land and sky gradient theme.
	string       configDir;                                             ///< Config directory path.
	int32_t      current_wallType  = 0;                                 ///< Wall type of the current round.
	int32_t      custom_background = 0;                                 ///< Custom menu background enabled.
	string       dataDir;                                               ///< Data directory path.
	int32_t      debris_level       = 1;                                ///< Debris density.
	bool         detailedLandscape  = false;                            ///< Detailed landscape enabled.
	bool         detailedSky        = false;                            ///< Detailed sky enabled.
	bool         ditherGradients    = true;                             ///< Dithered gradients enabled.
	bool         divide_money       = false;                            ///< Share team money with teammates.
	bool         do_box_wrap        = false;                            ///< Wrap the boxed playfield.
	bool         drawBackground     = true;                             ///< Draw the menu background.
	bool         dynamicMenuBg      = true;                             ///< Animated menu background.
	bool         fadingText         = false;                            ///< Fading menu text.
	double       fall_vector        = 0.;                               ///< Will be gravity * FPS_mod.
	int32_t      falling_dirt_balls = 0;                                ///< Falling dirt ball count.
	int32_t      fog                = 0;                                ///< Fog density.
	int32_t      fontHeight         = 0;                                ///< Fixed in ctor, no calls to text_height(font) needed.
	double       FPS_mod            = 0.;                               ///< Pre-calculated, used in many places.
	int32_t      frames_per_second  = 0;                                ///< Measured frame rate.
	int32_t      full_screen        = FULL_SCREEN_FALSE;                ///< Fullscreen mode.
	string       game_name;                                             ///< Game name.
	sGfxData     gfxData;                                               ///< Generated gradient data.
	double       gravity                   = 0.15;                      ///< Gravity constant.
	int32_t      halfHeight                = DEFAULT_SCREEN_HEIGHT / 2; ///< Half screen height.
	int32_t      halfWidth                 = DEFAULT_SCREEN_WIDTH / 2;  ///< Half screen width.
	double       interest                  = 1.25;                      ///< Bank interest rate.
	int32_t      itemtechLevel             = 5;                         ///< Item tech level.
	bool         isBoxed                   = false;                     ///< Boxed playfield active.
	bool         isGameLoaded              = true;                      ///< A savegame was loaded.
	int32_t      landSlideDelay            = MAX_GRAVITY_DELAY;         ///< Landslide delay.
	int32_t      landSlideType             = SLIDE_GRAVITY;             ///< Landslide mode.
	int32_t      landType                  = LAND_RANDOM;               ///< Landscape type.
	eLanguages   language                  = EL_ENGLISH;                ///< Interface language.
	int32_t      lightning                 = 0;                         ///< Lightning intensity.
	bool         loadGame                  = false;                     ///< Load a savegame.
	FONT*        main_font                 = nullptr;                   ///< Menu font.
	int32_t      maxFireTime               = 0;                         ///< Human aim time limit.
	int32_t      maxNumTanks               = 0;                         ///< Maximum tanks.
	double       maxVelocity               = 0.;                        ///< Maximum velocity.
	int32_t      max_screen_updates        = 64;                        ///< Dirty-rectangle budget.
	int32_t      menuBeginY                = 0;                         ///< Menu top.
	int32_t      menuEndY                  = 0;                         ///< Menu bottom.
	int32_t      meteors                   = 0;                         ///< Meteor count.
	BITMAP**     misc                      = nullptr;                   ///< Misc bitmaps.
	BITMAP**     missile                   = nullptr;                   ///< Missile bitmaps.
	int32_t      mouseclock                = 0;                         ///< Shop menu idle timer.
	bool         nameAboveTank             = true;                      ///< Show names above tanks.
	bool         network_enabled           = false;                     ///< Network play enabled.
	int32_t      network_port              = DEFAULT_NETWORK_PORT;      ///< Network port.
	double       nextCampaignRound         = 0;                         ///< When AI players will be raised next.
	int32_t      numAvailable              = 0;                         ///< Shop stock count.
	int32_t      numGamePlayers            = 0;                         ///< Round player count.
	int32_t      numHumanPlayers           = 0;                         ///< Human player count.
	int32_t      numPermanentPlayers       = 0;                         ///< Permanent player count.
	int32_t      number_of_bitmaps         = 0;                         ///< Custom bitmap count.
	bool         osMouse                   = true;                      ///< Whether we should use the OS or custom mouse.
	bool         play_music                = true;                      ///< Background music enabled.
	PLAYER**     players                   = nullptr;                   ///< Round players.
	PLAYER*      playerOrder[ MAXPLAYERS ] = { nullptr };               ///< Ordered round players.
	uint32_t     rounds                    = 5;                         ///< Round count.
	int32_t      satellite                 = 0;                         ///< Satellite laser size.
	uint32_t     saved_gameindex           = 0;                         ///< Selected savegame.
	char const** saved_game_list           = nullptr;                   ///< Savegame names.
	uint32_t     saved_game_list_size      = 0;                         ///< Savegame name count.
	int32_t      scoreHitUnit              = 75;                        ///< Score per unit hit.
	int32_t      scoreRoundWinBonus        = 10000;                     ///< Score bonus per round win.
	int32_t      scoreSelfHit              = 25;                        ///< Score penalty per self hit.
	int32_t      scoreTeamHit              = 10;                        ///< Score penalty per team hit.
	int32_t      scoreUnitDestroyBonus     = 5000;                      ///< Score bonus per unit destroyed.
	int32_t      scoreUnitSelfDestroy      = 2500;                      ///< Score penalty per self destruction.
	int32_t      screenHeight              = DEFAULT_SCREEN_HEIGHT;     ///< Screen height.
	int32_t      screenWidth               = DEFAULT_SCREEN_WIDTH;      ///< Screen width.
	double       sellpercent               = 0.80;                      ///< Shop sell rate.
	char         server_name[ 129 ]        = { 0x0 };                   ///< Server name.
	char         server_port[ 129 ]        = { 0x0 };                   ///< Server port.
	bool         shadowedText              = true;                      ///< Shadowed menu text.
	bool         showAIFeedback            = true;                      ///< Show AI feedback.
	bool         showFPS                   = false;                     ///< Show frame rate.
	int32_t      skipComputerPlay          = SKIP_HUMANS_DEAD;          ///< Skip AI turns mode.
	BITMAP*      sky                       = nullptr;                   ///< Sky bitmap.
	double       slope[ 360 ][ 2 ]         = { { 0x0 } };               ///< Ballistics direction table.
	int32_t      sound_driver              = SD_AUTODETECT;             ///< Sound driver.
	bool         sound_enabled             = true;                      ///< Sound enabled.
	SAMPLE**     sounds                    = nullptr;                   ///< Sound effects.
	int32_t      startmoney                = 15000;                     ///< Starting cash.
	BITMAP**     stock                     = nullptr;                   ///< Stock bitmaps.
	bool         swayingText               = true;                      ///< Swaying menu text.
	BITMAP**     tank                      = nullptr;                   ///< Tank bitmaps.
	BITMAP**     tankgun                   = nullptr;                   ///< Tank gun bitmaps.
	int32_t      temp_screenHeight         = 0;                         ///< 0 to detect command line arguments.
	int32_t      temp_screenWidth          = 0;                         ///< Versus loaded configuration.
	int32_t      time_to_fall              = 0;                         ///< Amount of time dirt will hover.
	BITMAP**     title                     = nullptr;                   ///< Title bitmaps.
	int32_t      turntype                  = TURN_RANDOM;               ///< Turn order mode.
	double       viscosity                 = 0.5;                       ///< Viscosity constant.
	int32_t      violent_death             = 0;                         ///< Violent death level.
	int32_t      voices                    = 0;                         ///< Reserved sound voices.
	int32_t      volley_delay              = 10;                        ///< Delay factor for volley shots, 5-50.
	int32_t      volume_factor             = MAX_VOLUME_FACTOR;         ///< Volume scaling factor.
	int32_t      wallColour                = GREEN;                     ///< Wall color.
	int32_t      wallType                  = WALL_RUBBER;               ///< Wall type.
	int32_t      weapontechLevel           = 5;                         ///< Weapon tech level.
	BOX          window;                                                ///< Main window box.
	int32_t      windstrength  = 8;                                     ///< Wind strength.
	int32_t      windvariation = 1;                                     ///< Wind variation.

	// Text structures holding (translated) lines of in game text
	TEXTBLOCK* gloat        = nullptr; ///< Gloating lines.
	TEXTBLOCK* ingame       = nullptr; ///< In-game lines.
	TEXTBLOCK* instructions = nullptr; ///< Instruction lines.
	TEXTBLOCK* panic        = nullptr; ///< Panic lines.
	TEXTBLOCK* kamikaze     = nullptr; ///< Kamikaze lines.
	TEXTBLOCK* retaliation  = nullptr; ///< Retaliation lines.
	TEXTBLOCK* revenge      = nullptr; ///< Revenge lines.
	TEXTBLOCK* suicide      = nullptr; ///< Suicide lines.
	TEXTBLOCK* war_quotes   = nullptr; ///< War quotes.


private:
	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	DIR* music_dir = nullptr;
};

#define HAS_ENVIRONMENT 1

#endif // ENVIRONMENT_DEFINE
