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


#include "teleport.h"

#include "environment.h"
#include "item.h"
#include "sound.h"
#include "tank.h"

#ifdef NETWORK
#  include "player.h"
#endif

CTeleport::~CTeleport() {
	require_update();
	update();
	if ( dim_cur.w > 0 ) {
		global.make_bgupdate( dim_cur.x, dim_cur.y, dim_cur.w, dim_cur.h );
	}
	if ( dim_old.w > 0 ) {
		global.make_bgupdate( dim_old.x, dim_old.y, dim_old.w, dim_old.h );
	}

	if ( remote ) {
		remote->destroy = true;
		remote->remote  = nullptr;
	}

	object = nullptr;
	remote = nullptr;

	// Take out of the chain:
	global.remove_object( this );
}

CTeleport::CTeleport(
	CVirtualObject* target_obj,
	int32_t         destination_x,
	int32_t         destination_y,
	int32_t         obj_radius,
	int32_t         duration,
	int32_t         type
)
	: CVirtualObject()
	, clock( duration )
	, object( target_obj )
	, radius( obj_radius )
	, start_clock( duration ) {

	if ( object ) {
		x = object->x;
		y = object->y;
	}

	// Ensure the destination is not occupied by another tank:
	bool need_check = ( type != ITEM_SWAPPER );
	while ( need_check ) {
		CTank* lt   = nullptr;
		need_check = false;

		global.get_head_of_class( CLASS_TANK, &lt );
		while ( lt ) {
			if ( ( std::abs( lt->x - destination_x ) < obj_radius ) && ( lt->y > destination_y )
			     && ( ( lt->y - destination_y ) < obj_radius ) ) {
				need_check = true;

				// Maybe move left
				if ( ( ( destination_x > ( obj_radius * 2 ) ) && ( destination_x <= lt->x ) )
				     || ( destination_x >= ( env.screen_width - ( obj_radius * 2 ) ) ) ) {
					destination_x -= ROUND( std::abs( lt->x - destination_x ) );
				}
				// Or move right
				else if ( destination_x < ( env.screen_width - ( obj_radius * 2 ) ) ) {
					destination_x += ROUND( std::abs( lt->x - destination_x ) );
				}

				// Maybe move up
				if ( ( ( destination_y > ( MENUHEIGHT + ( obj_radius * 2 ) ) ) && ( destination_y <= lt->y ) )
				     || ( destination_y >= ( env.screen_height - ( obj_radius * 2 ) ) ) ) {
					destination_y -= ROUND( std::abs( lt->y - destination_y ) );
				}
				// Or move down
				else if ( destination_y < ( env.screen_height - ( obj_radius * 2 ) ) ) {
					destination_y += ROUND( std::abs( lt->y - destination_y ) );
				}
			}


			lt->get_next( &lt );
		}
	} // end of needing to check the destination

	try {
		remote = new CTeleport( this, destination_x, destination_y );
	} catch ( std::bad_alloc& e ) {
		std::cerr << "Error creating CTeleport: " << e.what() << std::endl;
	}

	play_fire_sound( ITEM_TELEPORT + WEAPONS, ROUND( x ), 255, 1000 );

#ifdef NETWORK
	// this seems to be the teleport we usually use
	int   playerindex = 0;
	bool  found       = false;
	CTank* the_tank    = dynamic_cast< CTank* >( target_obj );

	// match the player with the tank
	while ( ( playerindex < env.num_game_players ) && ( !found ) ) {
		if ( ( env.players[ playerindex ]->tank ) && ( env.players[ playerindex ]->tank == the_tank ) ) {
			found = true;
		} else {
			++playerindex;
		}
	}

	if ( found ) {
		char buffer[ 64 ] = { 0x0 };
		snprintf( buffer, 63, "CTeleport %d %d %d", playerindex, destination_x, destination_y );
		env.send_to_clients( buffer );
	}
#endif // NETWORK

	// Add to the chain:
	global.add_object( this );
}

CTeleport::CTeleport( CTeleport* remote_end, int32_t dest_x, int32_t dest_y ) : CVirtualObject(), remote( remote_end ) {
	this->x = dest_x;
	this->y = dest_y;
	if ( remote ) {
		clock      = remote_end->start_clock;
		radius     = remote_end->radius;
		start_clock = remote_end->start_clock;
	}

	// Add to the chain:
	global.add_object( this );
}

void CTeleport::applyPhysics() {
	if ( object ) {
		if ( !clock ) {
			object->x      = remote->x;
			object->y      = remote->y;
			remote->object = object;
			object         = nullptr;
			remote->clock--;
		}
	} else {
		clock = remote->clock;
	}

	if ( clock-- < -start_clock / 2 ) {
		destroy = true;
	}
}

void CTeleport::draw() {
	if ( !remote ) {
		return;
	}

	double  pClock   = clock;
	int32_t blobSize = 8;
	int32_t pRadius  = radius;
	int32_t maxblobs = 1;

	// When the teleporting finishes, the blobs enlarge and disperse
	// using this then growing factor:
	if ( pClock < 1.0 ) {
		pClock = 1.0 + ( 1.0 - ( pClock * 2.0 ) );
	}

	auto transMod = ROUND( 255. - ( pClock / start_clock * 255. ) );
	if ( transMod > 255 ) {
		transMod = 255;
	} else if ( transMod < 0 ) {
		transMod = 0;
	}

	blobSize           -= ROUND( 8. / ( start_clock / pClock ) + 1. );
	pRadius            -= ROUND( radius / ( start_clock / pClock ) + 1. );
	maxblobs           += pRadius * 4;

	BITMAP* tempBitmap  = create_bitmap( radius * 2, radius * 2 );
	blit( global.canvas, tempBitmap, ROUND( remote->x - radius ), ROUND( remote->y - radius ), 0, 0, radius * 2, radius * 2
	);

	if ( object && remote ) {
		remote->draw();
	}

	drawing_mode( DRAW_MODE_TRANS, nullptr, 0, 0 );
	set_trans_blender( 0, 0, 0, transMod );

	for ( auto i = ROUND( maxblobs + pClock ); i > pClock; --i ) {
		auto    xOff  = ROUND( perlin2DPoint( 1.0, 200, 1278 + x + ( i * 100 ), pClock, 0.25, 6 ) * pRadius );
		auto    yOff  = ROUND( perlin2DPoint( 1.0, 200, 9734 + y + ( i * 100 ), pClock, 0.25, 6 ) * pRadius );
		int32_t t_col = getpixel( tempBitmap, pRadius + xOff, pRadius + yOff );
		circlefill( global.canvas, x + xOff, y + yOff, blobSize, t_col );
	}

	drawing_mode( DRAW_MODE_SOLID, nullptr, 0, 0 );

	set_update_area( x - pRadius - blobSize, y - pRadius - blobSize, ( pRadius + blobSize ) * 2, ( pRadius + blobSize ) * 2 );
	require_update();

	destroy_bitmap( tempBitmap );
}
