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

#include "virtobj.h"

#include "environment.h"

#include <cassert>

void CVirtualObject::add_update_area( int32_t left_, int32_t top_, int32_t width_, int32_t height_ ) {
	// compute right and bottom coordinates for new and old areas
	int32_t newRight  = left_ + width_;
	int32_t newBottom = top_ + height_;
	int32_t oldRight  = dim_cur.x + dim_cur.w;
	int32_t oldBottom = dim_cur.y + dim_cur.h;

	// updating the left or top coordinate if necessary
	dim_cur.x = std::min( left_, dim_cur.x );
	dim_cur.y = std::min( top_, dim_cur.y );

	// updating the width or height if necessary
	dim_cur.w = std::max( newRight, oldRight ) - dim_cur.x + 1;
	dim_cur.h = std::max( newBottom, oldBottom ) - dim_cur.y + 1;
}

void CVirtualObject::applyPhysics() {
	x += xv;
	y += yv;
}

void CVirtualObject::draw() {
	assert( bitmap && "ERROR: CVirtualObject::draw() called without bitmap!" );

	if ( !destroy && bitmap ) {

		rotate_sprite( global.canvas, bitmap, x - ( width / 2. ), y - ( height / 2. ), itofix( angle ) );

		// The update area depends on the rotation state (aka the angle)
		if ( angle ) {
			int32_t length = std::max( width, height ) + ( std::min( width, height ) / 2 );
			set_update_area( x - ( length / 2. ), y - ( length / 2. ), length, length );
		} else {
			set_update_area( x - ( width / 2. ) - 1, y - ( height / 2. ) - 1., width + 2, height + 2 );
		}
		require_update();
	}
}

void CVirtualObject::initialise() {
	age     = 0;
	max_age  = -1;
	x       = 0;
	y       = 0;
	xv      = 0;
	yv      = 0;
	destroy = false;
	dim_cur = dim_old = sBox();
}

/// @brief Set a new bitmap and store width and height for easy drawing.
void CVirtualObject::set_bitmap( BITMAP* bitmap_ ) {
	if ( bitmap_ != bitmap ) {
		bitmap = bitmap_;

		if ( bitmap ) {
			height = bitmap->h;
			width  = bitmap->w;
		} else {
			height = 0;
			width  = 0;
		}
	}
}

void CVirtualObject::set_update_area( int32_t left_, int32_t top_, int32_t width_, int32_t height_ ) {
	dim_cur.x = left_;
	dim_cur.y = top_;
	dim_cur.w = width_;
	dim_cur.h = height_;
}

/** @brief update
 *
 * This method triggers an update of the canvas (aka drawing area) with the
 * dimensions and position of this object.
 */
void CVirtualObject::update() {
	if ( !needsUpdate.load( ATOMIC_READ ) ) {
		return;
	}

	// Add update area for the current dimension
	if ( dim_cur.w > 0 ) {
		int32_t left =
			LEFT == align    ? dim_cur.x
			: RIGHT == align ? dim_cur.x - dim_cur.w
					 : dim_cur.x - ( dim_cur.w / 2 );
		int32_t top    = LEFT == align  ? dim_cur.y
		               : RIGHT == align ? dim_cur.y - dim_cur.h
		                                : dim_cur.y - ( dim_cur.h / 2 );
		int32_t right  = std::min( env.screen_width, left + dim_cur.w + 2 );
		int32_t bottom = std::min( env.screen_height, top + dim_cur.h + 2 );

		if ( ( right > left ) && ( bottom > top ) ) {
			global.make_update( left, top, right - left, bottom - top );
		}
	} // End of updating current area

	// If the dimensions changed, the old area needs an update, too
	if ( ( dim_old.w > 0 ) && ( dim_old != dim_cur ) ) {
		int32_t left =
			LEFT == align    ? dim_old.x
			: RIGHT == align ? dim_old.x - dim_old.w
					 : dim_old.x - ( dim_old.w / 2 );
		int32_t top    = LEFT == align  ? dim_old.y
		               : RIGHT == align ? dim_old.y - dim_old.h
		                                : dim_old.y - ( dim_old.h / 2 );
		int32_t right  = std::min( env.screen_width, left + dim_old.w + 2 );
		int32_t bottom = std::min( env.screen_height, top + dim_old.h + 2 );

		if ( ( right > left ) && ( bottom > top ) ) {
			global.make_update( left, top, right - left, bottom - top );
		}
	} // End of updating old area

	dim_old = dim_cur;

	needsUpdate.store( false, ATOMIC_WRITE );
}
