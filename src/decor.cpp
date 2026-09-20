#include "decor.h"

#include "random.h"
#include "sound.h"
#include "tank.h"
#include "weapon.h"


/// @brief Default constructor
CDecor::CDecor( double x_, double y_, double xv_, double yv_, int32_t max_radius, int32_t type_, int32_t delay_ )
	: CPhysicalObject( false )
	, cur_wind( global.wind )
	, delay( delay_ )
	, max_grav_accel( -4. * env.fall_vector )
	, max_wind( env.wind_strength )
	, max_wind_accel( global.wind * env.fps_mod )
	, radius( max_radius )
	, type( type_ ) {
	x  = x_;
	y  = y_;
	xv = xv_;
	yv = yv_;

	if ( DECOR_DIRT == type ) {
		// The core data is taken from the meteors.
		weap_type = SML_METEOR + ( max_radius / 2 ); // results in (0, 1, 1, 2, 2) for radius [1;5]
		mass     = naturals[ weap_type - WEAPONS ].mass;
		drag     = naturals[ weap_type - WEAPONS ].drag / 5.;

		// Special physics for dirt debris:
		phys_type = PT_DIRTBOUNCE;

		// Only keep dirt alive while it is really moving,
		// if it becomes too slow, only keep it for 2 seconds
		max_age = 2 * env.frames_per_second;

		// The diameter is just used so it does not have to
		// be calculated each time update_dirt() is called.
		diameter = radius * 2;

		// Calculate how many pixels are needed per call to update_dirt()
		grab_per_call = ( ( diameter + 1 ) * ( diameter + 1 ) ) / ( delay > 1 ? delay : 1 );
	} else if ( DECOR_SMOKE == type ) {
		int32_t tempCol = 128 + ( get_rand() % 64 );

		if ( max_radius <= 3 ) {
			radius = 3;
		} else {
			radius = 3 + ( get_rand() % ( max_radius - 2 ) );
		}

		color = makecol( tempCol, tempCol, tempCol );
		mass  = 1.0 + ( static_cast< double >( get_rand() % 5 ) / 10. );
		drag  = 0.9 + ( static_cast< double >( get_rand() % 90 ) / 100. );

		// maximum age depends on the maximum radius and the real radius,
		// plus 0 to 2 extra seconds.
		max_age  = ( ( max_radius - ( max_radius - radius ) ) / 3 ) + ( get_rand() % 3 );
		max_age *= env.frames_per_second;

		// Special physics for smoke, only for repulsion
		phys_type = PT_SMOKE;

		// Smoke does not need the dirt grabber
		ready = true;
	} else {
		destroy = true;
	}

	max_vel = env.max_velocity * ( 1.20 + ( mass / ( .01 * MAX_POWER ) ) );

	// Add to the chain:
	global.add_object( this );
}

/// @brief Constructor with bitmap
CDecor::CDecor(
	double       x_,
	double       y_,
	double       xv_,
	double       yv_,
	int32_t      max_radius,
	int32_t      type_,
	int32_t      delay_,
	sDebrisItem* deb_item,
	sDebrisItem* met_item
)
	: CDecor( x_, y_, xv_, yv_, max_radius, type_, delay_ ) {
	// Everything done in delegated ctor, only img to set
	dirt = deb_item;
	set_bitmap( dirt ? dirt->bmp : nullptr );
	// Note: It is safe to distribute dirt->bmp, because bitmap normally holds
	// global graphics and must not be destroyed.
	meteor = met_item;

	if ( ( nullptr == dirt ) || !has_bitmap() ) {
		// Can't work without...
		destroy = true;
	}
}

/// @brief default destructor
CDecor::~CDecor() {
	if ( DECOR_DIRT == type ) {
		// Draw dirt on terrain and add landslide
		rotate_sprite( global.terrain, dirt->bmp, ROUND( x - radius ), ROUND( y - radius ), itofix( angle ) );
		global.add_land_slide(x - radius - 1, x + radius + 1, false );
	}

	if ( dirt ) {
		global.free_debris_item( dirt );
		dirt = nullptr;
	}

	if ( meteor ) {
		global.free_debris_item( meteor );
		meteor = nullptr;
	}

	// Update the last drawing area
	int32_t calcRadius = radius;

	if ( DECOR_DIRT == type ) {
		++calcRadius;
	} else if ( DECOR_SMOKE == type ) {
		// The older, the larger...
		calcRadius = static_cast< int32_t >( radius * ( 4.0 * age / max_age ) );
	}

	set_update_area( x - calcRadius - 1, y - calcRadius - 1, ( calcRadius * 2 ) + 2, ( calcRadius * 2 ) + 2 );
	require_update();
	this->update();

	// Take out of the chain:
	global.remove_object( this );
}

/// @brief let smoke drift and disperse with the wind
void CDecor::applyPhysics() {
	if ( destroy ) {
		return;
	}

	if ( delay > 0 ) {
		--delay;
		return;
	}

	// for detecting bounces
	double old_yv = yv;

	if ( DECOR_DIRT == type ) {

		// Check whether movement ended
		double movement = FABSDISTANCE2( xv, yv, 0, 0 );
		bool   on_floor = is_on_floor(); // Needed again below.

		if ( on_floor && ( ( hit_something && ( movement < 0.8 ) ) || ( movement < 0.2 ) ) ) {

			// It ended!

			// fix y:
			auto dirt_bottom = ROUND( y + dirt->bmp->h );
			if ( ( ( y - radius ) > MENUHEIGHT ) && ( dirt_bottom < env.screen_height )
			     && ( PINK != getpixel( global.terrain, x, dirt_bottom ) ) ) {
				--y;
			}

			xv      = 0.;
			yv      = 0.;
			destroy = true;
			require_update();

		} else {
			hit_something = false; // Enable checking.

			// Now apply physics
			repulse_decor();
			CPhysicalObject::applyPhysics();

			// Be sure x/y values are sane (Can drift into walls
			// on rare wind conditions.)
			if ( x < 2 ) {
				x = 2;
			}
			if ( x > ( env.screen_width - 2 ) ) {
				x = env.screen_width - 2;
			}
			if ( y > ( env.screen_height - 2 ) ) {
				y = env.screen_height - 2;
			}

			// Maybe play a sound on bounce
			if ( !global.skipping_computer_play && ( old_yv > .5 ) && ( yv < -0.1 ) ) {
				play_natural_sound( DIRT_FRAGMENT, ROUND( x ), radius * 16, 1200 - ( radius * 50 ) );
			}
		}

		// raise age if movement is below 0.5
		if ( ( on_floor || ( FABSDISTANCE2( xv, yv, 0, 0 ) < .5 ) ) && ( ++age > max_age ) ) {
			destroy = true;
		}

	} else if ( DECOR_SMOKE == type ) {
		// Apply wind first
		int32_t ageMod = ROUND( std::abs( cur_wind / ( max_wind / 2.0 ) ) ) + 1;

		/* This produces: (with max wind = 8)
		 * wind = 0 : round(0 / (8 / 2)) + 1 = round(0 / 4) + 1 = 0 + 1 = 1 <-- normal aging
		 * wind = 1 : round(1 / (8 / 2)) + 1 = round(1 / 4) + 1 = 0 + 1 = 1 <-- normal aging
		 * wind = 4 : round(4 / (8 / 2)) + 1 = round(4 / 4) + 1 = 1 + 1 = 2 <-- raised aging
		 * wind = 6 : round(6 / (8 / 2)) + 1 = round(6 / 4) + 1 = 2 + 1 = 3 <-- fast aging
		 * wind = 8 : round(8 / (8 / 2)) + 1 = round(8 / 4) + 1 = 2 + 1 = 3 <-- fast aging
		 */
		age += ageMod;

		// Set further values
		// Try to reach half distance to the maximum values per second
		double xaccel = ( ( xv + max_wind_accel ) / 2 ) / static_cast< double >( env.frames_per_second );
		double yaccel = ( ( yv + max_grav_accel ) / 2 ) / static_cast< double >( env.frames_per_second / 10. );

		// Apply current acceleration
		xv += xaccel;
		yv += yaccel;

		// Add repulsion:
		repulse_decor();

		// Be sure that neither xv outruns wind nor yv is
		// higher than reverse gravity
		if ( std::abs( xv ) > std::abs( cur_wind ) ) {
			xv = cur_wind;
		}
		if ( yv < max_grav_accel ) {
			yv = max_grav_accel;
		}

		// Don't push through the floor
		if ( ( y + yv ) >= env.screen_height ) {
			yv *= -0.5;
			xv *= 0.95;
		}

		// The faster the smoke is blown by the wind, the less it rises:
		if ( ( yv < -1. ) && ( std::abs( xv ) > 1. ) ) {
			yv = ( yv + ( yv / std::abs( xv ) ) ) / 2.;
		}
		// and if the smoke is going down, halve yv
		else if ( yv > 0. ) {
			yv /= 2.;
		}

		// Now the velocity can be applied.
		x += xv;
		y += yv;

		// Destroy the smoke if it goes off-screen or is diffused
		auto calcRadius = ROUND( radius * ( 4.0 * age / max_age ) );

		if ( ( x < ( 1 - calcRadius ) ) || ( x >= ( env.screen_width + calcRadius ) )
		     || ( y < ( MENUHEIGHT - calcRadius ) ) || ( age > max_age ) ) {
			destroy = true;
		}
	}
}

/// @brief draw decor according to current settings and type.
void CDecor::draw() {
	if ( !ready && !destroy ) {
		update_dirt();
		if ( ready ) {
			// finished! See whether there are enough pixels
			if ( got_pixels <= radius ) {
				// nope.
				destroy = true;
			}
		}
	}

	if ( destroy || ( delay > 0 ) ) {
		return;
	}

	int32_t calcRadius = radius;

	if ( DECOR_DIRT == type ) {
		// Rotate according to xv and yv
		angle += ROUND( yv + ( ( SIGNd( xv ) * 5. ) - xv ) );

		// Be sure the angle is in order:
		if ( angle < 0 ) {
			angle += 360;
		}
		if ( angle > 360 ) {
			angle -= 360;
		}

		// And draw it:
		if ( y > MENUHEIGHT ) {
			CPhysicalObject::draw();
			++calcRadius;
		}
	} else if ( DECOR_SMOKE == type ) {
		// The older, the larger...
		calcRadius = static_cast< int32_t >( radius * ( 4.0 * age / max_age ) );

		drawing_mode( DRAW_MODE_TRANS, nullptr, 0, 0 );
		set_trans_blender( 0, 0, 0, 255 - ( 255 * age / max_age ) );
		circlefill( global.canvas, x, y, calcRadius, color );
	}

	drawing_mode( global.current_drawing_mode, nullptr, 0, 0 );

	set_update_area( x - calcRadius - 1, y - calcRadius - 1, ( calcRadius * 2 ) + 2, ( calcRadius * 2 ) + 2 );
	require_update();
}

/// In case of too much decor for the machine, allow forced ageing
void CDecor::force_aging( int32_t frames ) {
	age += frames;
	if ( age > max_age ) {
		destroy = true;
	}
}

/// return true if a dirt debris item "lies" on the floor, or is squeezed in a
/// dirt slide.
bool CDecor::is_on_floor() {
	int32_t scr_r_x = env.screen_width - 2;  // shortcut;
	int32_t scr_b_y = env.screen_height - 2; // ditto;

	// If the debris is above the screen or directly on the floor,
	// return at once:
	if ( y <= MENUHEIGHT ) {
		return false;
	}
	if ( y >= ( scr_b_y - radius ) ) {
		return true;
	}

	// Use safe values:
	auto round_x = ROUND( x );
	auto round_y = ROUND( y );

	// sanitize x value:
	if ( round_x < 1 ) {
		round_x = 1;
	} else if ( round_x > scr_r_x ) {
		round_x = scr_r_x;
	}

	// rounded boundaries, clipped to the screen:
	int32_t left   = std::max( 1, round_x - radius );
	int32_t top    = std::max( MENUHEIGHT, round_y - radius );
	int32_t right  = std::min( scr_r_x, round_x + radius );
	int32_t bottom = std::min( scr_b_y, round_y + radius );

	// Go from left to right and check whether the surface is above the bottom.
	int32_t surf_hits = 0;
	bool    on_floor  = false;
	for ( int32_t i = left; !on_floor && ( i <= right ); ++i ) {
		if ( global.surface[ i ].load( ATOMIC_READ ) <= bottom ) {
			// Actually this could mean that the debris is within
			// a hole in a mountain that hasn't been slid down, yet.
			bool in_dirt = false;

			for ( int32_t j = bottom; !in_dirt && ( j >= top ); --j ) {
				if ( PINK != getpixel( global.terrain, i, j ) ) {
					in_dirt = true;
				}
			}

			if ( in_dirt && ( ++surf_hits >= radius ) ) {
				on_floor = true;
			}
		}
	}

	return on_floor;
}

/// DIRT and Smoke (somewhat) can be repulsed, too
void CDecor::repulse_decor() {
	CTank*  lt     = nullptr;
	double xaccel = 0;
	double yaccel = 0;

	global.get_head_of_class( CLASS_TANK, &lt );

	while ( lt ) {
		if ( !lt->destroy ) {

			if ( lt->repulse( x + xv, y + yv, &xaccel, &yaccel, phys_type ) ) {
				xv += xaccel;
				yv += yaccel;
			}
		}
		lt->getNext( &lt );
	}
}

/// Small scale dirt grabber
void CDecor::update_dirt() {
	int32_t togo    = grab_per_call + 1;
	auto    deb_rad = static_cast< double >( radius );

	while ( togo ) {
		double deb_dist =
			FABSDISTANCE2( static_cast< double >( grab_x ), static_cast< double >( grab_y ), deb_rad, deb_rad );
		if ( deb_dist <= deb_rad ) {
			int32_t tcol = getpixel( dirt->bmp, grab_x, grab_y );

			// If this is a meteor and the terrain had no pixel
			// there, take one out of the rock instead.
			if ( ( PINK == tcol ) && meteor ) {
				tcol = getpixel( meteor->bmp, grab_x, grab_y );
			}

			// If this is valid, scorch the colour and put it back.
			if ( PINK != tcol ) {
				double deb_mod = deb_dist / deb_rad;
				auto   new_r   = ROUND( getr( tcol ) / ( 1.25 + deb_mod ) );
				auto   new_g   = ROUND( getg( tcol ) / ( 1.66 + deb_mod ) );
				auto   new_b   = ROUND( getb( tcol ) / ( 1.33 + deb_mod ) );
				putpixel( dirt->bmp, grab_x, grab_y, makecol( new_r, new_g, new_b ) );
				++got_pixels;
			}
		} // End of position in range

		else {
			// If the position is not in range, erase the surplus pixel
			putpixel( dirt->bmp, grab_x, grab_y, PINK );
		}

		// This point is done
		--togo;

		// Advance coordinates
		if ( ++grab_x > diameter ) {
			grab_x = 0;
			if ( ++grab_y > diameter ) {
				// end of work
				togo  = 0;
				ready = true;
				if ( meteor ) {
					global.free_debris_item( meteor );
					meteor = nullptr;
				}
			}
		}
	} // End of having pixels to grab
}
