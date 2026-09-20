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

#include "beam.h"

#include "decor.h"
#include "environment.h"
#include "explosion.h"
#include "physobj.h"
#include "player.h"
#include "random.h"
#include "sound.h"
#include "tank.h"

#include <cassert>


// Static helper for the drawing methods
static int32_t beamRadius = 1;
static int32_t beamSeed   = 0;

// Helper methods for the drawing methods
static void lazerPoint( BITMAP *dest, int32_t x1, int32_t y1, int32_t color );
static void lightningPoint( BITMAP *dest, int32_t x1, int32_t y1, int32_t age );

/// @brief CBeam constructor
CBeam::CBeam( CPlayer *player_, double x_, double y_, int32_t fire_angle, int32_t weapon_type, EBeamType beam_type )
	: CPhysicalObject( BT_WEAPON == beam_type )
	, beam_type( beam_type )
	, tgt_right_x( env.screen_width ) {
	this->player   = player_;
	this->weap_type = weapon_type;

	assert( ( ( ( weap_type >= SML_LIGHTNING ) && ( weap_type <= LRG_LIGHTNING ) )
	          || ( ( weap_type >= SML_LAZER ) && ( weap_type <= LRG_LAZER ) ) )
	        && "ERROR: CBeam ctor called with something else than Lightning or Laser!" );

#ifdef NETWORK
	char buffer[ 256 ];
	sprintf( buffer, "CBeam %d %d %d %d", (int)x_, (int)y_, fire_angle, weapon_type );
	env.send_to_clients( buffer );
#endif // NETWORK

	x     = x_;
	y     = y_;
	angle = fire_angle % 360;
	xv    = env.slope[ angle ][ 0 ];
	yv    = env.slope[ angle ][ 1 ];


	if ( weap_type < WEAPONS ) {
		weap = &( weapon[ weap_type ] );
	} else {
		weap = &( naturals[ weap_type - WEAPONS ] );
	}
	radius = weap->radius;

	/* All beams should have the same age, no matter what the FPS settings
	 * are.
	 * Based on the default of 60 frames per second, the following frame
	 * lengths are wanted:
	 * Small lightning :  5 frames ( 1/12 second)
	 * Medium lightning: 10 frames ( 1/ 6 second)
	 * Large lightning : 15 frames ( 3/12 second)
	 * Lightning increase : 5 frames per size = 1/12 FPS
	 * Small laser     : 10 frames ( 1/6 second)
	 * Medium lightning: 20 frames ( 1/3 second)
	 * Large lightning : 30 frames ( 1/2 second)
	 * Laser increase     : 10 frames per size = 1/6 FPS
	 */
	int32_t age_per_size = env.frames_per_second / 12; // Doubled for laser
	int32_t base_age     = 5;                          // Doubled for laser
	int32_t weap_size    = 0;                          // aka "small"

	if ( ( weap_type >= SML_LIGHTNING ) && ( weap_type <= LRG_LIGHTNING ) ) {
		num_points = 4 + ( get_rand() % 9 ); // 4 - 12
		weap_size = weap_type - SML_LIGHTNING;
	} else if ( ( weap_type >= SML_LAZER ) && ( weap_type <= LRG_LAZER ) ) {
		base_age     *= 2;
		age_per_size *= 2;
		weap_size     = weap_type - SML_LAZER;
		if ( BT_SDI != beam_type ) {
			// The SDI constructor produces its own color
			color = makecol( 255 - ( ( weap_type - SML_LAZER ) * 64 ), 128, 64 + ( ( weap_type - SML_LAZER ) * 64 ) );
		}
		if ( !global.skipping_computer_play && ( ( BT_WEAPON == beam_type ) || ( BT_SDI == beam_type ) ) ) {
			play_fire_sound( weap_type, ROUND( x ), 128 + ( radius * 10 ), 1500 - ( radius * 50 ) );
		}
	}

	max_age = base_age + ( age_per_size * weap_size );
	damage = static_cast< double >( weap->damage ) / static_cast< double >( max_age );

	// Set an offset seed
	seed = get_rand() % std::max( env.screen_width, env.screen_height );

	create_beam_path();

	// Now that the points are clear, a lightning bolt can emit its thunder:
	if ( !global.skipping_computer_play && ( BT_NATURAL == beam_type ) ) {
		play_natural_sound( weap_type, ( points[ 0 ].x + points[ num_points - 1 ].x ) / 2, 175 + ( radius * 10 ), 1000 );
	}

	// Add to the chain unless it is a mind shot:
	if ( BT_MIND_SHOT != beam_type ) {
		global.add_object( this );
	}
}

/// @brief special constructor for SDI lasers
CBeam::CBeam( CPlayer *player_, double x_, double y_, double tx, double ty, int32_t weapon_type, bool is_burnt_out )
	: CBeam( player_, x_, y_, GET_ANGLE( std::abs( ty - y_ ), tx - x_ ) + 90, weapon_type, BT_SDI ) {
	if ( player ) {
		++player->sdi_shots;
	}

	// SDI lasers are redder than normal, even more if burnt_out
	color = makecol(
		is_burnt_out ? 255 : 240 - ( ( weap_type - SML_LAZER ) * 16 ),
		is_burnt_out ? 32 : 64,
		is_burnt_out ? ( weap_type - SML_LAZER ) * 32 : 128
	);

	// Limit the laser to the missiles coordinates
	points[ num_points - 1 ].x = ROUND( tx );
	points[ num_points - 1 ].y = ROUND( ty );
}

/// @brief CBeam destructor
CBeam::~CBeam() {
	require_update();
	update();

	if ( BT_MIND_SHOT != beam_type ) {
		global.make_bgupdate( dim_cur.x, dim_cur.y, dim_cur.w, dim_cur.h );
		global.make_bgupdate( dim_old.x, dim_old.y, dim_old.w, dim_old.h );

		// Let the land slide where the beam burned through:
		global.add_land_slide( tgt_left_x, tgt_right_x, false );

		// Apply damage to all hit tanks:
		CTank *lt = nullptr;
		global.get_head_of_class( CLASS_TANK, &lt );
		while ( lt ) {
			lt->apply_damage();
			lt->getNext( &lt );
		}

		// Take out of the chain:
		global.remove_object( this );

		// The player is allowed to fire one more SDI laser again:
		if ( ( BT_SDI == beam_type ) && player ) {
			--player->sdi_shots;
		}
	}
}

void CBeam::applyPhysics() {
	if ( ++age > max_age ) {
		destroy = true;
	}

	if ( BT_SDI != beam_type ) {
		create_beam_path();
	}

	if ( BT_MIND_SHOT != beam_type ) {
		if ( !global.skipping_computer_play && !( get_rand() % ( env.frames_per_second / 5 ) ) ) {
			try {
				new CDecor(
					points[ num_points - 1 ].x,
					points[ num_points - 1 ].y,
					( get_rand() % 7 ) - 3,
					1 - ( get_rand() % 6 ),
					radius,
					DECOR_SMOKE,
					0
				);
			} catch ( std::exception &e ) {
				std::cerr << __func__ << " new CDecor: " << e.what() << std::endl;
			}
		}

		try {
			new CExplosion(
				player,
				points[ num_points - 1 ].x,
				points[ num_points - 1 ].y,
				points[ num_points - 1 ].x - points[ 0 ].x,
				points[ num_points - 1 ].y - points[ 0 ].y,
				weap_type,
				damage,
				is_weapon_fire
			);
		} catch ( std::exception &e ) {
			std::cerr << __func__ << " new CExplosion: " << e.what() << std::endl;
		}
	}
}

void CBeam::draw() {
	// never draw mind shots!
	if ( BT_MIND_SHOT == beam_type ) {
		return;
	}

	int32_t oldDrawingMode = global.current_drawing_mode;

	drawing_mode( DRAW_MODE_TRANS, nullptr, 0, 0 );
	global.current_drawing_mode = DRAW_MODE_TRANS;
	set_trans_blender( 0, 0, 0, 50 );

	beamRadius = radius;
	beamSeed   = seed;

	for ( int32_t i = 1; i < num_points; ++i ) {
		int32_t left   = std::min( points[ i - 1 ].x, points[ i ].x );
		int32_t top    = std::min( points[ i - 1 ].y, points[ i ].y );
		int32_t right  = std::max( points[ i - 1 ].x, points[ i ].x );
		int32_t bottom = std::max( points[ i - 1 ].y, points[ i ].y );

		if ( ( weap_type >= SML_LIGHTNING ) && ( weap_type <= LRG_LIGHTNING ) ) {
			do_line( global.canvas, points[ i - 1 ].x, points[ i - 1 ].y, points[ i ].x, points[ i ].y, age, lightningPoint
			);
		} else if ( ( weap_type >= SML_LAZER ) && ( weap_type <= LRG_LAZER ) ) {
			do_line( global.canvas, points[ i - 1 ].x, points[ i - 1 ].y, points[ i ].x, points[ i ].y, color, lazerPoint
			);
		}

		add_update_area( left - radius, top - radius, right - left + ( 2 * radius ), bottom - top + ( 2 * radius ) );
	}

	drawing_mode( oldDrawingMode, nullptr, 0, 0 );
	global.current_drawing_mode = oldDrawingMode;

	require_update();
}

// === Private method implementations ===
// ======================================

/// @brief Create the basic points array with path tracing
void CBeam::create_beam_path() {
	// First determine the direct target - where does the beam end?
	double tx = x, ty = y;
	hit_something = false;

	// If this is not the first call, use the already known endpoints
	if ( ( points[ 0 ].x || points[ 0 ].y || points[ num_points - 1 ].x || points[ num_points - 1 ].y )
	     && !global.is_dirt_in_box( points[ 0 ].x, points[ 0 ].y, points[ num_points - 1 ].x, points[ num_points - 1 ].y ) ) {
		tx = points[ num_points - 1 ].x;
		ty = points[ num_points - 1 ].y;
	} else {
		// The first point is the starting point, the last will become the target
		points[ 0 ].x = ROUND( x );
		points[ 0 ].y = ROUND( y );
	}

	while ( !hit_something && ( tx > -radius ) && ( tx < ( env.screen_width + radius ) ) && ( ty > -radius )
	        && ( ty < ( env.screen_height + radius ) ) ) {

		// Assume PINK for off screen pixels
		int32_t col = PINK;

		if ( ( tx > 0 ) && ( tx < ( env.screen_width - 1 ) ) && ( ty > MENUHEIGHT )
		     && ( ty < ( env.screen_height - 1 ) ) ) {
			col = getpixel( global.terrain, tx, ty );
		}

		if ( PINK == col ) {
			tx += xv;
			ty += yv;
		} else {
			hit_something = true;
		}
	} // End of tracing pixels

	// tx and ty now result in the first obstacle (or screen border)
	// on a direct path.
	points[ num_points - 1 ].x = ROUND( tx );
	points[ num_points - 1 ].y = ROUND( ty );

	// If this is a lightning strike, points between the first and last
	// have to be (re-)generated.
	make_lightning_path();

	// Generate new lightning offsets checking for collisions:
	trace_beam_path();

	// If this is a mind_shot, it is immediately destroyed
	if ( BT_MIND_SHOT == beam_type ) {
		destroy = true;
	}
}

/// @brief get the end of a mind shot laser
void CBeam::get_end_point( int32_t &x, int32_t &y ) {
	x = points[ num_points - 1 ].x;
	y = points[ num_points - 1 ].y;
}

/// @brief create the lightning steps between the beginning and the end
void CBeam::make_lightning_path() {
	if ( ( num_points > 2 ) && ( weap_type >= SML_LIGHTNING ) && ( weap_type <= LRG_LIGHTNING ) ) {
		int32_t maxP     = num_points - 1;
		double  stepping = FABSDISTANCE2( points[ 0 ].x, points[ 0 ].y, points[ maxP ].x, points[ maxP ].y ) / maxP;

		for ( int32_t i = 1; i < maxP; ++i ) {
			points[ i ].x = ROUND(
				x + ( xv * ( static_cast< double >( i ) * stepping ) )
				+ ( perlin2DPoint( 1.0, 10. * radius, points[ i ].x + seed, points[ i ].y, 0.3, 6 ) * radius
			            * 10. )
			);
			points[ i ].y = ROUND(
				y + ( yv * ( static_cast< double >( i ) * stepping ) )
				+ ( perlin2DPoint( 1.0, 10. * radius, points[ i ].x, points[ i ].y + seed, 0.3, 6 ) * radius
			            * 10. )
			);
		}
	} // End of lightning preparation
}

/// @brief this method is used by the satellite to move the beam with itself.
void CBeam::move_start( double x_, double y_ ) {
	x             = x_;
	y             = y_;
	points[ 0 ].x = ROUND( x );
	points[ 0 ].y = ROUND( y );
}

/// @brief walk through the beam points and check whether anything is hit
void CBeam::trace_beam_path() {
	int32_t minRange = radius + 2;
	bool    canHit   = false;

	hit_something     = false;

	for ( int32_t i = 1; !hit_something && ( i < num_points ); ++i ) {
		double start_x   = points[ i - 1 ].x;
		double start_y   = points[ i - 1 ].y;
		double end_x     = points[ i ].x;
		double end_y     = points[ i ].y;
		bool   chkTanks = BT_SDI != beam_type && global.are_tanks_in_box( start_x, start_y, end_x, end_y );
		bool   chkDirt  = global.is_dirt_in_box( start_x, start_y, end_x, end_y );

		// Break this if there is nothing possibly in between
		if ( !( chkTanks || chkDirt ) ) {
			continue;
		}

		int32_t range  = 0;
		double  distX  = end_x - start_x;
		double  distY  = end_y - start_y;
		double  absX   = std::abs( distX );
		double  absY   = std::abs( distY );
		double  moveX  = distX / ( absX > absY ? absX : absY );
		double  moveY  = distY / ( absY > absX ? absY : absX );
		auto    toMove = ROUND( std::max( absX, absY ) );

		// Now wander along the path:
		while ( !hit_something && ( range < toMove ) && ( start_x > 0 ) && ( start_x < ( env.screen_width - 1 ) )
		        && ( start_y > MENUHEIGHT ) && ( start_y < ( env.screen_height - 1 ) )
		        && ( !chkDirt || ( PINK == getpixel( global.terrain, start_x, start_y ) ) ) ) {

			// Only check for tanks if the total range is large enough
			// and if there are tanks in the path
			if ( ( range >= minRange ) && !canHit ) {
				canHit = true;
			}
			if ( canHit && chkTanks ) {
				CTank *lt = nullptr;
				global.get_head_of_class( CLASS_TANK, &lt );
				while ( lt ) {
					// Tank found, is it hit?
					if ( !lt->destroy
					     && lt->is_in_box( start_x - radius, start_y - radius, start_x + radius, start_y + radius ) ) {
						hit_something = true;
						lt->require_update();

						// 'Lock' the beam end on the tank:
						if ( start_y < ( lt->y + radius ) ) {
							start_y = lt->y + radius;
						}
						if ( start_y > ( lt->y + ( 2 * radius ) ) ) {
							start_y = lt->y + ( 2 * radius );
						}
						if ( start_x < ( lt->x - ( radius / 2. ) ) ) {
							start_x = lt->x - ( radius / 2. );
						}
						if ( start_x > ( lt->x + ( radius / 2. ) ) ) {
							start_x = lt->x + ( radius / 2. );
						}

						// Get the in_rates
						double in_rate_x, in_rate_y;
						if ( ( BT_MIND_SHOT != beam_type )
						     && lt->is_in_ellipse( start_x, start_y, radius, radius, in_rate_x, in_rate_y ) ) {
							double in_rate = in_rate_x * in_rate_y;
							if ( in_rate < 0.9 ) {
								// Beams do not 'splash'.
								in_rate = 0.9;
							}

							lt->add_damage(
								player,
								static_cast< double >( damage ) * in_rate
									* ( player ? player->damage_multiplier : 1. )
							);
						}
						// That's it
						lt    = nullptr;
						moveX = 0.;
						moveY = 0.;
					} // End of having a tank
					else {
						lt->getNext( &lt );
					}
				} // End of looping tanks
			}

			// Advance start_x/Y
			start_x += moveX;
			start_y += moveY;
			++range;
		} // End of regular check

		// If dirt was hit, hit_something must be adapted
		if ( ( PINK != getpixel( global.terrain, start_x, start_y ) ) ) {
			hit_something = true;
		}

		if ( hit_something
		     && ( ( ROUND( start_x ) != points[ num_points - 1 ].x ) || ( ROUND( start_y ) != points[ num_points - 1 ].y )
		     ) ) {
			// Reset the points to the new circumstances:
			points[ num_points - 1 ].x = ROUND( start_x );
			points[ num_points - 1 ].y = ROUND( start_y );
			make_lightning_path();

			// Note down x position for dirt slide on destruction:
			if ( ( start_x - radius - 1 ) < tgt_left_x ) {
				tgt_left_x = ROUND( start_x - radius - 1. );
			}
			if ( ( start_x + radius + 1 ) > tgt_right_x ) {
				tgt_right_x = ROUND( start_x + radius + 1. );
			}
		} // End of checking pixels
	}         // End of looping points
}

// === static helper methods ===
// =============================
static void lazerPoint( BITMAP *dest, int32_t x1, int32_t y1, int32_t color ) {
	circlefill( dest, x1, y1, beamRadius, color );
}

static void lightningPoint( BITMAP *dest, int32_t x1, int32_t y1, int32_t age ) {
	double pRad = ( perlin2DPoint( 1.0, 2, x1 + age, y1 + beamSeed, 0.3, 6 ) + 1 ) / 2 * beamRadius + 1;
	double offX = ( perlin2DPoint( 1.0, 10 * pRad, x1 + age + beamSeed, y1 + age, 0.3, 6 ) + 1 ) * pRad / 2.;
	double offY = ( perlin2DPoint( 1.0, 10 * pRad, x1 + age, y1 + age + beamSeed, 0.3, 6 ) + 1 ) * pRad / 2.;

	circlefill( dest, x1 + offX, y1 + offY, pRad, WHITE );
}
