#include "satellite.h"

#include "beam.h"
#include "environment.h"
#include "random.h"

SATELLITE::SATELLITE() : x( env.screen_width / 2 ) {
	prev_x = x;
}

void SATELLITE::draw() const {
	drawing_mode( DRAW_MODE_SOLID, nullptr, 0, 0 );
	draw_sprite( global.canvas, env.misc[ SATELLITE_IMAGE ], ROUND( x ), y );
	global.make_update( ROUND( x ) - 20, y, 80, 60 );
	global.make_update( ROUND( prev_x ), y, 80, 60 );
}

void SATELLITE::move() {
	// Be sure an owned beam is valid
	if ( beam && beam->destroy ) {
		beam = nullptr;
	}

	// Frame-rate independent movement: acceleration and velocity are tuned
	// for 60 FPS, so scale both by the per-frame step (exact trajectory).
	double const step = 1. / env.frame_count_mod;

	// reverse movement if the satellite reaches the screen borders
	if ( x < -5 ) {
		xv += step;
	} else if ( x > ( env.screen_width - 20 ) ) {
		xv -= step;
	}

	prev_x  = x;
	x      += xv * step;

	// If the satellite is firing, move the beam
	if ( beam ) {
		beam->move_start( xv < 0 ? x + 10 : x + 40, y + 20 );
	}
}

void SATELLITE::shoot() {
	if ( ( SL_NONE != env.satellite ) && ( global.naturals_activated < 4 )
	     && ( nullptr == beam )
	     // 1% chance to fire
	     && ( !( get_rand() % 100 ) ) ) {
		try {
			beam = new CBeam(
				nullptr,
				xv < 0 ? x + 10 : x + 40,
				y + 20,
				get_rand() % 35 + ( xv < 0 ? 320 : 5 ),
				SML_LAZER + ( get_rand() % env.satellite ),
				BT_NATURAL
			);
			global.naturals_activated++;
		} catch ( ... ) {
			// No problem... fire another time. ;)
		}
	}
}
