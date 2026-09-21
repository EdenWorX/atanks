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


#include "missile.h"

#include "aicore.h"
#include "beam.h"
#include "decor.h"
#include "explosion.h"
#include "player.h"
#include "random.h"
#include "sound.h"
#include "tank.h"
#include "weapon.h"

#include <cassert>

/* Note: If you wonder why the CMissile ctor needs the AI_LEVEL, it is used
 *       for two things:
 *       1. Whether repulsion is considered for mind shots depends on the
 *          ai_level of the bot tracking the missile, and
 *       2. the SDI check must make sure to not re-test its own mind shots.
 */
CMissile::CMissile(
	CPlayer*      player_,
	double       xpos,
	double       ypos,
	double       xvel,
	double       yvel,
	int32_t      weapon_type,
	EMissileType missile_type,
	int32_t      ai_level_,
	int32_t      delay_idx_
)
	: CPhysicalObject( MT_WEAPON == missile_type )
	, ai_level( ai_level_ )
	, missile_type( missile_type ) {
	this->player = player_;

#ifdef NETWORK
	char buffer[ 256 ];
	sprintf( buffer, "CMissile %d %d %lf %lf %d", ROUND( xpos ), ROUND( ypos ), xvel, yvel, weapon_type );
	env.send_to_clients( buffer );
#endif

	// Set position and movement
	x  = xpos;
	y  = ypos;
	xv = xvel;
	yv = yvel;

	// Get and set weapon/item data
	weap_type = weapon_type;
	if ( weap_type < WEAPONS ) {
		weap = &weapon[ weap_type ];
	} else {
		weap = &naturals[ weap_type - WEAPONS ];
	}
	assert( env.missile[ weap->picpoint ] && "Missile has no Bitmap loaded!" );
	set_bitmap( env.missile[ weap->picpoint ] );
	drag     = weap->drag;
	mass     = weap->mass;
	noimpact = weap->noimpact;

	// The max_vel value results in a small missile being able to be accelerated
	// by 25% over MAX_POWER, while a large Napalm Bomb can go up to 220%.
	max_vel = env.max_velocity * ( 1.20 + ( mass / ( .01 * MAX_POWER ) ) );
	DEBUG_LOG_PHY( "CPhysicalObject", "env.max_vel: %5.2lf, mass: %5.2lf, obj.max_vel: %5.2lf", env.max_velocity, mass, max_vel )

	// Meteors and dirt balls are "volatile" and can not be accelerated
	// over MAX_POWER. (Pre-caution against "forever" going naturals)
	if ( ( ( SML_METEOR <= weap_type ) && ( LRG_METEOR >= weap_type ) )
	     || ( ( DIRT_BALL <= weap_type ) && ( SUP_DIRT_BALL >= weap_type ) ) ) {
		max_vel = std::min( max_vel, static_cast< double >( MAX_POWER ) );
	}

	if ( ( SML_METEOR <= weap_type ) && ( LRG_METEOR >= weap_type ) ) {
		angle  = get_rand() % 360;
		spin   = ( get_rand() % 20 ) - 10;
		max_age = MAX_METEOR_AGE;
	} else if ( weap_type == NAPALM_JELLY ) {
		// Napalm grows, others do not:
		is_growing      = true;
		grow_radius     = 1;
		max_age         = MAX_JELLY_AGE;
		allow_dirty_wrap = false;
	} else {
		grow_radius = weap->radius;
		if ( FUNKY_BOMBLET == weap_type ) {
			max_age = ( MAX_MISSILE_AGE / 7 ) + ( get_rand() % ( MAX_MISSILE_AGE / 4 ) );
		}
		// With MMA 15 seconds, this is 2 + [0;3] = [2;5] seconds
		else if ( FUNKY_DEATHLET == weap_type ) {
			max_age = ( MAX_MISSILE_AGE / 5 ) + ( get_rand() % ( MAX_MISSILE_AGE / 3 ) );
		}
		// With MMA 15 seconds, this is 3 + [0;4] = [3;7] seconds
		else {
			max_age = MAX_MISSILE_AGE;
		}
	}

	// Finalize max_age to be for frames, not seconds:
	max_age *= env.frames_per_second;

	// Set funky colour of the funky bomblets/deathlets and add some max_age
	// variation so they do not detonate in groups.
	if ( ( weap_type == FUNKY_BOMBLET ) || ( weap_type == FUNKY_DEATHLET ) ) {
		int32_t temp_number = get_rand() % 5;
		switch ( temp_number ) {
			case 0:
				funky_colour = makecol( 200, 0, 0 );
				break;
			case 1:
				funky_colour = makecol( 0, 200, 0 );
				break;
			case 2:
				funky_colour = makecol( 0, 0, 200 );
				break;
			case 3:
				funky_colour = makecol( 200, 200, 0 );
				break;
			case 4:
				funky_colour = makecol( 200, 0, 200 );
				break;
			default:
				break;
		}

		// Variation +/- 1 Second in frames:
		max_age += ( get_rand() % ( 2 * env.frames_per_second ) ) - env.frames_per_second;
	}

	// Some weapons must not wrap through dirt ceilings if the bottom
	// pixel they would warp into is occupied:
	if ( ( ( SML_ROLLER <= weap_type ) && ( SMALL_MIRV >= weap_type ) )
	     || ( ( CLUSTER <= weap_type ) && ( SUP_CLUSTER >= weap_type ) )
	     || ( ( SML_NAPALM <= weap_type ) && ( LRG_NAPALM >= weap_type ) ) || ( CLUSTER_MIRV == weap_type ) ) {
		allow_dirty_wrap = false;
	}

	// If this is a mind shot with a delay characteristic, like
	// the chain missile, set a delay range by its index.
	// This causes CPhysicalObject::applyPhysics() to not detonate
	// the missile unless the set distance was travelled through
	// dirt. This simulates the clearing of the path by previous
	// missiles, so the AI can track those weapons better.
	if ( MT_MIND_SHOT == missile_type ) {
		this->mind_delay = weap->radius * delay_idx_;
	}

	// Otherwise add it to the chain:
	// Do not put mind shots in there or the object updaters
	// will not only try to apply physics, but use delete on
	// them when they get destroyed.
	else {
		global.add_object( this );
	}
}

CMissile::~CMissile() {
	// Take out of the chain:
	if ( MT_MIND_SHOT != missile_type ) {
		global.remove_object( this );
	}
}

void CMissile::applyPhysics() {
	// Increase age and get rid of the missile if it
	// is caught in some endless loop.
	if ( ( ++age > max_age ) || ( y < -65535 ) ) {
		hit_something = true;
		if ( MT_MIND_SHOT != missile_type ) {
			trigger();
		} else {
			destroy = true;
		}
		return;
	} else {
		hit_something = false;
	}

	// Riot charges / blasts go off immediately:
	if ( ( RIOT_CHARGE <= weap_type ) && ( RIOT_BLAST >= weap_type ) ) {
		trigger();
		return;
	}

	// Napalm grows first:
	if ( is_growing ) {
		if ( age < max_age ) {
			grow_radius = ROUND(
				static_cast< double >( weap->radius )
				* ( static_cast< double >( age ) / static_cast< double >( max_age ) )
			);
			if ( grow_radius < 2 ) {
				grow_radius = 2;
			}
		} else {
			is_growing = false; // Finished growing!
		}
	}

	// Normal and digging physic types need an angle
	// and can be repulsed
	if ( ( PT_NORMAL == phys_type ) || ( PT_DIGGING == phys_type ) ) {

		if ( ( SML_METEOR <= weap_type ) && ( LRG_METEOR >= weap_type ) ) {
			angle = ( angle + spin ) % 360;
		} else {
			angle = ROUND( RAD2DEG( atan( yv / xv ) ) * 256. / 360. ) - 64 + ( xv < 0 ? 128 : 0 );
		}

		if ( ( MT_MIND_SHOT != missile_type )
		     // You need 20 SDI units to have them calculate repulsing 100% of the time
		     || ( ( SDI_PREDICTOR == ai_level ) && ( ( get_rand() % 20 ) < player->ni[ ITEM_SDI ] ) )
		     // Useless and Guesser never predict repulsors and Rangefinder starts with a 50% chance.
		     || ( ( GUESSER_PLAYER < ai_level ) // [7;12] Rangefinder; [4;12] Targetter, [3;12] Deadly, [1;12] Deadly+1
		          && ( ( ( maxAiLevel - RAND_AI_0P ) + ( maxAiLevel - RAND_AI_1P ) ) < 10 ) )
		     // Note: Even the deadly has no 100%, which is on purpose.
		) {
			repulse_missile();
		}
	}

	// === 1) Handle standard physics projectiles ===
	if ( PT_NORMAL == phys_type ) {
		apply_physics_normal();
	} // --- end of normal physic types ---

	// === 2) Handle rolling projectiles ===
	else if ( PT_ROLLING == phys_type ) {
		apply_physics_rolling();
	} // --- end of rolling physic types ---

	// === 3) Handle funky projectiles ===
	else if ( PT_FUNKY_FLOAT == phys_type ) {
		apply_physics_funky();
	} // --- end of funky float physic types ---

	// === 4) Handle other projectiles ===
	else {
		apply_physics_other();
	} // end of checking 'others'

	// Final check against the terrain
	if ( !hit_something && ( y > MENUHEIGHT ) && ( y < ( env.screen_height - 1 ) ) ) {
		auto    round_x = ROUND( x );
		auto    round_y = ROUND( y );
		int32_t hitpix  = getpixel( global.terrain, round_x, round_y );
		if ( ( ( PT_DIGGING == phys_type ) && ( PINK == hitpix ) )
		     || ( ( PT_DIGGING != phys_type ) && ( PINK != hitpix ) ) ) {
			hit_something = true;
		}
	}

	// No "ceiling drops" are triggered in boxed mode
	if ( !hit_something && ( y <= MENUHEIGHT ) && env.is_boxed ) {
		yv           = 0;
		hit_something = true;
	}

	// Be sure any trigger/detonation is on screen ...
	if ( hit_something && ( y <= MENUHEIGHT ) ) {
		y = MENUHEIGHT + 1;
	}

	trigger_test();
}

/// @return The number of bounces done since the missile was fired
int32_t CMissile::bounced() const {
	return bounces;
}

/// @return -1 if the missile flies to the left, 1 if it flies to the right
/// or does not have any vertical movement
int32_t CMissile::direction() const {
	return SIGN( xv );
}

void CMissile::draw() {
	if ( destroy
	     // Do not draw mind shots
	     || ( MT_MIND_SHOT == missile_type ) ) {
		return;
	}

	// draw arrow impostor if it is above the screen
	if ( y < MENUHEIGHT ) {
		// Back up original values
		BITMAP* bbitmap = get_bitmap();
		double  by      = y;
		int32_t bangle  = angle;

		// Set arrow values
		set_bitmap( env.misc[ 3 ] );
		y     = MENUHEIGHT + ( height / 2. );
		angle = 0;
		CPhysicalObject::draw();

		// restore original values:
		set_bitmap( bbitmap );
		y     = by;
		angle = bangle;

		return;
	}

	// draw missile on the screen

	// Napalm jellies need a special drawing due to their growing nature.
	if ( weap_type == NAPALM_JELLY ) {
		if ( is_growing ) {
			draw_Napalm_Blob( this, x, y, grow_radius, age / weap->etime );
		} else {
			draw_Napalm_Blob( this, x, y, weap->radius, age / weap->etime );
		}

	} // end of napalm

	// try drawing a funky bomblet
	else if ( ( FUNKY_BOMBLET == weap_type ) || ( FUNKY_DEATHLET == weap_type ) ) {

		circlefill( global.canvas, x, y, 4, funky_colour );
		circle( global.canvas, x, y, 5, BLACK );
		set_update_area( x - 10, y - 10, 20, 20 );
		require_update();
	}

	// draw anything else
	else {

		// Digging weapons scorch the earth they travel through
		if ( PT_DIGGING == phys_type ) {
			uint32_t scorches = 3 + ( 3 * ABSDISTANCE2( x, y, x + xv, y + yv ) );

			for ( uint32_t i = 0; i < scorches; ++i ) {
				int32_t sx = ROUND( x ) + ( ( get_rand() % 5 ) - 2 ); // [-2;2]
				int32_t sy = ROUND( y ) + ( ( get_rand() % 5 ) - 2 ); // [-2;2]

				if ( ( sx > 1 ) && ( sx < env.screen_width ) && ( sy > MENUHEIGHT )
				     && ( sy < env.screen_height ) ) {
					int32_t pc = getpixel( global.terrain, sx, sy );
					if ( PINK != pc ) {
						putpixel(
							global.terrain,
							ROUND( x ),
							sy,
							makecol( getr( pc ) * .900, getg( pc ) * .825, getb( pc ) * .866 )
						);
					}
				} // end of having valid coordinates
			}         // end of looping scorches
		}                 // end of scorching

		CPhysicalObject::draw();
	}
}

/// === Private methods ===
///=========================


/// @brief little helper struct to fire SDI lasers in a fairer way
struct sSDI {
	int32_t am    = 0;
	double  dist  = 0.; // Distance used for sorting
	double  lvl   = 0.; // Level of the SDI, One per 5 SDIs bought
	double  mod   = 0.; // Level mod in the range of 1.0 to 1.5 (Caps levels at 10 which is 50xSDI this way)
	sSDI*   next  = nullptr;
	double  range = 100.; // The more SDI, the further the shot
	CTank*   tank  = nullptr;
	double  x     = 0.;
	double  y     = 0.;
};

void CMissile::apply_physics_funky() {
	// Funky Floats have a 0.75% chance to randomly change their direction
	// (rate scaled to the frame rate so it stays 0.75% per 60 FPS frame)
	if ( 0 == ( get_rand() % ROUND( 150 * env.frame_count_mod ) ) ) {

		int32_t floatee_action = get_rand() % 4;

		// Three possibilities:
		// A) 25% chance to reverse x movement
		// B) 25% chance to reverse y movement
		// C) 50% chance to pick a random target to home into.
		// If A is chosen and there is no x movement, or B is chosen and
		// there is no y movement, option C is pulled.
		if ( ( 1 == floatee_action ) && ( std::abs( xv ) > 0.5 ) ) {
			xv *= -1.;
		} else if ( ( 3 == floatee_action ) && ( std::abs( yv ) > 0.5 ) ) {
			yv *= -1.;
		} else {
			CTank* floatee_tgt = global.get_random_tank();
			if ( floatee_tgt ) {
				CWeapon* launchWeap =
					FUNKY_BOMBLET == weap_type ? &weapon[ FUNKY_BOMB ]
					: FUNKY_DEATHLET == weap_type
						? &weapon[ FUNKY_DEATH ]
						: nullptr;
				double speed =
					( launchWeap->launchSpeed
				          + ROUND( ( launchWeap ? launchWeap->speedVariation : 0.0 )
				                   * ( launchWeap ? launchWeap->launchSpeed : 0.0 )
				                   * noise( get_rand() % 1000000 ) ) )
					* env.fps_mod;
				double fdiff = ABSDISTANCE2( floatee_tgt->x, floatee_tgt->y, x, y );
				xv           = ( floatee_tgt->x - x ) / fdiff * speed;
				yv           = ( floatee_tgt->y - y ) / fdiff * speed;
			}
		}
	}

	// Funky floats simply bounce on borders.
	if ( ( ( x + xv ) < 1 ) || ( ( x + xv ) > ( env.screen_width - 1 ) ) ) {
		xv = -xv;
	} else {
		x += xv;
	}

	// The same applies to the screen bottom,
	// but here according to floor type
	if ( ( y + yv ) >= env.screen_height ) {
		if ( WALL_RUBBER == env.current_wall_type ) {
			yv *= -BOUNCE_CHANGE;
			xv *= 0.95;
		} else if ( WALL_SPRING == env.current_wall_type ) {
			yv *= -SPRING_CHANGE;
			xv *= 1.05;
		} else if ( ( WALL_WRAP == env.current_wall_type ) && env.is_boxed && env.do_box_wrap ) {
			y = MENUHEIGHT + 1;
		} else {
			y            = env.screen_height;
			yv           = 0;
			hit_something = true;
			age          = max_age;
		}
	}

	// On the screen top the direction is just reversed, even if
	// there is a ceiling in boxed mode. However, if it is a wrap
	// ceiling, and the ceiling wrap is enabled, got to the bottom.
	else if ( ( y + yv ) <= MENUHEIGHT ) {
		if ( ( WALL_WRAP == env.current_wall_type ) && env.is_boxed && env.do_box_wrap ) {
			y = env.screen_height - 2;
		} else {
			yv *= -0.95;
			xv *= 0.95;
		}
	}

	// eventually apply yv
	y += yv;
}

void CMissile::apply_physics_normal() {
	// Standard physics can be applied
	CPhysicalObject::applyPhysics();

	// If a mind shot napalm jelly hit something, it is counted as
	// destroyed immediately.
	if ( hit_something && ( MT_MIND_SHOT == missile_type ) && ( NAPALM_JELLY == weap_type ) ) {
		destroy = true;
	}

	// mirvs trigger above ground
	if ( !hit_something && ( ( weap_type == SMALL_MIRV ) || ( weap_type == CLUSTER_MIRV ) ) && ( yv > 0 ) ) {

		int32_t above_ground = height_above_ground();

		if ( ( above_ground > 0 ) && ( above_ground < TRIGGER_HEIGHT ) ) {
			hit_something = true;
		}
	}


	// Missiles that get too slow on a rubber floor, trigger when stopped.
	if ( !hit_something && ( WALL_RUBBER == env.current_wall_type ) && ( ROUND( y ) >= ( env.screen_height - 2 ) )
	     && ( ( std::abs( xv ) + std::abs( yv ) ) < 0.8 ) ) {
		hit_something = true;
	}


	// Unless something is hit, smoke might be produced:
	if ( !hit_something && !global.skipping_computer_play && ( MT_MIND_SHOT != missile_type )
	     && !( get_rand() % ( env.frames_per_second / 10 ) ) ) {
		try {
			new CDecor( x, y, xv / env.frames_per_second, xv / env.frames_per_second, weap->radius / 20, DECOR_SMOKE, 0 );
		} catch ( std::exception& e ) {
			std::cerr << __func__ << " new CDecor: " << e.what() << std::endl;
		}
	}
}

void CMissile::apply_physics_other() {
	// Check X:
	if ( ( ( x + xv ) < 1 ) || ( ( x + xv ) > ( env.screen_width - 1 ) ) ) {
		if ( WALL_RUBBER == env.current_wall_type ) {
			xv *= -0.5;
		} else if ( WALL_SPRING == env.current_wall_type ) {
			xv *= -SPRING_CHANGE;
		} else if ( WALL_WRAP == env.current_wall_type ) {
			x = xv > 0. ? 1 : env.screen_width - 1;
		} else {
			x            = xv < 0. ? 1 : env.screen_width - 1;
			xv           = 0;
			hit_something = true;
		}
	}

	// Check Y :
	if ( ( ( y + yv ) >= env.screen_height ) || ( ( y + yv ) < MENUHEIGHT ) ) {
		yv *= -0.5;
		xv *= 0.95;
	}

	// Apply gravitation, digging types are reversed and scorch the ground:
	if ( PT_DIGGING == phys_type ) {
		yv -= env.fall_vector * 0.05;
	} else {
		yv += env.fall_vector * 0.05;
	}

	// Apply velocity
	y += yv;
	x += xv;
}

void CMissile::apply_physics_rolling() {
	// check whether anything is hit
	auto round_x = ROUND( x );
	auto round_y = ROUND( y );
	if ( ( x < 2 ) || ( x > ( env.screen_width - 3 ) ) || ( y > ( env.screen_height - 3 ) )
	     || ( PINK != getpixel( global.terrain, round_x, round_y ) ) ) {
		hit_something = true;
	}

	else {
		// To honour the size of the rollers, they have the following
		// rules about the path they roll:
		// - The small roller can climb four and fall six pixels.
		// - The large roller can climb six and fall nine pixels.
		// - The death roller can climb nine and fall twelve pixels.
		int32_t maxClimb = SML_ROLLER == weap_type ? 4 : LRG_ROLLER == weap_type ? 6 : DTH_ROLLER == weap_type ? 9 : 1;
		int32_t maxFall  = SML_ROLLER == weap_type ? 6 : LRG_ROLLER == weap_type ? 9 : DTH_ROLLER == weap_type ? 12 : 1;

		// get next surface pixel
		float surfY = static_cast< float >( global.surface[ ROUND( x + xv ) ].load( ATOMIC_READ ) ) - 1.f;

		// Check whether the terrain is going down
		if ( surfY > y ) {
			// Do not fall more than 'maxFall' pixels if terrain gives way.
			if ( surfY <= ( y + maxFall ) ) {
				y = surfY - 1;
			} else {
				y += maxFall;
			}
			// Do not fall through the floor:
			if ( y > ( env.screen_height - maxClimb ) ) {
				y = env.screen_height - maxClimb;
			}
		}

		// Check whether the terrain is going up
		else if ( surfY < y ) {
			// If terrain is going up, it can be climbed,
			// or the rollers path ends here
			if ( surfY >= ( y - maxClimb ) ) {
				y = surfY - 1;
			} else {
				// too steep!
				hit_something = true;
			}
		}

		// Normal transversal movement, one pixel per step at 60 FPS.
		// Bank fractions so rollers keep their wall-clock speed at any rate:
		roll_carry += 1. / env.frame_count_mod;
		while ( roll_carry >= 1. ) {
			roll_carry -= 1.;
			x += xv > 0 ? 1 : xv < 0 ? -1 : 0;

			// Adapt angle according to movement
			if ( xv > 0.0 ) {
				angle = ( angle + 3 ) % 256;
			}
			if ( xv < 0.0 ) {
				angle = ( angle + 253 ) % 256;
			}
		}

		// Fix y if the projectile threats to go through the floor
		if ( !hit_something && ( y > ( env.screen_height - 5 ) ) ) {
			y = env.screen_height - 5;
		}
	} // End of rolling projectile movement
}

sSDI* CMissile::build_sdi_list( sSDI* sdi ) {
	// Reset SDI list:
	for ( int32_t i = 0; i < MAXPLAYERS; i++ ) {
		sdi[ i ].next = nullptr;
	}

	// Create the SDI list
	CTank*   lt   = nullptr;
	sSDI*   pSDI = nullptr;
	int32_t idx  = 0;

	global.get_head_of_class( CLASS_TANK, &lt );
	while ( lt ) {
		/* A tank is not considered for SDI shots if:
		 * 1 The tank is destroyed (obviously)
		 * 2 The tank is this missile owners tank
		 * 3 The tank is flying
		 * 4 The tank already has an SDI beam firing
		 * 5 The SDI has no shots left for this sequence
		 * 6 The SDI beam would shoot downwards.
		 */
		if ( !lt->destroy                                             // 1
		     && ( lt->player != player )                              // 2
		     && !lt->is_flying()                                       // 3
		     && !lt->player->sdi_has_fired.load( ATOMIC_READ )        // 4
		     && ( lt->player->ni[ ITEM_SDI ] > lt->player->sdi_shots ) // 5
		     && ( ( lt->y - 10. ) >= y ) ) {                          // 6
			double start_x    = lt->x;
			double start_y    = lt->y - 10.;
			sdi[ idx ].am    = lt->player->ni[ ITEM_SDI ] - lt->player->sdi_shots;
			sdi[ idx ].lvl   = ( sdi[ idx ].am - ( static_cast< int32_t >( sdi[ idx ].am ) % 5 ) ) / 5.;
			sdi[ idx ].mod   = 1. + ( std::min( 10., sdi[ idx ].lvl ) / 20. );
			sdi[ idx ].tank  = lt;
			sdi[ idx ].range = static_cast< double >( SDI_DISTANCE );
			sdi[ idx ].dist  = FABSDISTANCE2( x, y, start_x, start_y );
			sdi[ idx ].x     = start_x;
			sdi[ idx ].y     = start_y;


			/* add the SDI to the list if:
			 * 1: The missile is within maximum range
			 *    Note: For every 5 SDIs the maximum range is raised by 5% with a maximum of 50%
			 * 2: but further away than the minimum distance and
			 * 3: no dirt is between the gun top and the missile.
			 */
			if ( ( sdi[ idx ].dist <= ( sdi[ idx ].range * sdi[ idx ].mod ) ) // 1
			     && ( sdi[ idx ].dist > lt->player->ni[ ITEM_SDI ] )          // 2
			     && !check_pixels_between_two_points( &start_x, &start_y, x, y, 0.0, nullptr ) /* 3 */ ) {
				// This can be added!
				if ( pSDI ) {
					// Must be sorted in
					sSDI* curr = pSDI;
					while ( curr->next && ( curr->next->dist < sdi[ idx ].dist ) ) {
						curr = curr->next;
					}
					// Now curr->next is either nullptr or is farther away.
					sdi[ idx ].next = curr->next;
					curr->next      = &sdi[ idx ];
				} else {
					// It is the new head
					pSDI = &sdi[ idx ];
				}
				++idx;
			} // end of in range
		}         // End of having SDI
		lt->get_next( &lt );
	} // End of looping tanks

	return pSDI;
}

void CMissile::check_cluster() {
	CWeapon* submunition    = &weapon[ weap->submunition ];
	double  divergenceStep = static_cast< double >( weap->divergence ) / static_cast< double >( weap->numSubmunitions - 1 );
	int32_t startPoint     = divergenceStep < 0. ? 0 : 180;
	int32_t randStart      = get_rand() % 1000000;
	EPhysType submunitionPhys = PT_NORMAL;
	double    inheritedXV     = weap->impartVelocity * xv;
	double    inheritedYV     = weap->impartVelocity * yv;
	int32_t   start_y          = ROUND( y ) - 20;
	bool      ceiling_crash   = false;

	// See whether the weapon was fired into a ceiling:
	// This applies for both steel ceilings and wrap ceilings,
	// but the latter only if no ceiling wrap is activated or
	// if the next pixel at the bottom is dirt.
	if ( env.is_boxed && ( start_y <= MENUHEIGHT ) // Base condition
	     && ( ( ( WALL_WRAP == env.current_wall_type )
	            && ( !env.do_box_wrap // <- No wrap makes it steel
	                                  // \/ dirt makes the ceiling unwrapable
	                 || ( global.surface[ ROUND( x ) ].load( ATOMIC_READ ) < env.screen_height ) ) )
	          || ( WALL_STEEL == env.current_wall_type ) ) ) { // This always blasts
		ceiling_crash = true;
		// If the weapon is fired into a ceiling, adapt starting y
		start_y = MENUHEIGHT + 20;
	}

	// if napalm is going off, play its burn out sound
	if ( ( weap_type >= SML_NAPALM ) && ( weap_type <= LRG_NAPALM ) ) {
		play_explosion_sound( weap_type, ROUND( x ), 128 + ( weap->radius / 2 ), 1000 );
	}

	// Change physics of the submunitions for the funky bombs
	if ( ( weap_type == FUNKY_BOMB ) || ( weap_type == FUNKY_DEATH ) ) {
		submunitionPhys = PT_FUNKY_FLOAT;
	}

	// If this is a steel wall hit, the start point angle needs
	// to be adapted.
	if ( ( WALL_STEEL == env.current_wall_type ) && !ceiling_crash ) {
		if ( ( CLUSTER <= weap_type ) && ( SUP_CLUSTER >= weap_type ) ) {
			if ( x < 2 ) {
				startPoint -= weap->divergence + 1 + ( get_rand() % 10 );
			} else if ( x > ( env.screen_width - 3 ) ) {
				startPoint += weap->divergence + 1 + ( get_rand() % 10 );
			}
		} else if ( ( SML_NAPALM <= weap_type ) && ( LRG_NAPALM >= weap_type ) ) {
			if ( x < 2 ) {
				startPoint -= 10 + get_rand() % 21;
			} else if ( x > ( env.screen_width - 3 ) ) {
				startPoint += 10 + get_rand() % 21;
			}
		}
	}

	// If this is a ceiling crash, the start point angle needs
	// to be erased.
	if ( ceiling_crash ) {
		startPoint = 0;
		if ( ( SMALL_MIRV == weap_type ) || ( CLUSTER_MIRV == weap_type ) ) {
			inheritedYV = std::abs( inheritedYV );
		}
	}

	// The spread can be created!
	for ( int32_t sc = 0; sc < weap->numSubmunitions; ++sc ) {
		CMissile* newmis       = nullptr;
		double   launchSpeed  = weap->launchSpeed;
		int32_t  newMissCount = submunition->countdown;
		auto     newMissAngle = ROUND(
                        ( divergenceStep * static_cast< double >( sc ) ) + static_cast< double >( startPoint )
                        - ( static_cast< double >( weap->divergence ) / 2. )
                );

		// Manipulate angle if applicable
		if ( weap->spreadVariation > 0. ) {
			newMissAngle += ROUND(
				static_cast< double >( weap->divergence ) * weap->spreadVariation * noise( randStart + 1054 + sc )
			);
		}

		// Be sure the angle is valid
		while ( newMissAngle < 0 ) {
			newMissAngle += 360;
		}
		newMissAngle %= 360;

		// Manipulate number of submunition projectiles if applicable
		if ( submunition->countVariation > 0 ) {
			newMissCount += ROUND(
				static_cast< double >( submunition->countdown ) * submunition->countVariation
				* noise( randStart + 78689 + sc )
			);
			// This might go wrong, so be sure it doesn't
			if ( newMissCount <= 0 ) {
				newMissCount = 0;
			}
		}

		// Manipulate launching speed if applicable
		if ( weap->speedVariation > 0 ) {
			launchSpeed += ROUND( weap->speedVariation * weap->launchSpeed * noise( randStart + 124786 + sc ) );
		}

		// Launch new submunition missile
		// Note on funky floats: They do *not* home in on random
		// tanks when started, it is just a possibility in
		// applyPhysics() *only*
		try {
			newmis = new CMissile(
				player,
				x,
				start_y,
				env.slope[ newMissAngle ][ 0 ] * launchSpeed * env.fps_mod + inheritedXV,
				env.slope[ newMissAngle ][ 1 ] * launchSpeed * env.fps_mod + inheritedYV,
				weap->submunition,
				missile_type,
				ai_level,
				0
			);
			newmis->phys_type  = submunitionPhys;
			// Countdown is compared against the per-frame age, so scale the
			// fixed frame count to the frame rate (60 FPS baseline).
			newmis->countdown = ROUND( newMissCount * env.frame_count_mod );
			newmis->set_update_area( newmis->x - 20, newmis->y - 20, 40, 40 );
		} catch ( std::exception& e ) {
			std::cerr << __func__ << " new CMissile: " << e.what() << std::endl;
		}
	} // End of looping submunitions
}

bool CMissile::check_missile_hit( sSDI* sdi ) {
	bool will_hit = false;

	// Try to predict the coordinates where the missile will go down:
	CMissile mind_shot( player, x, y, xv, yv, weap_type, MT_MIND_SHOT, SDI_PREDICTOR, 0 );

	// Adapt missile drag if the player has dimpled/slick projectiles
	if ( player->ni[ ITEM_DIMPLEP ] ) {
		mind_shot.drag *= item[ ITEM_DIMPLEP ].vals[ 0 ];
	} else if ( player->ni[ ITEM_SLICKP ] ) {
		mind_shot.drag *= item[ ITEM_SLICKP ].vals[ 0 ];
	}

	// Keep flying/rolling/digging/whatever until the missile hits something
	// or the tank is out of explosion range
	double   x_dist    = sdi->x - mind_shot.x;
	double   y_dist    = sdi->y - mind_shot.y;
	double   x_vel     = 0.;
	double   y_vel     = 0.;
	uint32_t max_range = ROUNDu( sdi->range * std::max( ROUND( sdi->range ), weap->radius ) );
	mind_shot.get_velocity( x_vel, y_vel );

	// Apply physics until the missile is either destroyed, or
	// it is moving away from the tank and the tank is outside
	// the blast radius.
	while ( !mind_shot.destroy
	        && ( ( SIGN( x_dist ) == SIGN( x_vel ) ) || ( SIGN( y_dist ) == SIGN( y_vel ) )
	             || ( ABSDISTANCE2( sdi->x, sdi->y, mind_shot.x, mind_shot.y ) < max_range ) ) ) {
		mind_shot.applyPhysics();
		x_dist = sdi->x - mind_shot.x;
		y_dist = sdi->y - mind_shot.y;
		mind_shot.get_velocity( x_vel, y_vel );
	}

	// If the missile is destroyed, check whether the explosion would
	// a) hit this tank,
	// b) be nearer than the missile is now, and
	// c) will not kill the tank if shot down, and
	// d) will not be repulsed.
	// If so, shoot it down!
	if ( mind_shot.destroy ) {
		CTank*   lt    = sdi->tank;
		int32_t x_rad = DRILLER == weap_type ? weap->radius / 20 : weap->radius;
		int32_t y_rad = ( ( SHAPED_CHARGE <= weap_type ) && ( CUTTER >= weap_type ) ) ? weap->radius / 20 : weap->radius;
		double  tank_rad = lt->get_diameter() / 2.;

		if ( ( std::abs( x_dist ) <= x_rad )            // tank in x range
		     && ( std::abs( y_dist ) <= y_rad )         // tank in y range
		     && ( ( std::abs( x - sdi->x ) > x_rad )    // misses x radius now
		          || ( std::abs( y - sdi->y ) > y_rad ) // misses y radius now
		                                                // Is farther away than when it goes off:
		          || ( ABSDISTANCE2( x, y, lt->x, lt->y ) >= ABSDISTANCE2( mind_shot.x, mind_shot.y, lt->x, lt->y ) ) )
		     // Not a direct hit with repulsors up while not being buried.
		     && !( lt->is_in_box( mind_shot.x - tank_rad, mind_shot.y - tank_rad, mind_shot.x + tank_rad, mind_shot.y + tank_rad )
		           && lt->has_repulsor_activated() && ( ( BURIED_LEVEL / 4 ) > lt->how_buried( nullptr, nullptr ) ) ) ) {

			// The point looks promising, but is it worth it?
			double dmg = get_hit_damage( lt, static_cast< EWeaponType >( weap_type ), x, y );
			if ( dmg < ( lt->sh + lt->l ) ) {
				will_hit = true;
			}
		}
	}

	return will_hit;
}

bool CMissile::check_roller( double old_delta_x ) {
	bool quell = noimpact;
	if ( age > 1 ) {
		quell    = true; // No detonation, just switch to rolling
		phys_type = PT_ROLLING;
		age      = 0;

		// Set rolling start position and initial movement
		y  -= 5;
		xv  = 0;
		yv  = 0;

		// Possibly fix x
		if ( x <= 1 ) {
			x  = 1;
			xv = 1;
		} else if ( x >= ( env.screen_width - 2 ) ) {
			x  = env.screen_width - 2;
			xv = -1;
		}

		// Possibly fix y
		auto    round_x = ROUND( x );
		int32_t surf_y  = global.surface[ round_x ].load( ATOMIC_READ );
		if ( ( y >= surf_y )                // y is surface or below
		     && ( y <= ( surf_y + 2 ) ) ) { // but not buried more than 2 px
			y = surf_y - 1;
		}

		// Set movement if not done already:
		if ( ( xv > -0.9 ) && ( xv < 0.9 ) ) {
			bool can_go_left  = ( round_x > 3 );
			bool can_go_right = ( round_x < ( env.screen_width - 4 ) );

			if ( can_go_left || can_go_right ) {
				if ( can_go_left ) {
					can_go_left = ( PINK == getpixel( global.terrain, round_x - 1, y ) );
				}
				if ( can_go_right ) {
					can_go_right = ( PINK == getpixel( global.terrain, round_x + 1, y ) );
				}
			} // End of checking direction pixels

			if ( can_go_left && can_go_right ) {
				// Prefer old movement direction
				xv = SIGN( old_delta_x );
			} else if ( can_go_left ) {
				xv = -1;
			} else if ( can_go_right ) {
				xv = 1;
			} else {
				// nothing worked, both paths are blocked.
				xv = get_rand() % 2 ? -1 : 1;
			}
		}

		// If the roller is hammered into a wall, detonate it
		if ( ( ( WALL_STEEL == env.current_wall_type ) && ( ( x <= 2 ) || ( x >= ( env.screen_width - 3 ) ) ) )
		     || ( env.is_boxed && ( y <= MENUHEIGHT )
		          && ( ( ( WALL_WRAP == env.current_wall_type )
		                 && ( !env.do_box_wrap || ( global.surface[ ROUND( x ) ].load( ATOMIC_READ ) < env.screen_height ) ) )
		               || ( WALL_STEEL == env.current_wall_type ) ) ) ) {
			quell        = false;
			hit_something = true;
		}
	} // End of switching roller physics
	return quell;
}

// Check to see if any tanks have SDI defense. If they
// do, then see if this missile should be shot down.
// Returns the shooting tank if a shot is to be fired
// or NULL if no tank will shoot.
/* Update:
 * -------
 * check_sdi() can do it all on its own and is now called from
 * trigger_test() if (and *only* if) the missile isn't going off
 * anyway.
 * This allows SDI to be spared on missiles that are exploding anyway.
 * Further it makes the game loop much simpler.
 * - Sven
 *
 * Update:
 * -------
 * If an SDI is really considered to fire, a MIND SHOT is now used to
 * determine where the missile will explode, or whether it'll miss.
 * Tests showed, that it will result in a lower average number of
 * iterations than the old per-pixel check. And as a nice side effect,
 * the result is many times more accurate. ;-)
 * - Sven
 */
void CMissile::check_sdi() {
	static sSDI sdi[ MAXPLAYERS ];

	if (    // The Predictor don't checks itself:
		( SDI_PREDICTOR == ai_level )
		// can't shoot jelly, dirt balls, digging projectiles
		// and anything with submunition.
		|| ( PT_DIGGING == phys_type ) || ( weap_type == NAPALM_JELLY ) || ( weap->submunition > 0 )
		// Funky Floats are "invisible" with a chance of 50%
		|| ( ( PT_FUNKY_FLOAT == phys_type ) && ( get_rand() % 2 ) ) ) {
		return;
	}

	// Build the SDI list
	sSDI* pSDI = build_sdi_list( sdi );

	// Create the SDI list
	CTank* lt       = nullptr;
	bool  shotDown = false;


	// Move through the sorted list of SDI stations and see whether anybody
	// can shoot this one down.
	while ( !shotDown && pSDI ) {
		// 20% base chance with +1% per SDI over one.
		if ( ( get_rand() % 100 ) < ( 19 + pSDI->am ) ) {
			if ( check_missile_hit( pSDI ) ) {
				shotDown = true;

				// The actual shooting is only done if this is no mind shot
				if ( MT_MIND_SHOT != missile_type ) {
					lt = pSDI->tank;

					// The player has a 1% chance per SDI (with 50% max)
					// that one of the lasers burns out.
					// The chance can become this high to prevent players with
					// few missiles to shoot down to buy hundreds of SDI units.
					int32_t chance = pSDI->am > 50 ? 50 : pSDI->am;
					bool    burnt  = ( get_rand() % 100 ) < chance;

					try {
						new CBeam(
							lt->player,
							pSDI->x,
							pSDI->y,
							x,
							y,
							pSDI->am > 50   ? LRG_LAZER
							: pSDI->am > 25 ? MED_LAZER
									: SML_LAZER,
							burnt
						);

						trigger();

						if ( burnt ) {
							pSDI->tank->player->ni[ ITEM_SDI ]--;
						}

						pSDI->tank->player->sdi_has_fired.store( true, ATOMIC_WRITE );

					} catch ( ... ) {
						// Just not shot down. ;)
						shotDown = false;
					}
				} else {
					// But the bot needs to know that its shot ends here
					destroy = true;
				}
			} // end of not missing
		}
		pSDI = pSDI->next;
	} // End of going through SDI list
}

void CMissile::check_tanks() {
	CTank* lt = nullptr;

	// Has it hit a tank?
	global.get_head_of_class( CLASS_TANK, &lt );
	while ( lt ) {
		if ( !lt->destroy && lt->is_in_box( x, y, x, y ) ) {
			hit_something = true;
			if ( MT_MIND_SHOT != missile_type ) {
				lt->require_update();
			}

			// Be sure the detonation does not take place
			// on the gun top.
			if ( y < lt->y ) {
				y = lt->y; // I think we can live with this 'shift'.
			}
		}
		lt->get_next( &lt );
	}
}

// This function returns the distance above ground of
// the missile.
int32_t CMissile::height_above_ground() {
	auto rx = ROUND( x );

	if ( ( rx < 1 ) || ( rx >= env.screen_width ) ) {
		return -1;
	}

	double  sx     = xv / yv;
	double  px     = x + sx;
	double  py     = y + 1.;
	int32_t height = 1;

	while ( ( py < env.screen_height ) && ( px > .9 ) && ( px < ( env.screen_width - .9 ) )
	        && ( ( py < BOXED_TOP ) || ( PINK == getpixel( global.terrain, px, py ) ) ) ) {
		px += sx;
		py += 1.;
		++height;

		// If this is a wrapping wall, px must be wrapped of course
		if ( WALL_WRAP == env.current_wall_type ) {
			if ( px < 1. ) {
				px = env.screen_width - 1. - ( 1. - std::abs( px ) );
			}
			if ( px > ( env.screen_width - 1. ) ) {
				px = 1 + ( env.screen_width - 1. - px );
			}
		}
	}

	return height;
}

// Modify xv/yv according to repulse shields in the vicinity of the missile
void CMissile::repulse_missile() {
	CTank*  lt     = nullptr;
	double xaccel = 0;
	double yaccel = 0;

	global.get_head_of_class( CLASS_TANK, &lt );

	while ( lt ) {
		if ( !lt->destroy && ( lt->player != player ) ) {

			if ( lt->repulse( x + xv, y + yv, &xaccel, &yaccel, phys_type ) ) {
				xv += xaccel;
				yv += yaccel;
			}
		}
		lt->get_next( &lt );
	}
}

void CMissile::trigger() {
	// Create explosion
	try {
		new CExplosion( player, x, y, xv, yv, weap_type, is_weapon_fire );
	} catch ( std::exception& e ) {
		std::cerr << __func__ << " new CExplosion: " << e.what() << std::endl;
	}

	// If the explosion is near a wrapping wall, a second "fake"
	// explosion must be generated to display the wrapping effect
	if ( ( WALL_WRAP == env.current_wall_type ) && ( weap_type < WEAPONS ) ) {
		int32_t left  = 0;
		int32_t top   = 0;
		int32_t x_rad = weapon[ weap_type ].radius;
		int32_t y_rad = weapon[ weap_type ].radius;

		// Driller is smaller
		if ( DRILLER == weap_type ) {
			x_rad /= 20;
		}

		// shaped charges are flatter
		if ( ( SHAPED_CHARGE <= weap_type ) && ( CUTTER >= weap_type ) ) {
			y_rad /= 20;
		}

		// Set wrapped x position
		if ( x < x_rad ) {
			left = ROUND( env.screen_width + x );
		} else if ( x > ( env.screen_width - x_rad ) ) {
			left = ROUND( x - env.screen_width );
		}

		// (possibly) set wrapped y position
		if ( env.is_boxed && env.do_box_wrap ) {
			if ( y < ( y_rad + MENUHEIGHT ) ) {
				top = ROUND( env.screen_height + y );
			} else if ( y > ( env.screen_height - y_rad ) ) {
				top = ROUND( y - env.screen_height + MENUHEIGHT );
			}
		}

		// If an x/y-position is found, generate second explosion:
		if ( left || top ) {
			int32_t new_x = left ? left : ROUND( x );
			int32_t new_y = top ? top : ROUND( y );
			try {
				new CExplosion( player, new_x, new_y, xv, yv, weap_type, is_weapon_fire );
			} catch ( std::exception& e ) {
				std::cerr << __func__ << " new CExplosion: " << e.what() << std::endl;
			}
		}
	}

	destroy = true;

	if ( weap_type < SML_METEOR ) {
		play_explosion_sound( weap_type, ROUND( x ), 255, 1000 );
	} else {
		play_natural_sound( weap_type, ROUND( x ), 255, 1000 );
	}
}

void CMissile::trigger_test() {
	bool quell = noimpact;

	// No tests are needed if a too high velocity has
	// just lacerated the projectile.
	if ( lacerated ) {
		if ( ( MT_MIND_SHOT == missile_type ) || quell ) {
			// Only end of performance is needed to know.
			destroy = true;
		} else {
			trigger();
		}
		return;
	}

	// update hit_something if any tank was hit
	check_tanks();

	// Unless quelled, check what is to be done
	bool do_check = ( !quell && ( y > MENUHEIGHT ) );

	// Do not check if the missile is on screen, does not hit anything and the pixel check is negative (no dirt)
	if ( do_check && !hit_something && ( y < env.screen_height ) && ( x >= 0. ) && ( x < env.screen_width ) ) {
		do_check = ( PINK != getpixel( global.terrain, ROUND( x ), ROUND( y ) ) );
	}

	// Only continue if all checks passed:
	if ( do_check ) {
		// A roller changes from flying to rolling
		if ( ( weap_type >= SML_ROLLER ) && ( weap_type <= DTH_ROLLER ) && ( PT_NORMAL == phys_type ) ) {
			quell = check_roller( xv );
		} // End of roller handling

		// A burrower or penetrator changes from flying to digging
		else if ( ( weap_type == BURROWER ) || ( weap_type == PENETRATOR ) ) {
			if ( PT_DIGGING == phys_type ) {
				// If it hit, it must not be quelled.
				quell = !hit_something;
			} else if ( PT_NORMAL == phys_type ) {
				// This is the switch
				phys_type  = PT_DIGGING;
				quell     = true;
				xv       *= 0.1;
				yv       *= 0.1;
				age       = 0;
			}
		} // End of burrower handling

		// If a weapon has submunition, it goes off now
		else if ( weap->submunition >= 0 ) {
			quell = true; // This one is done

			if ( ( weap->numSubmunitions > 0 ) && ( MT_MIND_SHOT != missile_type ) ) {
				check_cluster();
			} // End of having submunition count

			destroy = true;
		} // End of having a submunition type defined
	}

	// If a countdown was set and this is old enough, this missile is no
	// longer quelled, regardless of what this means for this weapon type
	if ( ( countdown >= 0 ) && ( age >= countdown ) ) {
		quell = false;
	}

	// Riot charges always go off in an instant.
	if ( ( weap_type >= RIOT_CHARGE ) && ( weap_type <= RIOT_BLAST ) ) {
		quell   = false;
		destroy = true;
	}


	// Eventually trigger missiles that hit something or are lacerated
	if ( hit_something && !quell ) {
		if ( MT_MIND_SHOT == missile_type ) {
			destroy = true;
		} else {
			trigger();
		}
	} else if ( ( yv > 0. ) && ( ( weap_type < RIOT_BOMB ) || ( weap_type > SMALL_DIRT_SPREAD ) ) && ( weap_type < BALLISTICS ) ) {
		// Otherwise it is time to check whether any tank SDI shoots it down
		check_sdi();
	}
}

/// @brief special method to update private members iof sub munition missiles.
/// This method is only interesting for CAICore tracing clusters.
void CMissile::update_submun( EPhysType p_type, int32_t cnt_down ) {
	phys_type  = p_type;
	countdown = cnt_down;
}
