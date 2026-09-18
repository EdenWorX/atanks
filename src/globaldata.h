#ifndef GLOBALDATA_DEFINE
#define GLOBALDATA_DEFINE
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


#include "box.h"
#include "environment.h"
#include "globaltypes.h"
#include "main.h"
#include "text.h"
#include "wrap_dirent.h"

#include <atomic>

#include <sys/types.h>


#ifdef USE_MUTEX_INSTEAD_OF_SPINLOCK
#  include <mutex>
#  define CSpinLock std::mutex
#else
#  include "spinlock.h"
#endif // USE_MUTEX_INSTEAD_OF_SPINLOCK


#ifndef EXTERNS_H_COLORS_DECLARED
extern int32_t BLACK;
#  define GLOBADATA_BLACK_DECLARED 1
#endif // EXTERNS_H_COLORS_DECLARED


/// Forwards that do not need to be known here
struct sDebrisItem;
struct sDebrisPool;
class PLAYER;
class TANK;
class VIRTUAL_OBJECT;

/** @class GLOBALDATA
 * @brief Values used globally during a game round.
 *
 * This class holds all values and the corresponding functions for everything
 * that can change during a game round.
 *
 * Everything that is fixed during a game round is consolidated in ENVIRONMENT.
 **/
class GLOBALDATA {
	typedef VIRTUAL_OBJECT vobj_t;
	typedef sDebrisItem    item_t;

public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Zero-initialize the round state.
	explicit GLOBALDATA();
	/// Free the round state.
	~GLOBALDATA();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */
	/// Queue a landslide between two surface columns.
	void addLandSlide( int32_t left, int32_t right, bool do_lock );

	/// Queue a landslide; coordinates are rounded.
	void addLandSlide( double left, double right, bool do_lock ) { addLandSlide( ROUND( left ), ROUND( right ), do_lock ); }

	/// Append an object to its class list.
	void addObject( vobj_t* object );
	/// Test whether any tank overlaps a box.
	bool areTanksInBox( int32_t x1, int32_t y1, int32_t x2, int32_t y2 );
	/// Test whether any tank overlaps a box; coordinates are rounded.
	bool areTanksInBox( double x1, double y1, double x2, double y2 );
	/// Delete all game objects.
	void clear_objects();
	/// Release all round resources.
	void destroy();
	/// Apply queued screen updates.
	void do_updates();
	/// Allocate update buffers; runs once.
	void first_init();
	/// Return a debris item to the pool.
	void free_debris_item( item_t* item );
	int32_t                          get_command(); ///< Read the pending menu command.
	TANK*                            get_curr_tank(); ///< Read the tank whose turn it is.
	item_t*                          get_debris_item( int32_t radius ); ///< Take a blast-sized debris item from the pool.
	TANK*                            get_next_tank( bool* wrapped_around ); ///< Advance to the next live tank.
	TANK*                            get_random_tank(); ///< Pick a random live tank.
	void                             initialise(); ///< Clear objects and reset per-round state.
	bool                             isCloseBtnPressed(); ///< Read the close-button flag.
	void                             lockClass( eClass class_ ); ///< Lock an object-class list.
	void                             lockLand(); ///< Lock the terrain.
	void                             make_bgupdate( int32_t x, int32_t y, int32_t w, int32_t h ); ///< Queue a background dirty rectangle.
	void                             make_fullUpdate(); ///< Queue a full-screen update.
	void                             make_update( int32_t x, int32_t y, int32_t w, int32_t h ); ///< Queue a foreground dirty rectangle.
	void                             newRound(); ///< Clear temporary objects and reset round counters.
	void                             pressCloseButton(); ///< Set the close-button flag.
	void                             removeObject( vobj_t* object ); ///< Unlink an object from its class list.
	void                             removeTank( TANK* tank ); ///< Remove a tank from the turn order.
	void                             replace_canvas(); ///< Repaint the canvas after a video mode change.
	void                             set_curr_tank( TANK* tank_ ); ///< Set the tank whose turn it is.
	void                             set_command( int32_t cmd ); ///< Store the pending menu command.
	void                             slideLand(); ///< Apply queued landslides to the terrain.
	void                             unlockClass( eClass class_ ); ///< Unlock an object-class list.
	void                             unlockLand(); ///< Unlock the terrain.
	void                             unlockLandSlide( int32_t left, int32_t right ); ///< Release a landslide region lock.

	/// Fetch the locked head of an object-class list.
	template< typename Head_T > void getHeadOfClass( eClass class_, Head_T** head_ ) {
		if ( class_ < CLASS_COUNT ) {
			objLocks[ class_ ].lock();
			*head_ = static_cast< Head_T* >( heads[ class_ ] );
			objLocks[ class_ ].unlock();
		} else {
			*head_ = nullptr;
		}
	}

	/* Special Status and Information Getters */
	/// Average background color of a motion-compensated box.
	[[nodiscard]] int32_t get_avg_bgcolor( int32_t x1, int32_t y1, int32_t x2, int32_t y2, double xv, double yv ) const;
	/// Test whether a box holds terrain.
	[[nodiscard]] bool    isDirtInBox( int32_t x1, int32_t y1, int32_t x2, int32_t y2 ) const;

	/// Test whether a box holds terrain; coordinates are rounded.
	[[nodiscard]] bool    isDirtInBox( double x1, double y1, double x2, double y2 ) const {
                return isDirtInBox( ROUND( x1 ), ROUND( y1 ), ROUND( x2 ), ROUND( y2 ) );
	}

	/* Compatibility function to not use old configurations */
	/// Consume obsolete configuration fields.
	static void load_from_file( FILE* file );

	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	int32_t     AI_clock{ -1 };                            ///< Elapsed AI thinking time.
	BITMAP*     canvas{ nullptr };                         ///< Main drawing canvas.
	char const* client_message{ nullptr };                 ///< Message sent from client to main menu.
	PLAYER*     client_player{ nullptr };                  ///< Player on the client side.
	int32_t     curland{ 0 };                              ///< Current land gradient index.
	int32_t     current_drawing_mode{ DRAW_MODE_SOLID };   ///< Active Allegro drawing mode.
	uint32_t    currentround{ 0 };                         ///< Current round number.
	int32_t     cursky{ 0 };                               ///< Current sky gradient index.
	bool        demo_mode{ false };                        ///< Demo (AI-only) mode active.
	bool        hasTooMuchDeco{ false };                   ///< Set to true if the set FPS are too hard to reach.
	BOX*        lastUpdates{ nullptr };                    ///< Dirty rectangles of the previous frame.
	int32_t     lastUpdatesCount{ 0 };                     ///< Dirty rectangle count of the previous frame.
	double      lastwind{ 0. };                            ///< Wind of the previous turn.
	int32_t     naturals_activated{ 0 };                   ///< Naturals triggered this round.
	int32_t     numTanks{ 0 };                             ///< Live tanks in the turn order.
	TANK*       order[ MAXPLAYERS ]{ nullptr };            ///< Turn order.
	bool        showScoreBoard{ false };                   ///< Scoreboard overlay requested.
	bool        skippingComputerPlay{ false };             ///< AI turns are fast-forwarded.
	int32_t     stage{ STAGE_AIM };                        ///< Current round stage.
	bool        stopwindow{ false };                       ///< Suppress window updates.
	ai32_t*     surface{ nullptr };                        ///< Per-column terrain heights.
	char        tank_status[ 128 ]{ 0x0 };                 ///< Status line text.
	int32_t     tank_status_colour{ BLACK };               ///< Status line color.
	BITMAP*     terrain{ nullptr };                        ///< Destructible terrain bitmap.
	bool        updateMenu{ true };                        ///< Menu needs redrawing.
	BOX*        updates{ nullptr };                        ///< Dirty rectangles queued for redraw.
	char*       update_string{ nullptr };                  ///< Legacy update-checker message.
	int32_t     used_voices{ 0 };                          ///< Audio voices currently playing.
	double      wind{ 0. };                                ///< Current wind strength.


private:
	typedef sDebrisPool debpool_t;


	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	// Combine make_update and make_bgupdate with safety checks
	void addUpdate( int32_t x, int32_t y, int32_t w, int32_t h, BOX* target, int32_t& target_count ) const;


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	bool       close_button_pressed{ false };
	CSpinLock  cbpLock; //[c]lose_[b]utton_[p]ressed
	CSpinLock  cmdLock;
	bool       combineUpdates{ true };
	int32_t    command{ 0 };
	TANK*      currTank{ nullptr };
	debpool_t* debris_pool{ nullptr };
	int8_t*    done{ nullptr };
	double*    dropIncr{ nullptr };
	int32_t*   dropTo{ nullptr };
	int32_t*   fp{ nullptr };
	vobj_t*    heads[ CLASS_COUNT ]{ nullptr };
	CSpinLock  landLock;
	CSpinLock  objLocks[ CLASS_COUNT ];
	vobj_t*    tails[ CLASS_COUNT ]{ nullptr };
	int32_t    tankindex{ 0 };
	int32_t    updateCount{ 0 };
	double*    velocity{ nullptr };
};

#define HAS_GLOBALDATA 1

#endif
