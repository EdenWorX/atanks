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

#include "globaldata.h"

#include "debris_pool.h"
#include "files.h"
#include "player.h"
#include "random.h"
#include "sound.h"
#include "tank.h"

#include <cassert>

CGlobalData::CGlobalData() {
	// memset initialization, because Visual C++ 2013 can't do lists, yet.
	memset( order, 0, sizeof( CTank* ) * MAXPLAYERS );
	memset( tank_status, 0, sizeof( char ) * 128 );
	memset( heads, 0, sizeof( vobj_t* ) * CLASS_COUNT );
	memset( tails, 0, sizeof( vobj_t* ) * CLASS_COUNT );
}

CGlobalData::~CGlobalData() {
	this->destroy();
}

/// @brief goes through the columns from @a left to @a right and sets slide type according to @a do_lock
void CGlobalData::add_land_slide( int32_t left, int32_t right, bool do_lock ) {
	// Opt out soon if no landslide is to be done
	if ( ( SLIDE_NONE == env.landslide_type ) || ( SLIDE_TANK_ONLY == env.landslide_type ) ) {
		return;
	}

	int32_t minX = std::min( left, right );
	int32_t maxX = std::max( left, right );

	if ( minX < 1 ) {
		minX = 1;
	}
	if ( minX > ( env.screen_width - 1 ) ) {
		minX = env.screen_width - 1;
	}
	if ( maxX < 1 ) {
		maxX = 1;
	}
	if ( maxX > ( env.screen_width - 1 ) ) {
		maxX = env.screen_width - 1;
	}

	if ( do_lock ) {
		memset( &done[ minX ], 3, sizeof( char ) * ( maxX - minX + 1 ) );
	} else {
		memset( &done[ minX ], 2, sizeof( char ) * ( maxX - minX + 1 ) );
	}
}

void CGlobalData::add_object( vobj_t* object ) {
	if ( nullptr == object ) {
		return;
	}

	EClass class_ = object->getClass();

	obj_locks[ class_ ].lock();

	/// --- case 1: first of its kind ---
	if ( nullptr == tails[ class_ ] ) {
		heads[ class_ ] = object;
		tails[ class_ ] = object;
	}

	/// --- case 2: normal addition ---
	else {
		tails[ class_ ]->next = object;
		object->prev          = tails[ class_ ];
		tails[ class_ ]       = object;
	}

	obj_locks[ class_ ].unlock();
}

// Combine both make_update and make_bgupdate with safety checks for
// the dimensions. This reduces code duplication.
void CGlobalData::add_update( int32_t x, int32_t y, int32_t w, int32_t h, sBox* target, int32_t& target_count ) const {
	assert( target && "ERROR: add_update called with nullptr target!" );

	bool combined = false;

	assert( ( w > 0 ) && ( h > 0 ) ); // No zero/negative updates, please!

	int32_t left   = std::max( x - 1, 0 );
	int32_t top    = std::max( y - 1, 0 );
	int32_t right  = std::min( x + w + 1, env.screen_width );
	int32_t bottom = std::min( y + h + 1, env.screen_height );

	// If the update is outside the screen, it is not needed:
	if ( ( bottom <= 0 ) /* most common case */
	     || ( left >= env.screen_width ) || ( right <= 0 ) || ( top >= env.screen_height ) ) {
		return;
	}

	assert( ( left < right ) );
	assert( ( top < bottom ) );

	if ( combine_updates && target_count && ( target_count < env.max_screen_updates ) ) {
		// Re-purpose sBox::w as x2 and sBox::h as y2:
		sBox prev(
			target[ target_count - 1 ].x,
			target[ target_count - 1 ].y,
			target[ target_count - 1 ].x + target[ target_count - 1 ].w,
			target[ target_count - 1 ].y + target[ target_count - 1 ].h
		);
		sBox next( left, top, right, bottom );

		if ( ( next.w > ( prev.x - 3 ) ) && ( prev.w > ( next.x - 3 ) ) && ( next.h > ( prev.y - 3 ) )
		     && ( prev.h > ( next.y - 3 ) ) ) {
			next.set(
				next.x < prev.x ? next.x : prev.x,
				next.y < prev.y ? next.y : prev.y,
				next.w > prev.w ? next.w : prev.w,
				next.h > prev.h ? next.h : prev.h
			);
			// recalculate x2/y2 back into w/h
			target[ target_count - 1 ].set( next.x, next.y, next.w - next.x, next.h - next.y );

			// Make sure the target update is correct:
			assert( ( target[ target_count - 1 ].w > 0 ) && ( target[ target_count - 1 ].h > 0 ) );

			combined = true;
		}
	}

	if ( !combined ) {
		target[ target_count++ ].set( left, top, right - left, bottom - top );
	}

	if ( !stop_window && ( target_count <= env.max_screen_updates ) ) {
		env.window_update( left, top, right - left, bottom - top );
	}
}

// return true if any living tank is in the given box.
// left/right and top/bottom are determined automatically.
bool CGlobalData::are_tanks_in_box( int32_t x1, int32_t y1, int32_t x2, int32_t y2 ) {
	CTank* lt = dynamic_cast< CTank* >( heads[ CLASS_TANK ] );

	while ( lt ) {
		// Tank found, is it in the box?
		if ( ( !lt->destroy ) && lt->isInBox( x1, y1, x2, y2 ) ) {
			return true;
		}
		lt->getNext( &lt );
	}

	return false;
}

bool CGlobalData::are_tanks_in_box( double x1, double y1, double x2, double y2 ) {
	return are_tanks_in_box( ROUND( x1 ), ROUND( y1 ), ROUND( x2 ), ROUND( y2 ) );
}

/// @brief remove and delete *all* objects stored.
void CGlobalData::clear_objects() {
	int32_t class_ = 0;

	while ( class_ < CLASS_COUNT ) {
		while ( tails[ class_ ] ) {
			delete tails[ class_ ];
		}
		++class_;
	}
}

// Call before calling allegro_exit()!
void CGlobalData::destroy() {
	clear_objects();

	if ( debris_pool ) {
		delete debris_pool;
		debris_pool = nullptr;
	}

	if ( canvas ) {
		destroy_bitmap( canvas );
	}
	canvas = nullptr;
	if ( terrain ) {
		destroy_bitmap( terrain );
	}
	terrain = nullptr;

	delete[] done;
	done = nullptr;

	delete[] fp;
	fp = nullptr;

	delete[] surface;
	surface = nullptr;

	delete[] drop_to;
	drop_to = nullptr;

	delete[] velocity;
	velocity = nullptr;

	delete[] drop_incr;
	drop_incr = nullptr;

	delete[] updates;
	updates = nullptr;

	delete[] last_updates;
	last_updates = nullptr;
}

void CGlobalData::do_updates() {
	bool isBgUpdNeeded = last_updates_count > 0;

	acquire_bitmap( screen );
	for ( int32_t i = 0; i < update_count; ++i ) {
		blit( canvas,
		      screen,
		      updates[ i ].x,
		      updates[ i ].y,
		      updates[ i ].x,
		      updates[ i ].y,
		      updates[ i ].w,
		      updates[ i ].h );

		if ( isBgUpdNeeded ) {
			make_bgupdate( updates[ i ].x, updates[ i ].y, updates[ i ].w, updates[ i ].h );
		}
	}
	release_bitmap( screen );
	if ( !isBgUpdNeeded ) {
		last_updates_count = update_count;
		memcpy( last_updates, updates, sizeof( sBox ) * env.max_screen_updates );
	}
	update_count = 0;
}

// Do what has to be done after the game starts
void CGlobalData::first_init() {
	// get memory for updates
	try {
		updates = new sBox[ env.max_screen_updates ];
	} catch ( std::bad_alloc& e ) {
		cerr << "globaldata.cpp:" << __LINE__ << ":first_init() : "
		     << "Failed to allocate memory for updates [" << e.what() << "]" << endl;
		exit( 1 );
	}

	// get memory for last_updates
	try {
		last_updates = new sBox[ env.max_screen_updates ];
	} catch ( std::bad_alloc& e ) {
		cerr << "globaldata.cpp:" << __LINE__ << ":first_init() : "
		     << "Failed to allocate memory for last_updates [" << e.what() << "]" << endl;
		exit( 1 );
	}

	canvas = create_bitmap( env.screen_width, env.screen_height );
	if ( !canvas ) {
		cout << "Failed to create canvas bitmap: " << allegro_error << endl;
		exit( 1 );
	}

	terrain = create_bitmap( env.screen_width, env.screen_height );
	if ( !terrain ) {
		cout << "Failed to create terrain bitmap: " << allegro_error << endl;
		exit( 1 );
	}


	// get memory for the debris pool
	try {
		debris_pool = new sDebrisPool( env.max_screen_updates );
	} catch ( std::bad_alloc& e ) {
		cerr << "globaldata.cpp:" << __LINE__ << ":first_init() : "
		     << "Failed to allocate memory for debris_pool [" << e.what() << "]" << endl;
		exit( 1 );
	}


	try {
		done     = new int8_t[ env.screen_width ]{ 0 };
		fp       = new int32_t[ env.screen_width ]{ 0 };
		surface  = new ai32_t[ env.screen_width ]{ { 0 } };
		drop_to   = new int32_t[ env.screen_width ]{ 0 };
		velocity = new double[ env.screen_width ]{ 0 };
		drop_incr = new double[ env.screen_width ]{ 0 };
	} catch ( std::bad_alloc& e ) {
		cerr << "globaldata.cpp:" << __LINE__ << ":first_init() : "
		     << "Failed to allocate memory for base data arrays [" << e.what() << "]" << endl;
		exit( 1 );
	}

	initialise();
}

/** @brief delegate freeing of a debris item to the debris pool.
 *
 * This delegating function, instead of making the debris pool public,
 * exists as a point where locking, if it becomes necessary, can be
 * added without having to rewrite a lot of code.
 **/
void CGlobalData::free_debris_item( item_t* i ) {
	debris_pool->free_item( i );
}

int32_t CGlobalData::get_avg_bgcolor( int32_t x1, int32_t y1, int32_t x2, int32_t y2, double xv, double yv ) const {
	// Movement
	auto mvx      = ROUND( 10. * xv ); // eliminate slow movement
	auto mvy      = ROUND( 10. * yv ); // eliminate slow movement
	bool mv_left  = mvx < 0;
	bool mv_right = mvx > 0;
	bool mv_up    = mvy < 0;
	bool mv_down  = mvy > 0;

	// Boundaries
	int32_t min_x = 1;
	int32_t max_x = env.screen_width - 2;
	int32_t min_y = env.is_boxed ? MENUHEIGHT + 1 : MENUHEIGHT;
	int32_t max_y = env.screen_height - 2;

	// Coordinates
	int32_t left   = std::max( std::min( x1, x2 ), min_x );
	int32_t right  = std::min( std::max( x1, x2 ), max_x );
	int32_t centre = ( x1 + x2 ) / 2;
	int32_t top    = std::max( std::min( y1, y2 ), min_y );
	int32_t bottom = std::min( std::max( y1, y2 ), max_y );
	int32_t middle = ( y1 + y2 ) / 2;


	// Colors:
	int32_t col_tl, col_tc, col_tr; // top row
	int32_t col_ml, col_mc, col_mr; // middle row
	int32_t col_bl, col_bc, col_br; // bottom row
	int32_t r = 0, g = 0, b = 0;


	// Get Sky or Terrain colour, whatever fits:
	/*---------------------
	  --- Left side ---
	  ---------------------*/
	if ( PINK == ( col_tl = getpixel( terrain, left, top ) ) ) {
		col_tl = getpixel( env.sky, left, top );
	}
	if ( PINK == ( col_ml = getpixel( terrain, left, middle ) ) ) {
		col_ml = getpixel( env.sky, left, middle );
	}
	if ( PINK == ( col_bl = getpixel( terrain, left, bottom ) ) ) {
		col_bl = getpixel( env.sky, left, bottom );
	}

	/*---------------------
	  --- The Center ---
	---------------------*/
	if ( PINK == ( col_tc = getpixel( terrain, centre, top ) ) ) {
		col_tc = getpixel( env.sky, centre, top );
	}
	if ( PINK == ( col_mc = getpixel( terrain, centre, middle ) ) ) {
		col_mc = getpixel( env.sky, centre, middle );
	}
	if ( PINK == ( col_bc = getpixel( terrain, centre, bottom ) ) ) {
		col_bc = getpixel( env.sky, centre, bottom );
	}

	/*----------------------
	  --- Right side ---
	----------------------*/
	if ( PINK == ( col_tr = getpixel( terrain, right, top ) ) ) {
		col_tr = getpixel( env.sky, right, top );
	}
	if ( PINK == ( col_mr = getpixel( terrain, right, middle ) ) ) {
		col_mr = getpixel( env.sky, right, middle );
	}
	if ( PINK == ( col_br = getpixel( terrain, right, bottom ) ) ) {
		col_br = getpixel( env.sky, right, bottom );
	}


	// Fetch the rgb parts, according to movement:

	/* --- X-Movement --- */
	if ( mv_left ) {
		// Movement to the left, weight left side colour twice
		r += ( GET_R( col_tl ) + GET_R( col_ml ) + GET_R( col_bl ) ) * 2;
		g += ( GET_G( col_tl ) + GET_G( col_ml ) + GET_G( col_bl ) ) * 2;
		b += ( GET_B( col_tl ) + GET_B( col_ml ) + GET_B( col_bl ) ) * 2;
		// The others are counted once
		r += GET_R( col_tc ) + GET_R( col_mc ) + GET_R( col_bc ) + GET_R( col_tr ) + GET_R( col_mr ) + GET_R( col_br );
		g += GET_G( col_tc ) + GET_G( col_mc ) + GET_G( col_bc ) + GET_G( col_tr ) + GET_G( col_mr ) + GET_G( col_br );
		b += GET_B( col_tc ) + GET_B( col_mc ) + GET_B( col_bc ) + GET_B( col_tr ) + GET_B( col_mr ) + GET_B( col_br );
	} else if ( mv_right ) {
		// Movement to the right, weight right side colour twice
		r += ( GET_R( col_tr ) + GET_R( col_mr ) + GET_R( col_br ) ) * 2;
		g += ( GET_G( col_tr ) + GET_G( col_mr ) + GET_G( col_br ) ) * 2;
		b += ( GET_B( col_tr ) + GET_B( col_mr ) + GET_B( col_br ) ) * 2;
		// The others are counted once
		r += GET_R( col_tc ) + GET_R( col_mc ) + GET_R( col_bc ) + GET_R( col_tl ) + GET_R( col_ml ) + GET_R( col_bl );
		g += GET_G( col_tc ) + GET_G( col_mc ) + GET_G( col_bc ) + GET_G( col_tl ) + GET_G( col_ml ) + GET_G( col_bl );
		b += GET_B( col_tc ) + GET_B( col_mc ) + GET_B( col_bc ) + GET_B( col_tl ) + GET_B( col_ml ) + GET_B( col_bl );
	} else {
		// No x-movement, weight centre colour twice
		r += ( GET_R( col_tc ) + GET_R( col_mc ) + GET_R( col_bc ) ) * 2;
		g += ( GET_G( col_tc ) + GET_G( col_mc ) + GET_G( col_bc ) ) * 2;
		b += ( GET_B( col_tc ) + GET_B( col_mc ) + GET_B( col_bc ) ) * 2;
		// The others are counted once
		r += GET_R( col_tl ) + GET_R( col_ml ) + GET_R( col_bl ) + GET_R( col_tr ) + GET_R( col_mr ) + GET_R( col_br );
		g += GET_G( col_tl ) + GET_G( col_ml ) + GET_G( col_bl ) + GET_G( col_tr ) + GET_G( col_mr ) + GET_G( col_br );
		b += GET_B( col_tl ) + GET_B( col_ml ) + GET_B( col_bl ) + GET_B( col_tr ) + GET_B( col_mr ) + GET_B( col_br );
	}

	/* --- Y-Movement --- */
	if ( mv_up ) {
		// Movement upwards, weight top side colour twice
		r += ( GET_R( col_tl ) + GET_R( col_tc ) + GET_R( col_tr ) ) * 2;
		g += ( GET_G( col_tl ) + GET_G( col_tc ) + GET_G( col_tr ) ) * 2;
		b += ( GET_B( col_tl ) + GET_B( col_tc ) + GET_B( col_tr ) ) * 2;
		// The others are counted once
		r += GET_R( col_ml ) + GET_R( col_mc ) + GET_R( col_mr ) + GET_R( col_bl ) + GET_R( col_bc ) + GET_R( col_br );
		g += GET_G( col_ml ) + GET_G( col_mc ) + GET_G( col_mr ) + GET_G( col_bl ) + GET_G( col_bc ) + GET_G( col_br );
		b += GET_B( col_ml ) + GET_B( col_mc ) + GET_B( col_mr ) + GET_B( col_bl ) + GET_B( col_bc ) + GET_B( col_br );
	} else if ( mv_down ) {
		// Movement downwards, weight bottom side colour twice
		r += ( GET_R( col_bl ) + GET_R( col_bc ) + GET_R( col_br ) ) * 2;
		g += ( GET_G( col_bl ) + GET_G( col_bc ) + GET_G( col_br ) ) * 2;
		b += ( GET_B( col_bl ) + GET_B( col_bc ) + GET_B( col_br ) ) * 2;
		// The others are counted once
		r += GET_R( col_ml ) + GET_R( col_mc ) + GET_R( col_mr ) + GET_R( col_tl ) + GET_R( col_tc ) + GET_R( col_tr );
		g += GET_G( col_ml ) + GET_G( col_mc ) + GET_G( col_mr ) + GET_G( col_tl ) + GET_G( col_tc ) + GET_G( col_tr );
		b += GET_B( col_ml ) + GET_B( col_mc ) + GET_B( col_mr ) + GET_B( col_tl ) + GET_B( col_tc ) + GET_B( col_tr );
	} else {
		// No y-movement, weight middle colour twice
		r += ( GET_R( col_ml ) + GET_R( col_mc ) + GET_R( col_mr ) ) * 2;
		g += ( GET_G( col_ml ) + GET_G( col_mc ) + GET_G( col_mr ) ) * 2;
		b += ( GET_B( col_ml ) + GET_B( col_mc ) + GET_B( col_mr ) ) * 2;
		// The others are counted once
		r += GET_R( col_tl ) + GET_R( col_tc ) + GET_R( col_tr ) + GET_R( col_bl ) + GET_R( col_bc ) + GET_R( col_br );
		g += GET_G( col_tl ) + GET_G( col_tc ) + GET_G( col_tr ) + GET_G( col_bl ) + GET_G( col_bc ) + GET_G( col_br );
		b += GET_B( col_tl ) + GET_B( col_tc ) + GET_B( col_tr ) + GET_B( col_bl ) + GET_B( col_bc ) + GET_B( col_br );
	}


	/* I know this looks weird, but what we now have is some kind of summed
	 * matrix, which is always the same:
	 * Let's assume that xv and yv are both 0.0, so no movement is happening.
	 * The result is: (In counted times)
	 * 2|3|2  ( =  7)
	 * -+-+-
	 * 3|4|3  ( = 10)
	 * -+-+-
	 * 2|3|2  ( =  7)
	 *          = 24
	 * And it is always 24, no matter which movement combination you try
	 */

	r /= 24;
	g /= 24;
	b /= 24;

	return makecol( r > 0xff ? 0xff : r, g > 0xff ? 0xff : g, b > 0xff ? 0xff : b );
}

// Locks global->command for reading, reads value, then unlocks the variable
// and returns the value.
int32_t CGlobalData::get_command() {
	cmd_lock.lock();
	int32_t c = command;
	cmd_lock.unlock();
	return c;
}

CTank* CGlobalData::get_curr_tank() {
	return curr_tank;
}

/** @brief delegate getting a debris item to the debris pool.
 *
 * This delegating function, instead of making the debris pool public,
 * exists as a point where locking, if it becomes necessary, can be
 * added without having to rewrite a lot of code.
 **/
sDebrisItem* CGlobalData::get_debris_item( int32_t radius ) {
	return debris_pool->get_item( radius );
}

CTank* CGlobalData::get_next_tank( bool* wrapped_around ) {
	bool    found    = false;
	int32_t index    = tank_index + 1;
	int32_t oldindex = tank_index;
	int32_t wrapped  = 0;

	while ( !found && ( wrapped < 2 ) ) {
		if ( index >= MAXPLAYERS ) {
			index = 0;
			if ( wrapped_around ) {
				*wrapped_around = true;
			}
			wrapped++;
		}

		if ( order[ index ] && ( index != oldindex ) && !order[ index ]->destroy ) {
			found = true;
		} else {
			++index;
		}
	}

	tank_index = index;

	// If this tank is valid, the currently selected weapon must be checked
	// first and changed if depleted
	CTank* next_tank = order[ index ];
	if ( next_tank && next_tank->player ) {
		next_tank->check_weapon();
	}

	// Whatever happened, the status bar needs an update:
	if ( oldindex != index ) {
		update_menu = true;
	}

	return next_tank;
}

/// @brief randomly return one active tank
CTank* CGlobalData::get_random_tank() {
	int32_t idx      = get_rand() % MAXPLAYERS;
	int32_t attempts = 2;
	while ( ( !order[ idx ] || order[ idx ]->destroy ) && ( idx < MAXPLAYERS ) && attempts ) {
		if ( ++idx >= MAXPLAYERS ) {
			idx = 0;
			--attempts;
		}
	}

	return order[ idx ];
}

void CGlobalData::initialise() {
	clear_objects();
	num_tanks = 0;
	clear_to_color( canvas, WHITE );
	clear_to_color( terrain, PINK );

	for ( int32_t i = 0; i < env.screen_width; ++i ) {
		done[ i ]   = 0;
		drop_to[ i ] = env.screen_height - 1;
		fp[ i ]     = 0;
	}
}

// return true if the dirt reaches into the given box.
// left/right and top/bottom are determined automatically.
bool CGlobalData::is_dirt_in_box( int32_t x1, int32_t y1, int32_t x2, int32_t y2 ) const {
	int32_t top = std::max( std::min( y1, y2 ), env.is_boxed ? MENUHEIGHT + 1 : MENUHEIGHT );
	// Exit early if the box is below the playing area
	if ( top >= env.screen_height ) {
		return false;
	}

	int32_t bottom = std::min( std::max( y1, y2 ), env.screen_height - 2 );
	// Exit early if the box is over the playing area
	if ( bottom <= MENUHEIGHT ) {
		return false;
	}

	int32_t left  = std::max( std::min( x1, x2 ), 1 );
	int32_t right = std::min( std::max( x1, x2 ), env.screen_width - 2 );

	// If the box is outside the playing area, this loop won't do anything
	for ( int32_t x = left; x <= right; ++x ) {
		if ( surface[ x ].load( ATOMIC_READ ) <= bottom ) {
			return true;
		}
	}

	return false;
}

/// @return true if the close button was pressed
bool CGlobalData::is_close_btn_pressed() {
	cbp_lock.lock();
	bool result = close_button_pressed;
	cbp_lock.unlock();

	return result;
}

/** @brief load global data from a file
 * This method is still present to provide backwards
 * compatibility with configurations that were saved
 * before the values were moved to CEnvironment
 **/
void CGlobalData::load_from_file( FILE* file ) {
	char  line[ MAX_CONFIG_LINE + 1 ]  = { 0 };
	char  field[ MAX_CONFIG_LINE + 1 ] = { 0 };
	char  value[ MAX_CONFIG_LINE + 1 ] = { 0 };
	char* result                       = nullptr;

	setlocale( LC_NUMERIC, "C" );

	// read until we hit the line "*GLOBAL*" or "***" or EOF
	do {
		result = fgets( line, MAX_CONFIG_LINE, file );
		if ( !result || !strncmp( line, "***", 3 ) ) {
			// eof OR end of record
			return;
		}
	} while ( strncmp( line, "*GLOBAL*", 8 ) != 0 );

	bool is_done = false;

	while ( result && !is_done ) {
		// read a line
		memset( line, '\0', MAX_CONFIG_LINE );
		if ( ( result = fgets( line, MAX_CONFIG_LINE, file ) ) ) {

			// if we hit end of the record, stop
			if ( !strncmp( line, "***", 3 ) ) {
				return;
			}

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
			strncpy( value, &( line[ equal_position + 1 ] ), MAX_CONFIG_LINE );


			// Values that were moved to CEnvironment:
			// They are loaded, for compatibility, but the next
			// save will put them into the correct section anyway.
			// So these can eventually be removed.
			if ( !strcasecmp( field, "acceleratedai" ) ) {
				SAFE_STOI( env.skip_computer_play, value );
				if ( env.skip_computer_play > SKIP_HUMANS_DEAD ) {
					env.skip_computer_play = SKIP_HUMANS_DEAD;
				}
			} else if ( !strcasecmp( field, "checkupdates" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				env.check_for_updates = val > 0;
			} else if ( !strcasecmp( field, "colourtheme" ) ) {
				SAFE_STOI( env.colour_theme, value );
				if ( env.colour_theme < CT_REGULAR ) {
					env.colour_theme = CT_REGULAR;
				}
				if ( env.colour_theme > CT_CRISPY ) {
					env.colour_theme = CT_CRISPY;
				}
			} else if ( !strcasecmp( field, "debrislevel" ) ) {
				SAFE_STOI( env.debris_level, value );
			} else if ( !strcasecmp( field, "detailedland" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				env.detailed_landscape = val > 0;
			} else if ( !strcasecmp( field, "detailedsky" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				env.detailed_sky = val > 0;
			} else if ( !strcasecmp( field, "dither" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				env.dither_gradients = val > 0;
			} else if ( !strcasecmp( field, "dividemoney" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				env.divide_money = val > 0;
			} else if ( !strcasecmp( field, "enablesound" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				env.sound_enabled = val > 0;
			} else if ( !strcasecmp( field, "frames" ) ) {
				int32_t new_fps = 0;
				SAFE_STOI( new_fps, value );
				env.set_fps( new_fps );
			} else if ( !strcasecmp( field, "fullscreen" ) ) {
				SAFE_STOI( env.full_screen, value );
			} else if ( !strcasecmp( field, "interest" ) ) {
				SAFE_STOD( env.interest, value );
			} else if ( !strcasecmp( field, "language" ) ) {
				uint32_t stored_lang = 0;
				SAFE_STOUL( stored_lang, value );
				env.language = static_cast< ELanguages >( stored_lang );
			} else if ( !strcasecmp( field, "listenport" ) ) {
				SAFE_STOI( env.network_port, value );
			} else if ( !strcasecmp( field, "maxfiretime" ) ) {
				SAFE_STOI( env.max_fire_time, value );
			} else if ( !strcasecmp( field, "networking" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				env.network_enabled = val > 0;
			} else if ( !strcasecmp( field, "numpermanentplayers" ) ) {
				SAFE_STOI( env.num_permanent_players, value );
			} else if ( !strcasecmp( field, "OSMOUSE" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				env.os_mouse = val > 0;
			} else if ( !strcasecmp( field, "playmusic" ) ) {
				int32_t val = 0;
				SAFE_STOI( val, value );
				env.play_music = val > 0;
			} else if ( !strcasecmp( field, "rounds" ) ) {
				SAFE_STOUL( env.rounds, value );
			} else if ( !strcasecmp( field, "screenwidth" ) && !env.temp_screen_width ) {
				SAFE_STOI( env.screen_width, value );
				env.half_width        = env.screen_width / 2;
				env.temp_screen_width = env.screen_width;
			} else if ( !strcasecmp( field, "screenheight" ) && !env.temp_screen_height ) {
				SAFE_STOI( env.screen_height, value );
				env.half_height        = env.screen_height / 2;
				env.temp_screen_height = env.screen_height;
			} else if ( !strcasecmp( field, "scorehitunit" ) ) {
				SAFE_STOI( env.scoreHitUnit, value );
			} else if ( !strcasecmp( field, "scoreselfhit" ) ) {
				SAFE_STOI( env.scoreSelfHit, value );
			} else if ( !strcasecmp( field, "scoreroundwinbonus" ) ) {
				SAFE_STOI( env.scoreRoundWinBonus, value );
			} else if ( !strcasecmp( field, "scoreteamhit" ) ) {
				SAFE_STOI( env.scoreTeamHit, value );
			} else if ( !strcasecmp( field, "scoreunitdestroybonus" ) ) {
				SAFE_STOI( env.scoreUnitDestroyBonus, value );
			} else if ( !strcasecmp( field, "scoreunitselfdestroy" ) ) {
				SAFE_STOI( env.scoreUnitSelfDestroy, value );
			} else if ( !strcasecmp( field, "sell_percent" ) ) {
				SAFE_STOD( env.sell_percent, value );
			} else if ( !strcasecmp( field, "sounddriver" ) ) {
				SAFE_STOI( env.sound_driver, value );
			} else if ( !strcasecmp( field, "start_money" ) ) {
				SAFE_STOI( env.start_money, value );
			} else if ( !strcasecmp( field, "turn_type" ) ) {
				SAFE_STOI( env.turn_type, value );
			} else if ( !strcasecmp( field, "violentdeath" ) ) {
				SAFE_STOI( env.violent_death, value );
			}
		} // end of read a line properly
	}         // end of while not is_done
}

void CGlobalData::lock_class( EClass class_ ) {
	obj_locks[ class_ ].lock();
}

void CGlobalData::lock_land() {
	land_lock.lock();
}

void CGlobalData::make_bgupdate( int32_t x, int32_t y, int32_t w, int32_t h ) {
	if ( last_updates_count >= env.max_screen_updates ) {
		make_full_update();
		return;
	}

	assert( ( w > 0 ) && ( h > 0 ) );

	if ( ( w > 0 ) && ( h > 0 ) ) {
		add_update( x, y, w, h, last_updates, last_updates_count );
	}
}

void CGlobalData::make_full_update() {
	// Replace Updates with a full-screen update:
	combine_updates   = false;
	update_count      = 0;
	last_updates_count = 0;

	// They are split into 2 x 2 updates:
	for ( int32_t x = 0; x < 2; ++x ) {
		add_update( env.half_width * x, 0, env.half_width, env.half_height, updates, update_count );
		add_update( env.half_width * x, env.half_height, env.half_width, env.half_height, updates, update_count );
		add_update( env.half_width * x, 0, env.half_width, env.half_height, last_updates, last_updates_count );
		add_update( env.half_width * x, env.half_height, env.half_width, env.half_height, last_updates, last_updates_count );
	}

	combine_updates = true;
}

void CGlobalData::make_update( int32_t x, int32_t y, int32_t w, int32_t h ) {
	if ( update_count >= env.max_screen_updates ) {
		make_full_update();
		return;
	}

	// These assertions should catch screwed updates that make no sense
	assert( ( h <= env.screen_height ) && ( w <= env.screen_width ) );
	assert( ( w > 0 ) && ( h > 0 ) );

	if ( ( h > 0 ) && ( w > 0 ) ) {
		add_update( x, y, w, h, updates, update_count );
	}
}

void CGlobalData::new_round() {
	if ( ( current_round > 0 ) && ( current_round-- < env.next_campaign_round ) ) {
		env.next_campaign_round -= env.campaign_rounds;
	}

	tank_index          = 0;
	naturals_activated = 0;
	combine_updates     = true;

	// clean all but texts and tanks
	int32_t class_ = 0;
	while ( class_ < CLASS_COUNT ) {
		if ( ( CLASS_FLOATTEXT != class_ ) && ( CLASS_TANK != class_ ) ) {
			while ( tails[ class_ ] ) {
				delete tails[ class_ ];
			}
		}
		++class_;
	}


	// Re-init land slide
	for ( int32_t i = 0; i < env.screen_width; ++i ) {
		done[ i ]   = 2; // Check at once
		drop_to[ i ] = env.screen_height - 1;
		fp[ i ]     = 0;
	}

	// Init order array
	for ( auto& i : order ) {
		i = nullptr;
	}
}

/// @brief Tell global that the close button was pressed
void CGlobalData::press_close_button() {
	cbp_lock.lock();
	close_button_pressed = true;
	cbp_lock.unlock();
	set_command( GLOBAL_COMMAND_QUIT );
}

void CGlobalData::remove_object( vobj_t* object ) {
	if ( nullptr == object ) {
		return;
	}

	EClass class_ = object->getClass();

	/// --- 1: Is the list empty? ---
	if ( nullptr == heads[ class_ ] ) {
		return;
	}

	obj_locks[ class_ ].lock();

	/// --- 2: If the object is head, set it anew:
	if ( object == heads[ class_ ] ) {
		heads[ class_ ] = object->next;
	}

	/// --- 4: If the object is tail, set it anew:
	if ( object == tails[ class_ ] ) {
		tails[ class_ ] = object->prev;
	}

	/// --- 5: Take it out of the list:
	if ( object->prev ) {
		object->prev->next = object->next;
	}
	if ( object->next ) {
		object->next->prev = object->prev;
	}
	object->prev = nullptr;
	object->next = nullptr;

	obj_locks[ class_ ].unlock();
}

void CGlobalData::remove_tank( CTank* tank ) {
	if ( nullptr == tank ) {
		return;
	}

	for ( auto& i : order ) {
		if ( tank == i ) {
			i = nullptr;
		}
	}
}

void CGlobalData::replace_canvas() {

	for ( int32_t i = 0; i < last_updates_count; ++i ) {
		if ( ( last_updates[ i ].y + last_updates[ i ].h ) > MENUHEIGHT ) {
			blit( env.sky,
			      canvas,
			      last_updates[ i ].x,
			      last_updates[ i ].y - MENUHEIGHT,
			      last_updates[ i ].x,
			      last_updates[ i ].y,
			      last_updates[ i ].w,
			      last_updates[ i ].h );
			masked_blit(
				terrain,
				canvas,
				last_updates[ i ].x,
				last_updates[ i ].y,
				last_updates[ i ].x,
				last_updates[ i ].y,
				last_updates[ i ].w,
				last_updates[ i ].h
			);
		} // End of having an update below the top bar
	}

	int32_t l = 0;
	int32_t r = env.screen_width - 1;
	int32_t t = MENUHEIGHT;
	int32_t b = env.screen_height - 1;

	vline( canvas, l, t, b, env.wall_colour );     // Left edge
	vline( canvas, l + 1, t, b, env.wall_colour ); // Left edge
	vline( canvas, r, t, b, env.wall_colour );     // right edge
	vline( canvas, r - 1, t, b, env.wall_colour ); // right edge
	hline( canvas, l, b, r, env.wall_colour );     // bottom edge
	if ( env.is_boxed ) {
		hline( canvas, l, t, r, env.wall_colour ); // top edge
	}

	last_updates_count = 0;
}

// Set a new command, lock guarded
void CGlobalData::set_command( int32_t cmd ) {
	cmd_lock.lock();
	command = cmd;
	cmd_lock.unlock();
}

void CGlobalData::set_curr_tank( CTank* tank_ ) {
	if ( tank_ != curr_tank ) {
		if ( curr_tank ) {
			curr_tank->deactivate();
		}
		curr_tank = tank_;
		if ( curr_tank ) {
			curr_tank->activate();
		}
	}
}

/** @brief go through the land and slide what is to be slid and is not locked
 * Slide land basic control is done using the 'done[]' array.
 * done[x] == 0 : Nothing to do. All values assumed to be correct.
 * done[x] == 1 : This column is currently in sliding.
 * done[x] == 2 : This column is about to be slid, but the base values aren't set.
 * done[x] == 3 : This column is about to be slid but locked. (Explosion not done)
 **/
void CGlobalData::slide_land() {
	// Opt out soon if no landslide is to be done
	if ( ( SLIDE_NONE == env.landslide_type ) || ( SLIDE_TANK_ONLY == env.landslide_type )
	     || ( ( SLIDE_CARTOON == env.landslide_type ) && ( env.time_to_fall > 0 ) ) ) {
		return;
	}

	for ( int32_t col = 1; col < ( env.screen_width - 1 ); ++col ) {

		// Skip this column if it is done or locked
		if ( !done[ col ] || ( 3 == done[ col ] ) ) {
			continue;
		}

		// Set base settings if this hasn't happen, yet
		if ( 2 == done[ col ] ) {
			surface[ col ].store( 0, ATOMIC_WRITE );
			drop_to[ col ] = env.screen_height - 1;
			done[ col ]   = 1;

			// Calc the top and bottom of the column to slide

			// Find top-most non-PINK pixel
			int32_t row = MENUHEIGHT + ( env.is_boxed ? 1 : 0 );

			for ( ; ( row < drop_to[ col ] ) && ( PINK == getpixel( terrain, col, row ) ); ++row )
				;
			surface[ col ].store( row, ATOMIC_WRITE ); // This is the top pixel with all gaps

			// Find bottom-most PINK pixel
			int32_t top_row = row;
			for ( row = drop_to[ col ]; ( row > top_row ) && ( PINK != getpixel( terrain, col, row ) ); --row )
				;
			drop_to[ col ] = row;

			// Find bottom-most unsupported pixel
			for ( ; ( row >= top_row ) && ( PINK == getpixel( terrain, col, row ) ); --row )
				;

			// Check whether there is anything to do or not
			if ( ( row >= top_row ) && ( top_row < drop_to[ col ] ) ) {
				fp[ col ]       = row - top_row + 1;
				velocity[ col ] = 0; // Not yet
				done[ col ]     = 1; // Can be processed
			}

			// Otherwise this column is done
			else {
				if ( !skipping_computer_play && ( velocity[ col ] > .5 ) && ( fp[ col ] > 1 ) ) {
					play_natural_sound( DIRT_FRAGMENT, col, 64, 1000 - ( fp[ col ] * 800 / env.screen_height ) );
				}
				done[ col ] = 0; // Nothing to do
				fp[ col ]   = 0;
			}
		} // End of preparations

		// Do the slide if possible
		if ( 1 == done[ col ] ) {

			// Only slide if no neighbours are locked
			bool can_slide = true;
			for ( int32_t j = col - 1; can_slide && ( j > 0 ); --j ) {
				if ( 3 == done[ j ] ) {
					can_slide = false;
				} else if ( !done[ j ] ) {
					j = 0; // no further look needed.
				}
			}
			for ( int32_t j = col + 1; can_slide && ( j < ( env.screen_width - 1 ) ); ++j ) {
				if ( 3 == done[ j ] ) {
					can_slide = false;
				} else if ( !done[ j ] ) {
					j = env.screen_width; // no further look needed.
				}
			}

			if ( can_slide ) {
				// Do instant first, because only GRAVITY remains
				// which is the case if cartoon wait time is over.
				if ( ( SLIDE_INSTANT == env.landslide_type ) || skipping_computer_play ) {
					int32_t surf = surface[ col ].load( ATOMIC_READ );
					make_bgupdate( col, surf, 1, drop_to[ col ] - surf + 1 );
					make_update( col, surf, 1, drop_to[ col ] - surf + 1 );
					blit( terrain, terrain, col, surf, col, drop_to[ col ] - fp[ col ] + 1, 1, fp[ col ] );
					vline( terrain, col, surf, drop_to[ col ] - fp[ col ], PINK );
					velocity[ col ] = fp[ col ]; // Or no sound would be played if done
					done[ col ]     = 2;         // Recheck
				} else {
					velocity[ col ] += env.gravity;
					drop_incr[ col ] += velocity[ col ];

					auto    dropAdd  = ROUND( drop_incr[ col ] );
					int32_t max_top  = MENUHEIGHT + ( env.is_boxed ? 1 : 0 );

					if ( dropAdd > 0 ) {

						int32_t top_row = surface[ col ].load( ATOMIC_READ );

						assert( ( top_row >= 0 ) && ( top_row < terrain->h )
						        && "ERROR: top_row out of range!" );

						// If the top pixel is not PINK, and the source is not
						// too high, increase dropAdd:
						int32_t over_top = top_row - dropAdd;
						while ( ( over_top <= max_top ) && ( over_top > 0 )
						        && ( PINK != getpixel( terrain, col, over_top ) ) ) {
							++dropAdd;
							--over_top;
						}

						if ( dropAdd > ( drop_to[ col ] - ( top_row + fp[ col ] ) ) ) {
							dropAdd = static_cast< int32_t >(
								drop_to[ col ] - ( top_row + fp[ col ] ) + 1
							);
							drop_incr[ col ] = dropAdd;
							done[ col ]     = 2; // Recheck
							over_top        = top_row - dropAdd;
						}

						int32_t slide_height = fp[ col ] + dropAdd;

						assert( ( over_top >= 0 ) && ( over_top < terrain->h )
						        && "ERROR: top_row - dropAdd out of range!" );
						assert( ( slide_height > 0 ) && ( slide_height <= terrain->h )
						        && "ERROR: slide_height out of range!" );
						assert( ( ( over_top + slide_height ) <= terrain->h )
						        && "ERROR: over_top + slide_height is out of range!" );
						assert( ( ( top_row + slide_height ) <= terrain->h )
						        && "ERROR: over_top + slide_height is out of range!" );

						blit( terrain, terrain, col, over_top, col, top_row, 1, slide_height );
						make_bgupdate( col, over_top, 1, slide_height + dropAdd + 1 );
						make_update( col, over_top, 1, slide_height + dropAdd + 1 );
						// If the top row reaches to the ceiling, there might
						// not be a PINK pixel to blit. In that case, one has
						// to be painted "by hand", or the slide will produce
						// nice long columns. (Happens with dirt balls when
						// "fixed" under the menubar.
						if ( over_top <= max_top ) {
							putpixel( terrain, col, max_top, PINK );
							putpixel( terrain, col, max_top + 1, PINK );
						}

						surface[ col ].fetch_add( dropAdd );
						drop_incr[ col ] -= dropAdd;
					}
				}
			}
		} // End of actual slide
	}         // End of looping columns
}

void CGlobalData::unlock_class( EClass class_ ) {
	obj_locks[ class_ ].unlock();
}

void CGlobalData::unlock_land() {
	land_lock.unlock();
}

/// @brief goes through the columns from @a left to @a right and unlocks what is locked.
void CGlobalData::unlock_land_slide( int32_t left, int32_t right ) {
	// Opt out soon if no landslide is to be done
	if ( ( SLIDE_NONE == env.landslide_type ) || ( SLIDE_TANK_ONLY == env.landslide_type ) ) {
		return;
	}

	int32_t minX = std::min( left, right );
	int32_t maxX = std::max( left, right );

	if ( minX < 1 ) {
		minX = 1;
	}
	if ( maxX > ( env.screen_width - 1 ) ) {
		maxX = env.screen_width - 1;
	}

	for ( int32_t col = minX; col <= maxX; ++col ) {
		if ( ( done[ col ] > 2 ) || !done[ col ] ) {
			done[ col ] = 2;
		}
	}
}
