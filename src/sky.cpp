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

/**
 * Code for generating sky backgrounds, including moons.
 *
 * @todo Add clouds?
 **/

#include "sky.h"

#include "externs.h"
#include "gameloop.h"
#include "gfxData.h"
#include "levelcreator.h"
#include "main.h"
#include "moon.h"
#include "random.h"

#include <cassert>


static BITMAP* temp_sky = nullptr; //!< Static temp sky bitmap for faster sky creation

/** @brief generate_sky
 *
 * Given some input parameters, renders a sky (with moons) onto a bitmap.
 *
 * @param[in] lcr The LevelCreator instance to ask whether to continue or to break off
 * @param[in] grad The sGradient to use to draw the sky
 * @param[in] flags Bitmask with GENSKY_DETAILED set to draw a detailed sky and/or GENSKY_DITHERGRAD to dither colors.
 **/
void generate_sky( LevelCreator* lcr, sGradient const* grad, int32_t flags ) {
	assert( lcr && "lcr must not be nullptr here!" );
	if ( nullptr == lcr ) {
		// No LevelCreator, no sky.
		return;
	}

	double    messiness = ( static_cast< double >( get_rand() % 100 ) / 1000.0 + 0.05 );
	int const xoffset   = get_rand() % env.screenWidth;  // For perlin, random starting x
	int const yoffset   = get_rand() % env.screenHeight; // For perlin, random starting y

	temp_sky            = create_bitmap( env.sky->w, env.sky->h );
	clear_to_color( temp_sky, BLACK );
	clear_to_color( env.sky, BLACK );

	for ( int32_t x = 0; lcr->can_work() && ( x < env.screenWidth ); ++x ) {
		for ( int32_t y = 0; lcr->can_work() && ( y < ( env.screenHeight - MENUHEIGHT ) ); ++y ) {

			lcr->yield();

			double offset = 0;

			if ( flags & GENSKY_DETAILED ) {
				offset += perlin2DPoint( 1., 200, xoffset + x, yoffset + y, .3, 6 )
				        * ( static_cast< double >( env.screenHeight - MENUHEIGHT ) * messiness );
			}

			if ( flags & GENSKY_DITHERGRAD ) {
				offset += ( get_rand() % 10 ) - 5;
			}

			while ( ( ( y + offset ) < 0 ) || ( ( y + offset + 1 ) > ( env.screenHeight - MENUHEIGHT ) ) ) {
				offset /= 2;
			}

			global.lockLand();
			solid_mode();
			putpixel( temp_sky, x, y, gradientColorPoint( grad, env.screenHeight - MENUHEIGHT, y + offset ) );
			drawing_mode( global.current_drawing_mode, nullptr, 0, 0 );
			global.unlockLand();
		}
	}
	draw_moons( lcr, temp_sky, env.screenWidth, env.screenHeight - MENUHEIGHT );

	// Put temp sky onto the real bitmap:
	if ( lcr->can_work() ) {
		global.lockLand();
		solid_mode();
		blit( temp_sky, env.sky, 0, 0, 0, 0, env.sky->w, env.sky->h );
		global.unlockLand();
	}

	// clean up
	if ( temp_sky ) {
		destroy_bitmap( temp_sky );
	}
}
