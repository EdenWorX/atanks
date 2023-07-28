//
// Created by sed on 28.07.23.
//

#include "levelcreator.h"

#include "land.h"
#include "random.h"
#include "sky.h"


static inline double colorDistance( int32_t col1, int32_t col2 );
static inline void   set_level_settings( LevelCreator* lcr );


/// Level Creator Methods implementation
LevelCreator::LevelCreator() = default;

/// The operator is just a wrapper.
void LevelCreator::operator() () {
	fiVal = 1;
	set_level_settings( this );
	fiVal = 0;
}

void LevelCreator::add_fi() {
	fiLock.lock();
	++fiVal;
	fiLock.unlock();
}

bool LevelCreator::can_work() const {
	return !i_shall_die;
}

void LevelCreator::die_now() {
	i_shall_die = true;
}

bool LevelCreator::has_progress() {
	fiLock.lock();
	bool result = ( fiVal > 0 );
	fiVal       = 0;
	fiLock.unlock();

	return result;
}

bool LevelCreator::is_finished() const {
	return in_progress[ 3 ];
}

void LevelCreator::print_state() const {
	if ( in_progress[ 0 ] ) {
		draw_sprite( global.canvas, env.misc[ 1 ], env.halfWidth - 120, env.halfHeight + 115 );
		textout_centre_ex( global.canvas, font, env.ingame->Get_Line( 42 ), env.halfWidth, env.halfHeight + 116, WHITE, -1 );
		global.make_update( env.halfWidth - 120, env.halfHeight + 115, env.misc[ 1 ]->w, env.misc[ 1 ]->h );
	}

	if ( in_progress[ 1 ] ) {
		draw_sprite( global.canvas, env.misc[ 1 ], env.halfWidth - 120, env.halfHeight + 155 );
		textout_centre_ex( global.canvas, font, env.ingame->Get_Line( 44 ), env.halfWidth, env.halfHeight + 156, WHITE, -1 );
		global.make_update( env.halfWidth - 120, env.halfHeight + 155, env.misc[ 1 ]->w, env.misc[ 1 ]->h );
	}


	if ( in_progress[ 2 ] ) {
		draw_sprite( global.canvas, env.misc[ 1 ], env.halfWidth - 120, env.halfHeight + 195 );
		textout_centre_ex( global.canvas, font, env.ingame->Get_Line( 43 ), env.halfWidth, env.halfHeight + 196, WHITE, -1 );
		global.make_update( env.halfWidth - 120, env.halfHeight + 195, env.misc[ 1 ]->w, env.misc[ 1 ]->h );
	}
}

/// @brief Tell the LevelCreator that it does not need to yield any more
void LevelCreator::work_alone() {
	i_must_yield = false;
}

void LevelCreator::working_on( int32_t what ) {
	if ( ( what > 0 ) && ( what < 5 ) ) {
		add_fi();
		in_progress[ what - 1 ] = true;
	}
}

/// @brief yield if it is not working alone
void LevelCreator::yield() {
	if ( i_must_yield ) {
		std::this_thread::yield();
	}
}

/*****************************************************************************
 * colorDistance
 *
 * Treat two color values as 3D vectors of the form <r,g,b>.
 * Compute the scalar size of the difference between the two vectors.
 * *****************************************************************************/
double colorDistance( int32_t col1, int32_t col2 ) {
	int32_t r1 = getr( col1 );
	int32_t g1 = getg( col1 );
	int32_t b1 = getb( col1 );
	int32_t r2 = getr( col2 );
	int32_t g2 = getg( col2 );
	int32_t b2 = getb( col2 );

	// Treat the colour-cube as a space
	return FABSDISTANCE3( r1, g1, b1, r2, g2, b2 );
}

/** @brief Set up the next level to play
 *
 * This must work in parallel with the shop(), so any drawing must
 * lock the land, do the drawing and unlock it again.
 **/
static inline void set_level_settings( LevelCreator* lcr ) {

	//  -------------------------
	// ===  Choosing colours   ===
	//=============================
	lcr->working_on( 1 );

	// Choose first gradients for sky and land

	// First the land:
	if ( lcr->can_work() ) {
		global.curland = ( get_rand() % LANDS ) + ( CT_CRISPY == env.colourTheme ? LANDS : 0 );
		if ( !env.gfxData.land_gradient_strips[ global.curland ] ) {

			global.lockLand();

			env.gfxData.land_gradient_strips[ global.curland ] =
				create_gradient_strip( land_gradients[ global.curland ], ( env.screenHeight - MENUHEIGHT ) );

			global.unlockLand();
		}
	}

	// Then the sky
	if ( lcr->can_work() ) {
		global.cursky = ( get_rand() % SKIES ) + ( CT_CRISPY == env.colourTheme ? SKIES : 0 );

		if ( !env.gfxData.sky_gradient_strips[ global.cursky ] ) {
			global.lockLand();

			env.gfxData.sky_gradient_strips[ global.cursky ] =
				create_gradient_strip( sky_gradients[ global.cursky ], ( env.screenHeight - MENUHEIGHT ) );

			global.unlockLand();
		}
	}
	BITMAP* sky_gradient_strip = env.gfxData.sky_gradient_strips[ global.cursky ];

	//  -------------------------
	// === Rendering Landscape ===
	//=============================
	lcr->working_on( 2 );
	if ( lcr->can_work() ) {
		generate_land( lcr, get_rand() % env.screenWidth, env.screenHeight );
	}


	//  -------------------------
	// ===    Check Colours    ===
	//=============================
	if ( lcr->can_work() ) {
		int32_t peak_height = env.screenHeight;
		for ( int32_t z = 0; lcr->can_work() && ( z < env.screenWidth ); ++z ) {
			auto local_height = ROUND( env.screenHeight - global.surface[ z ].load() );
			if ( peak_height > local_height ) {
				peak_height = local_height;
			}
		}

		int32_t min_dist    = 128; // start with this colour distance wanted
		int32_t max_tries   = 16;  // These many tries before lowering the distance
		int32_t cur_try     = 1;
		int32_t bottom      = env.screenHeight - MENUHEIGHT;
		int32_t max_y       = std::min( env.screenHeight - peak_height, bottom );
		bool    has_colours = false;

		while ( !has_colours && lcr->can_work() ) {
			has_colours = true;

			for ( int32_t y = 2; lcr->can_work() && has_colours && ( y < max_y ); ++y ) {
				if ( colorDistance(
					     getpixel( sky_gradient_strip, 0, bottom - y ),
					     getpixel( global.terrain, 0, env.screenHeight - y )
				     )
				     < min_dist ) {
					has_colours = false;
				}
			}

			if ( !has_colours && lcr->can_work() ) {
				// Create new strip:
				global.cursky = ( get_rand() % SKIES ) + ( CT_CRISPY == env.colourTheme ? SKIES : 0 );

				if ( !env.gfxData.sky_gradient_strips[ global.cursky ] ) {
					global.lockLand();

					env.gfxData.sky_gradient_strips[ global.cursky ] = create_gradient_strip(
						sky_gradients[ global.cursky ],
						( env.screenHeight - MENUHEIGHT )
					);

					global.unlockLand();
				}
				sky_gradient_strip = env.gfxData.sky_gradient_strips[ global.cursky ];

				// Advance try and check:
				if ( ++cur_try > max_tries ) {
					cur_try   = 1;
					min_dist /= 2;

					// Break if min_dist is reduced to 1:
					if ( min_dist < 2 ) {
						has_colours = true;
					}
				} // end of advancing tries
			}         // end of handling wrong colours
		}                 // End of searching suitable sky colours
	}                         // end of thread not to be killed

	//  -------------------------
	// ===    Rendering Sky    ===
	//=============================
	lcr->working_on( 3 );

	if ( lcr->can_work() ) {
		if ( env.sky && ( ( env.sky->w != env.screenWidth ) || ( env.sky->h != ( env.screenHeight - MENUHEIGHT ) ) ) ) {
			destroy_bitmap( env.sky );
			env.sky = nullptr;
		}

		// see if we want a custom background
		if ( env.custom_background && env.bitmap_filenames ) {
			global.lockLand();
			if ( env.sky ) {
				destroy_bitmap( env.sky );
			}
			env.sky = load_bitmap( env.bitmap_filenames[ get_rand() % env.number_of_bitmaps ], nullptr );
			global.unlockLand();
		}

		// if we do not have a custom background (or do not want one) create a new background
		if ( !env.custom_background || !env.sky ) {
			global.lockLand();

			if ( !env.sky ) {
				env.sky = create_bitmap( env.screenWidth, env.screenHeight - MENUHEIGHT );
			}

			global.unlockLand();

			generate_sky(
				lcr,
				sky_gradients[ global.cursky ],
				( env.ditherGradients ? GENSKY_DITHERGRAD : 0 ) | ( env.detailedSky ? GENSKY_DETAILED : 0 )
			);
		}
	}

	lcr->working_on( 4 );
}
