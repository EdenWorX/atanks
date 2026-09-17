#ifndef GLOBALDATA_DEFINE
#define GLOBALDATA_DEFINE
/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2003  Thomas Hudson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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

	explicit GLOBALDATA();
	~GLOBALDATA();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */
	void addLandSlide( int32_t left, int32_t right, bool do_lock );

	void addLandSlide( double left, double right, bool do_lock ) { addLandSlide( ROUND( left ), ROUND( right ), do_lock ); }

	void addObject( vobj_t* object );
	bool areTanksInBox( int32_t x1, int32_t y1, int32_t x2, int32_t y2 );
	bool areTanksInBox( double x1, double y1, double x2, double y2 );
	void clear_objects();
	void destroy();
	void do_updates();
	void first_init();
	void free_debris_item( item_t* item );
	int32_t                          get_command();
	TANK*                            get_curr_tank();
	item_t*                          get_debris_item( int32_t radius );
	TANK*                            get_next_tank( bool* wrapped_around );
	TANK*                            get_random_tank();
	void                             initialise();
	bool                             isCloseBtnPressed();
	void                             lockClass( eClass class_ );
	void                             lockLand();
	void                             make_bgupdate( int32_t x, int32_t y, int32_t w, int32_t h );
	void                             make_fullUpdate();
	void                             make_update( int32_t x, int32_t y, int32_t w, int32_t h );
	void                             newRound();
	void                             pressCloseButton();
	void                             removeObject( vobj_t* object );
	void                             removeTank( TANK* tank );
	void                             replace_canvas();
	void                             set_curr_tank( TANK* tank_ );
	void                             set_command( int32_t cmd );
	void                             slideLand();
	void                             unlockClass( eClass class_ );
	void                             unlockLand();
	void                             unlockLandSlide( int32_t left, int32_t right );

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
	[[nodiscard]] int32_t get_avg_bgcolor( int32_t x1, int32_t y1, int32_t x2, int32_t y2, double xv, double yv ) const;
	[[nodiscard]] bool    isDirtInBox( int32_t x1, int32_t y1, int32_t x2, int32_t y2 ) const;

	[[nodiscard]] bool    isDirtInBox( double x1, double y1, double x2, double y2 ) const {
                return isDirtInBox( ROUND( x1 ), ROUND( y1 ), ROUND( x2 ), ROUND( y2 ) );
	}

	/* Compatibility function to not use old configurations */
	static void load_from_file( FILE* file );

	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	int32_t     AI_clock{ -1 };
	BITMAP*     canvas{ nullptr };
	char const* client_message{ nullptr }; // message sent from client to main menu
	PLAYER*     client_player{ nullptr };  // the index we use to know which one is the player on the client side
	int32_t     curland{ 0 };
	int32_t     current_drawing_mode{ DRAW_MODE_SOLID };
	uint32_t    currentround{ 0 };
	int32_t     cursky{ 0 };
	bool        demo_mode{ false };
	bool        hasTooMuchDeco{ false }; // Set to true if the set FPS are too hard to reach.
	BOX*        lastUpdates{ nullptr };
	int32_t     lastUpdatesCount{ 0 };
	double      lastwind{ 0. };
	int32_t     naturals_activated{ 0 };
	int32_t     numTanks{ 0 };
	TANK*       order[ MAXPLAYERS ]{ nullptr };
	bool        showScoreBoard{ false };
	bool        skippingComputerPlay{ false };
	int32_t     stage{ STAGE_AIM };
	bool        stopwindow{ false };
	ai32_t*     surface{ nullptr };
	char        tank_status[ 128 ]{ 0x0 };
	int32_t     tank_status_colour{ BLACK };
	BITMAP*     terrain{ nullptr };
	bool        updateMenu{ true };
	BOX*        updates{ nullptr };
	char*       update_string{ nullptr };
	int32_t     used_voices{ 0 };
	double      wind{ 0. };


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
