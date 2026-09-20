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
class CPlayer;
class CTank;
class CVirtualObject;

/** @class CGlobalData
 * @brief Values used globally during a game round.
 *
 * This class holds all values and the corresponding functions for everything
 * that can change during a game round.
 *
 * Everything that is fixed during a game round is consolidated in CEnvironment.
 **/
class CGlobalData {
	typedef CVirtualObject vobj_t;
	typedef sDebrisItem    item_t;

public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Zero-initialize the round state.
	explicit CGlobalData();
	/// Free the round state.
	~CGlobalData();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */
	/// Queue a landslide between two surface columns.
	void add_land_slide( int32_t left, int32_t right, bool do_lock );

	/// Queue a landslide; coordinates are rounded.
	void add_land_slide( double left, double right, bool do_lock ) { add_land_slide( ROUND( left ), ROUND( right ), do_lock ); }

	/// Append an object to its class list.
	void add_object( vobj_t* object );
	/// Test whether any tank overlaps a box.
	bool are_tanks_in_box( int32_t x1, int32_t y1, int32_t x2, int32_t y2 );
	/// Test whether any tank overlaps a box; coordinates are rounded.
	bool are_tanks_in_box( double x1, double y1, double x2, double y2 );
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
	int32_t                          get_command();                         ///< Read the pending menu command.
	CTank*                            get_curr_tank();                       ///< Read the tank whose turn it is.
	item_t*                          get_debris_item( int32_t radius );     ///< Take a pool debris item.
	CTank*                            get_next_tank( bool* wrapped_around ); ///< Advance to the next live tank.
	CTank*                            get_random_tank();                     ///< Pick a random live tank.
	void                             initialise();                          ///< Clear objects and reset per-round state.
	bool                             is_close_btn_pressed();                   ///< Read the close-button flag.
	void                             lock_class( EClass class_ );            ///< Lock an object-class list.
	void                             lock_land();                            ///< Lock the terrain.
	/// Queue a background dirty rectangle.
	void                             make_bgupdate( int32_t x, int32_t y, int32_t w, int32_t h );
	/// Queue a full-screen update.
	void                             make_full_update();                                         ///< Queue a full-screen update.
	void                             make_update( int32_t x, int32_t y, int32_t w, int32_t h ); ///< Queue a foreground dirty rectangle.
	void                             new_round(); ///< Clear objects, reset counters.
	void                             press_close_button();                                        ///< Set the close-button flag.
	void                             remove_object( vobj_t* object );                            ///< Unlink an object.
	void                             remove_tank( CTank* tank );                                  ///< Remove a tank from the turn order.
	void                             replace_canvas();                                          ///< Repaint after mode change.
	void                             set_curr_tank( CTank* tank_ );                              ///< Set the tank whose turn it is.
	void                             set_command( int32_t cmd );                                ///< Store the pending menu command.
	void                             slide_land();                                               ///< Apply queued landslides.
	void                             unlock_class( EClass class_ );                              ///< Unlock an object-class list.
	void                             unlock_land();                                              ///< Unlock the terrain.
	void                             unlock_land_slide( int32_t left, int32_t right );            ///< Release a landslide region lock.

	/// Fetch the locked head of an object-class list.
	template< typename head_t > void get_head_of_class( EClass class_, head_t** head_ ) {
		if ( class_ < CLASS_COUNT ) {
			obj_locks[ class_ ].lock();
			*head_ = static_cast< head_t* >( heads[ class_ ] );
			obj_locks[ class_ ].unlock();
		} else {
			*head_ = nullptr;
		}
	}

	/* Special Status and Information Getters */
	/// Average background color of a motion-compensated box.
	[[nodiscard]] int32_t get_avg_bgcolor( int32_t x1, int32_t y1, int32_t x2, int32_t y2, double xv, double yv ) const;
	/// Test whether a box holds terrain.
	[[nodiscard]] bool    is_dirt_in_box( int32_t x1, int32_t y1, int32_t x2, int32_t y2 ) const;

	/// Test whether a box holds terrain; coordinates are rounded.
	[[nodiscard]] bool    is_dirt_in_box( double x1, double y1, double x2, double y2 ) const {
                return is_dirt_in_box( ROUND( x1 ), ROUND( y1 ), ROUND( x2 ), ROUND( y2 ) );
	}

	/* Compatibility function to not use old configurations */
	/// Consume obsolete configuration fields.
	static void load_from_file( FILE* file );

	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	int32_t     ai_clock{ -1 };                          ///< Elapsed AI thinking time.
	BITMAP*     canvas{ nullptr };                       ///< Main drawing canvas.
	char const* client_message{ nullptr };               ///< Message sent from client to main menu.
	CPlayer*     client_player{ nullptr };                ///< Player on the client side.
	int32_t     cur_land{ 0 };                            ///< Current land sGradient index.
	int32_t     current_drawing_mode{ DRAW_MODE_SOLID }; ///< Active Allegro drawing mode.
	uint32_t    current_round{ 0 };                       ///< Current round number.
	int32_t     cur_sky{ 0 };                             ///< Current sky sGradient index.
	bool        demo_mode{ false };                      ///< Demo (AI-only) mode active.
	bool        has_too_much_deco{ false };                 ///< Set to true if the set FPS are too hard to reach.
	sBox*        last_updates{ nullptr };                  ///< Dirty rectangles of the previous frame.
	int32_t     last_updates_count{ 0 };                   ///< Dirty rectangle count of the previous frame.
	double      lastwind{ 0. };                          ///< Wind of the previous turn.
	int32_t     naturals_activated{ 0 };                 ///< Naturals triggered this round.
	int32_t     num_tanks{ 0 };                           ///< Live tanks in the turn order.
	CTank*       order[ MAXPLAYERS ]{ nullptr };          ///< Turn order.
	bool        show_score_board{ false };                 ///< Scoreboard overlay requested.
	bool        skipping_computer_play{ false };           ///< AI turns are fast-forwarded.
	int32_t     stage{ STAGE_AIM };                      ///< Current round stage.
	bool        stop_window{ false };                     ///< Suppress window updates.
	ai32_t*     surface{ nullptr };                      ///< Per-column terrain heights.
	char        tank_status[ 128 ]{ 0x0 };               ///< Status line text.
	int32_t     tank_status_colour{ BLACK };             ///< Status line color.
	BITMAP*     terrain{ nullptr };                      ///< Destructible terrain bitmap.
	bool        update_menu{ true };                      ///< CMenu needs redrawing.
	sBox*        updates{ nullptr };                      ///< Dirty rectangles queued for redraw.
	char*       update_string{ nullptr };                ///< Legacy update-checker message.
	int32_t     used_voices{ 0 };                        ///< Audio voices currently playing.
	double      wind{ 0. };                              ///< Current wind strength.


private:
	typedef sDebrisPool debpool_t;


	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	// Combine make_update and make_bgupdate with safety checks
	void add_update( int32_t x, int32_t y, int32_t w, int32_t h, sBox* target, int32_t& target_count ) const;


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	bool       close_button_pressed{ false };
	CSpinLock  cbp_lock; //[c]lose_[b]utton_[p]ressed
	CSpinLock  cmd_lock;
	bool       combine_updates{ true };
	int32_t    command{ 0 };
	CTank*      curr_tank{ nullptr };
	debpool_t* debris_pool{ nullptr };
	int8_t*    done{ nullptr };
	double*    drop_incr{ nullptr };
	int32_t*   drop_to{ nullptr };
	int32_t*   fp{ nullptr };
	vobj_t*    heads[ CLASS_COUNT ]{ nullptr };
	CSpinLock  land_lock;
	CSpinLock  obj_locks[ CLASS_COUNT ];
	vobj_t*    tails[ CLASS_COUNT ]{ nullptr };
	int32_t    tank_index{ 0 };
	int32_t    update_count{ 0 };
	double*    velocity{ nullptr };
};

#define HAS_GLOBALDATA 1

#endif
