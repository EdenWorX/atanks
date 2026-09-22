#include "shop.h"

#include "box.h"
#include "files.h"
#include "item.h"
#include "levelcreator.h"
#include "player.h"
#include "text.h" // for draw_text_in_box()
#include "weapon.h"

#include <algorithm>
#include <sstream>

#define SHOP_BAR_HEIGHT 29


/// ==== helper functions ====
static int32_t calc_potential_dmg( int32_t weap_num );
static void    divide_team_money();
static void    do_ai_shopping( CPlayer* pl, int32_t max_boost, int32_t max_score );
static void    draw_shop( CPlayer* pl );

/// ==== External functions used ====
void draw_simple_bg( bool drawImage );

/// ==== Shop-As-A-Class for easier handling ====
class Shop {
private:
	/* ======== Methods ======== */
	void check_mouse_buttons();
	void check_mouse_position();
	void check_mouse_wheel();
	void do_buy_sell( int32_t pl );
	void do_human_shopping( int32_t pl );
	void draw_shop_update( int32_t pl );
	void draw_weapon_list( CPlayer* pl );
	void finish( int32_t pl );
	void give_interests();
	void handle_move_up_down();
	void init();
	void perform_save_game();
	void put_into_trolley( int32_t pl );
	void reset( int32_t pl );
	void take_out_of_trolley( int32_t pl );


	/* ======== Members ======== */
	sBox           info_area{ 20, 60, 300, 400 };
	int32_t       btps{ 0 };
	string        info_text{ " " };
	bool          done{ false };
	int32_t       hoverOver{ -1 };
	int32_t       hoverOver_old{ -1 };
	int32_t       item_index{ 1 };
	int32_t       item_scrolled_old{ -1 };
	int32_t       item_scrolled_to{ 1 };
	bool          lb_pressed{ false };
	int32_t       lastMouse_b{ 0 };
	int32_t       lastMouse_x{ 0 };
	int32_t       lastMouse_y{ 0 };
	CLevelCreator* lvl_creator{ nullptr };
	int32_t       max_boost{ 0 };
	int32_t       max_score{ 0 };
	int32_t       money{ 0 };
	bool          need_draw{ false };
	bool          performed_save_game{ false };
	int32_t       selected_item{ -1 };
	int32_t       trolley[ THINGS ]{ 0 };
	int32_t       wheel_pos{ 0 };


public:
	explicit Shop( CLevelCreator* lvl_creator_ ) : lvl_creator( lvl_creator_ ) {}

	~Shop() = default;

	void operator() () {
		init();

		for ( int32_t pl = 0; ( pl < env.num_game_players ) && !global.is_close_btn_pressed(); pl++ ) {
			if ( HUMAN_PLAYER != env.players[ pl ]->type ) {
				// computer players have their own function for their shopping
				do_ai_shopping( env.players[ pl ], max_boost, max_score );
			} else {
				// Be sure no input from previous human players or from pressing
				// the "Play" button on the player selection screen carry over:
				flush_inputs();

				// First time drawing
				if ( !global.is_close_btn_pressed() ) {
					draw_shop( env.players[ pl ] );
				}

				// Reset shopping values
				reset( pl );

				// And hand over control:
				do_human_shopping( pl );

				// Now write back bought/sold items and remaining money
				finish( pl );
			}
		}

		// Eventually give all players interest on their remaining money
		give_interests();
	}
};

/// ==== Shop single responsibility methods ====

void Shop::check_mouse_buttons() {
	int32_t scrollArrowPos = env.screen_width - STUFF_BAR_WIDTH - 30;
	if ( ( mouse_x >= scrollArrowPos ) && ( mouse_x < ( scrollArrowPos + 24 ) ) ) {

		// Fast up
		if ( ( mouse_y >= ( env.half_height - 50 ) ) && ( mouse_y < ( env.half_height - 25 ) )
		     && ( item_scrolled_to > 1 ) ) {
			item_scrolled_to -= btps / 2;
			if ( item_scrolled_to < 1 ) {
				item_scrolled_to = 1;
			}
			need_draw = true;
		}

		// Up one item
		if ( ( mouse_y >= ( env.half_height - 24 ) ) && ( mouse_y < env.half_height ) && ( item_scrolled_to > 1 ) ) {
			--item_scrolled_to;
			need_draw = true;
		}

		// Down one item
		if ( ( mouse_y >= ( env.half_height + 1 ) ) && ( mouse_y < ( env.half_height + 25 ) )
		     && ( item_scrolled_to < ( env.num_available - btps ) ) ) {
			++item_scrolled_to;
			need_draw = true;
		}

		// Fast down
		if ( ( mouse_y >= ( env.half_height + 25 ) ) && ( mouse_y < ( env.half_height + 50 ) )
		     && ( item_scrolled_to < ( env.num_available ) ) ) {
			item_scrolled_to += btps / 2;
			if ( item_scrolled_to >= env.num_available - btps ) {
				item_scrolled_to = env.num_available - btps;
			}
			need_draw = true;
		}
	}
	if ( item_index < item_scrolled_to ) {
		item_index = item_scrolled_to;
	} else if ( item_index > ( item_scrolled_to + btps ) ) {
		item_index = item_scrolled_to + btps - 1;
	}
}

void Shop::check_mouse_position() {
	// Ensure the description shows what is selected
	int32_t hoverOver_new = env.available_items[ item_index ];

	if ( ( mouse_x >= ( env.screen_width - STUFF_BAR_WIDTH ) ) && ( mouse_x < env.screen_width ) ) {
		bool    isOver = false;
		int32_t zzz    = item_scrolled_to;

		for ( int32_t z = 1; ( z <= btps ) && !isOver; ++z ) {
			if ( ( mouse_y >= ( z * STUFF_BAR_HEIGHT ) ) && ( mouse_y < ( ( z * STUFF_BAR_HEIGHT ) + 30 ) ) ) {
				isOver = true;
			} else {
				++zzz;
			}
		}

		if ( isOver && ( hoverOver != env.available_items[ zzz ] ) ) {
			hoverOver_new = env.available_items[ zzz ];
			item_index    = zzz;
			need_draw     = true;
		}
	} // End of mouse_x in stuff bar

	// Switch description if necessary
	if ( hoverOver != hoverOver_new ) {
		if ( hoverOver_new > -1 ) {
			if ( hoverOver_new < WEAPONS ) {
				CWeapon const* weap = &weapon[ hoverOver_new ];
				info_text.assign( "Radius: " ).append( std::to_string( weap->radius ) );
				info_text.append( "\nYield : " )
					.append( std::to_string( calc_potential_dmg( hoverOver_new ) * weap->spread ) );
				info_text.append( "\n\n" ).append( weap->get_desc() );
			} else {
				int32_t itemNum = hoverOver_new - WEAPONS;
				CItem*   it      = &item[ itemNum ];
				if ( ( itemNum >= ITEM_VENGEANCE ) && ( itemNum <= ITEM_FATAL_FURY ) ) {
					double potDmg = calc_potential_dmg( ROUND( it->vals[ 0 ] ) ) * it->vals[ 1 ];
					info_text.assign( "Potential Damage: " ).append( std::to_string( ROUND( potDmg ) ) );
					info_text.append( "\n\n" ).append( it->get_desc() );
				} else {
					info_text.assign( it->get_desc() );
				}
			}
		} else {
			info_text.clear();
		}
		hoverOver = hoverOver_new;
		need_draw = true;

		draw_text_in_box( &info_area, info_text.c_str(), true );
	} // end of hovering on a different item
}

void Shop::check_mouse_wheel() {
	int32_t wheel_curr = mouse_z;
	if ( wheel_curr < wheel_pos ) {
		if ( ++item_scrolled_to >= ( env.num_available - btps ) ) {
			item_scrolled_to = env.num_available - btps;
		}
		if ( item_scrolled_to > item_index ) {
			item_index = item_scrolled_to;
		}
		need_draw = true;
	} else if ( wheel_curr > wheel_pos ) {
		if ( --item_scrolled_to < 1 ) {
			item_scrolled_to = 1;
		}
		if ( item_index >= ( item_scrolled_to + btps ) ) {
			item_index = item_scrolled_to + btps - 1;
		}
		need_draw = true;
	}
	wheel_pos = wheel_curr;
}

void Shop::do_buy_sell( int32_t pl ) {
	int32_t cost, amt, inInv;
	if ( selected_item < WEAPONS ) {
		cost  = weapon[ selected_item ].cost;
		amt   = weapon[ selected_item ].amt;
		inInv = env.players[ pl ]->nm[ selected_item ];
	} else {
		cost  = item[ selected_item - WEAPONS ].cost;
		amt   = item[ selected_item - WEAPONS ].amt;
		inInv = env.players[ pl ]->ni[ selected_item - WEAPONS ];
	}

	if ( key[ KEY_LCONTROL ] || key[ KEY_RCONTROL ] ) {
		cost *= 10;
		amt  *= 10;
	}

	// RMB sells items and takes precedence over LMB
	if ( lastMouse_b & 2 ) {
		if ( ( inInv + trolley[ selected_item ] ) >= amt ) {
			if ( trolley[ selected_item ] >= amt ) {
				money                    += cost;
				trolley[ selected_item ] -= amt;
				need_draw                 = true;
			} else if ( env.sell_percent > 0.01 ) {
				money                    += ROUND( cost * env.sell_percent );
				trolley[ selected_item ] -= amt;
				need_draw                 = true;
			}
		}
	} else if ( ( money >= cost ) && ( ( inInv + trolley[ selected_item ] ) < ( MAX_ITEMS_IN_STOCK - amt ) ) ) {
		if ( trolley[ selected_item ] <= -amt ) {
			if ( env.sell_percent > 0.01 ) {
				money                    -= ROUND( cost * env.sell_percent );
				trolley[ selected_item ] += amt;
				need_draw                 = true;
			}
		} else {
			money                    -= cost;
			trolley[ selected_item ] += amt;
			need_draw                 = true;
			if ( ( inInv + trolley[ selected_item ] ) > MAX_ITEMS_IN_STOCK ) {
				trolley[ selected_item ] = MAX_ITEMS_IN_STOCK;
			}
		}
	}
}

void Shop::do_human_shopping( int32_t pl ) {
	env.mouse_clock = 0;

	while ( !done ) {
		while ( !done && !need_draw ) {
			if ( global.is_close_btn_pressed() ) {
				lvl_creator->die_now();
				done = true;
				continue;
			}

			if ( ( lastMouse_x != mouse_x ) || ( lastMouse_y != mouse_y ) ) {
				lastMouse_x = mouse_x;
				lastMouse_y = mouse_y;
				if ( !env.os_mouse ) {
					need_draw = true;
				}
			}


			// Check the mouse button
			if ( !lb_pressed && ( mouse_b & 1 ) ) {
				// Check close shop button:
				if ( ( mouse_x >= ( env.half_width - 100 ) ) && ( mouse_x < ( env.half_width + 100 ) )
				     && ( mouse_y >= ( env.screen_height - 50 ) ) && ( mouse_y < ( env.screen_height - 25 ) ) ) {
					done = true;
				}
				env.mouse_clock = 0;
			}
			lb_pressed = ( mouse_b & 1 ) != 0;

			/* ========================
			 * === Keyboard control ===
			 * ========================
			 */
			handle_move_up_down();

			// buy or sell an item
			if ( ( K == KEY_RIGHT ) || ( K == KEY_D ) ) {
				put_into_trolley( pl );
			} else if ( ( K == KEY_LEFT ) || ( K == KEY_A ) ) {
				take_out_of_trolley( pl );
			} // end of buying/selling


			// check for adding or removing rounds
			else if ( ( K == KEY_PLUS_PAD ) || ( K == KEY_EQUALS ) ) {
				if ( ( env.rounds < MAX_ROUNDS ) && ( !env.mouse_clock ) ) {
					env.rounds++;
					global.current_round++;
					need_draw = true;
				}
			} else if ( ( K == KEY_MINUS_PAD ) || ( K == KEY_MINUS ) ) {
				if ( ( env.rounds > 1 ) && ( global.current_round > 1 ) && ( !env.mouse_clock ) ) {
					env.rounds--;
					global.current_round--;
					need_draw = true;
				}
			}

			// check for saving the game
			else if ( K == KEY_F10 ) {
				perform_save_game();
			}

			// Keyboard exit shop:
			if ( K == KEY_ENTER ) {
				done = true;
			}

			/* ========================
			 * === Mouse control    ===
			 * ========================
			 */

			// check mouse wheel
			check_mouse_wheel();

			// check mouse over items
			check_mouse_position();

			// Check mouse buttons against scrolling, buying and selling.
			if ( ( mouse_b & 1 ) && !env.mouse_clock ) {
				check_mouse_buttons();
			}

			// Check mouse buttons for clicks
			if ( ( ( mouse_b & 1 ) || ( mouse_b & 2 ) ) && ( mouse_x >= ( env.screen_width - STUFF_BAR_WIDTH ) )
			     && ( mouse_x < env.screen_width ) ) {
				selected_item = env.available_items[ item_index ];
			}

			// Only do the buying / selling when the mouse button is
			// released. This way users can pull the mouse off the item
			if ( ( selected_item > -1 ) && !( ( mouse_b & 1 ) || ( mouse_b & 2 ) ) ) {
				do_buy_sell( pl );
				selected_item = -1;
			} // end of mouse buttons released with item selected
			env.mouse_clock++;
			if ( env.mouse_clock > 5 ) {
				env.mouse_clock = 0;
			}
			lastMouse_b = mouse_b;

			// Sleep a bit if nothing happened
			if ( !done && !need_draw ) {
				LINUX_SLEEP;
			}
		} // end of input handling loop

		// If the close button was pressed, the creator thread must end ASAP and we have to get out of here.
		if ( global.is_close_btn_pressed() ) {
			lvl_creator->die_now();
			done = true;
			continue;
		}

		// Update display if anything happened:
		if ( need_draw ) {
			draw_shop_update( pl );
		} // end of drawing
	}         // end of player shopping loop
}

void Shop::draw_weapon_list( CPlayer* pl ) {
	// Some pre-calculations and settings.
	int32_t        start_x         = env.screen_width - STUFF_BAR_WIDTH;
	int32_t        halfBar        = STUFF_BAR_HEIGHT / 2;
	static int32_t qtyTxtLen      = 0;
	BITMAP*        imgReleased    = env.gfx_data.stuff_bar[ 0 ];
	BITMAP*        imgPressed     = env.gfx_data.stuff_bar[ 1 ];
	int32_t        col_add        = YELLOW;               // Bought items
	int32_t        col_sub        = makecol( 176, 0, 0 ); // Sold items
	static char    buf_cost[ 50 ] = { 0 };
	static char    buf_amt[ 50 ]  = { 0 };
	bool           full_redraw    = ( item_scrolled_to != item_scrolled_old );

	memset( buf_cost, 0, sizeof( char ) * 50 );
	memset( buf_amt, 0, sizeof( char ) * 50 );

	if ( 0 == qtyTxtLen ) {
		qtyTxtLen = text_length( font, "Qty. in inventory: ddd" );
	}

	// erase top gap:
	if ( full_redraw ) {
		global.lock_land();
		rectfill(
			global.canvas,
			start_x,
			STUFF_BAR_HEIGHT - 5,
			start_x + env.gfx_data.stuff_icon_base->w,
			STUFF_BAR_HEIGHT,
			makecol( 8, 110, 24 )
		);
		global.unlock_land();
		global.make_update( start_x, STUFF_BAR_HEIGHT - 5, env.gfx_data.stuff_icon_base->w, 5 );
	}

	// go through all items and draw them on the screen with
	// the amount of items in the trolley
	for ( int32_t slot = 1; ( slot <= btps ) && !global.is_close_btn_pressed(); ++slot ) {
		int32_t     itemIdx = slot + item_scrolled_to - 1;
		int32_t     itemNum = env.available_items[ itemIdx ];
		int32_t     start_y  = slot * STUFF_BAR_HEIGHT;
		char const* name    = nullptr;
		int32_t     amt     = 0;
		int32_t     d_div   = 1;

		// Only actually draw the slot, if it has changed:
		if ( !full_redraw && ( hoverOver_old != itemNum ) && ( hoverOver != itemNum ) ) {
			continue;
		}

		// Get text values:
		if ( itemNum < WEAPONS ) {
			d_div = weapon[ itemNum ].get_delay_div();
			name  = weapon[ itemNum ].get_name();
			amt   = pl->nm[ itemNum ] / d_div;
			snprintf( buf_cost, 49, "$%s", add_comma( weapon[ itemNum ].cost ) );
			snprintf( buf_amt, 49, "for %d", weapon[ itemNum ].amt / d_div );
		} else {
			name = item[ itemNum - WEAPONS ].get_name();
			amt  = pl->ni[ itemNum - WEAPONS ];
			snprintf( buf_cost, 49, "$%s", add_comma( item[ itemNum - WEAPONS ].cost ) );
			snprintf( buf_amt, 49, "for %d", item[ itemNum - WEAPONS ].amt );
		}

		// If the close button was pressed, the creator thread must end ASAP and we have to get out of here.
		if ( global.is_close_btn_pressed() ) {
			lvl_creator->die_now();
			done = true;
			continue;
		}

		global.lock_land();

		// Draw the background sprites
		draw_sprite( global.canvas, ( hoverOver == itemNum ) ? imgPressed : imgReleased, start_x, start_y );
		draw_sprite( global.canvas, env.gfx_data.stuff_icon_base, start_x, start_y );
		draw_sprite( global.canvas, env.stock[ itemNum ], start_x, start_y - 5 );
		global.make_update( start_x, start_y, STUFF_BAR_WIDTH, STUFF_BAR_HEIGHT + 5 );

		// Draw the text:
		textout_ex( global.canvas, font, name, start_x + 45, start_y - 1, BLACK, -1 );
		textprintf_ex(
			global.canvas,
			font,
			start_x + 45,
			start_y + halfBar - 4,
			BLACK,
			-1,
			"%s: %d",
			env.ingame->get_line( 40 ),
			amt
		);
		if ( trolley[ itemNum ] ) {
			textprintf_ex(
				global.canvas,
				font,
				start_x + 45 + qtyTxtLen,
				start_y + halfBar - 4,
				trolley[ itemNum ] > 0 ? col_add : col_sub,
				-1,
				"%+d",
				trolley[ itemNum ] / d_div
			);
		}
		textout_ex(
			global.canvas,
			font,
			buf_cost,
			env.screen_width - 45 - text_length( font, buf_cost ),
			start_y - 1,
			BLACK,
			-1
		);
		textout_ex(
			global.canvas,
			font,
			buf_amt,
			env.screen_width - 45 - text_length( font, buf_amt ),
			start_y + halfBar - 4,
			BLACK,
			-1
		);
		global.unlock_land();

		// Break up if done:
		// (This should not be triggered ever, as scroll is controlled by shop()
		//  to never be more than env.num_available-btps.)
		if ( ( itemIdx >= ( env.num_available - 1 ) ) && ( slot < btps ) ) {
			slot = btps + 1;
		}
	}

	fi = 1;
}

void Shop::draw_shop_update( int32_t pl ) {
	std::ostringstream txtbuf;
	need_draw = false;

	// No hardware mouse while drawing
	SHOW_MOUSE( nullptr )

	global.make_update( env.half_width - 200, 0, env.gfx_data.stuff_bar[ 0 ]->w, env.gfx_data.stuff_bar[ 0 ]->h );

	draw_sprite( global.canvas, env.gfx_data.stuff_bar[ 0 ], env.half_width - 200, 0 );
	textprintf_ex(
		global.canvas,
		font,
		env.half_width - 190,
		0,
		BLACK,
		-1,
		"%s %d: %s",
		env.ingame->get_line( 10 ),
		pl + 1,
		env.players[ pl ]->get_name()
	);
	textprintf_ex(
		global.canvas,
		font,
		env.half_width - 190,
		14,
		BLACK,
		-1,
		"%s: $%s",
		env.ingame->get_line( 11 ),
		add_comma( money )
	);

	txtbuf.str( "" );
	txtbuf << env.ingame->get_line( 12 ) << ": " << env.rounds - global.current_round << "/" << env.rounds;
	int txtlen = text_length( font, txtbuf.str().c_str() );
	textout_ex( global.canvas, font, txtbuf.str().c_str(), env.half_width + 180 - txtlen, 0, BLACK, -1 );

	txtbuf.str( "" );
	txtbuf << env.ingame->get_line( 13 ) << ": " << env.players[ pl ]->score;
	txtlen = text_length( font, txtbuf.str().c_str() );
	textout_ex( global.canvas, font, txtbuf.str().c_str(), env.half_width + 160 - txtlen, 14, BLACK, -1 );

	draw_weapon_list( env.players[ pl ] );

	// Update non-OS mouse movements
	SHOW_MOUSE( global.canvas )

	global.do_updates();
	hoverOver_old     = hoverOver;
	item_scrolled_old = item_scrolled_to;
}

void Shop::finish( int32_t pl ) {

	for ( int tItem = 0; tItem < WEAPONS; tItem++ ) {
		env.players[ pl ]->nm[ tItem ] += trolley[ tItem ];
	}
	for ( int tItem = WEAPONS; tItem < THINGS; tItem++ ) {
		env.players[ pl ]->ni[ tItem - WEAPONS ] += trolley[ tItem ];
	}
	env.players[ pl ]->money = money;
}

void Shop::give_interests() {
	for ( int32_t z = 0; ( z < env.num_game_players ) && !global.is_close_btn_pressed(); z++ ) {
		money            = env.players[ z ]->money;
		int32_t intLevel = 0;
		double  intSum   = 0.; // The summed-up interest
		DEBUG_LOG_FIN( env.players[ z ]->get_name(), "======================================================", 0 )
		DEBUG_LOG_FIN(
			env.players[ z ]->get_name(),
			"%2d.: %s enters the bank to get interest:",
			( z + 1 ),
			env.players[ z ]->get_name()
		)
		DEBUG_LOG_FIN( env.players[ z ]->get_name(), "     Starting Account: %10d", env.players[ z ]->money )
		DEBUG_LOG_FIN( env.players[ z ]->get_name(), "------------------------------------------------------", 0 )
		while ( money && ( intLevel++ < 5 ) ) {
			// Enter next level
			double intPerc  = ( env.interest - 1.0 ) / intLevel;
			double interest = money * intPerc;

			// The limit is only applicable on the first four levels,
			// in the fifth level interest is fully applied!
			if ( ( interest > MAX_INTEREST_AMOUNT ) && ( intLevel < 5 ) ) {
				interest = MAX_INTEREST_AMOUNT;
			}

			// Now sum the interest up and substract the counted money!
			intSum += interest;
			money  -= ROUND( interest / intPerc );

			DEBUG_LOG_FIN(
				env.players[ z ]->get_name(),
				"     Level %1d:  %8d credits are rated,",
				intLevel,
				static_cast< int32_t >( interest / intPerc )
			)
			DEBUG_LOG_FIN( env.players[ z ]->get_name(), "     Interest: %8d credits. (%5.2f%%)", interest, intPerc * 100. )

			// To get rid of (possible) rounding errors, add a security check:
			if ( ( money < ( 4 * intLevel ) ) || ( interest < 1 ) ) {
				money = 0; // With less there won't be any more interest anyway!
			}

			DEBUG_LOG_FIN( env.players[ z ]->get_name(), "     Unrated : %8d credits left.", money )
		}

		// Now give them their money:
		DEBUG_LOG_FIN( env.players[ z ]->get_name(), "     Sum:      %8d credits.", ROUND( intSum ) )
		DEBUG_LOG_FIN( env.players[ z ]->get_name(), "------------------------------------------------------", 0 )
		env.players[ z ]->money += ROUND( intSum );
		DEBUG_LOG_FIN( env.players[ z ]->get_name(), "     Final Account   : %10d", env.players[ z ]->money )
	} // End of looping players
}

void Shop::handle_move_up_down() {
	if ( keypressed() ) {
		k = readkey();
		K = k >> 8;
	} else {
		k = K = 0;
	}

	// Move up the list
	if ( ( K == KEY_UP ) || ( K == KEY_W ) ) {
		if ( item_index > 1 ) {
			item_index--;
		}
		if ( item_index < item_scrolled_to ) {
			item_scrolled_to = item_index;
		}
		need_draw = true;
	} else if ( ( K == KEY_PGUP ) || ( K == KEY_R ) ) {
		item_index -= btps;
		if ( item_index < 1 ) {
			item_index = 1;
		}
		if ( item_index < item_scrolled_to ) {
			item_scrolled_to = item_index;
		}
		need_draw = true;
	}

	// Move down the list
	else if ( ( K == KEY_DOWN ) || ( K == KEY_S ) ) {
		if ( item_index < ( env.num_available - 1 ) ) {
			item_index++;
		}
		if ( ( item_index - item_scrolled_to ) >= btps ) {
			item_scrolled_to = item_index - ( btps - 1 );
		}
		need_draw = true;
	} else if ( ( ( K == KEY_PGDN ) || ( K == KEY_F ) ) && ( item_scrolled_to <= ( env.num_available - btps ) ) ) {
		item_index += btps;
		if ( item_index > env.num_available - 1 ) {
			item_index = env.num_available - 1;
		}
		if ( ( item_index - item_scrolled_to ) >= btps ) {
			item_scrolled_to = item_index - ( btps - 1 );
		}
		need_draw = true;
	}

	// make sure the selected item is on the visible screen
	if ( item_index < item_scrolled_to ) {
		item_index = item_scrolled_to;
	} else if ( item_index >= ( item_scrolled_to + btps ) ) {
		item_index = item_scrolled_to + btps - 1;
	}
}

void Shop::init() {
	// If the close button was pressed, the creator thread must end ASAP and we have to get out of here.
	if ( global.is_close_btn_pressed() ) {
		lvl_creator->die_now();
		done = true;
		return;
	}

	draw_shop( nullptr );

	// Determine btps:
	btps = ROUNDu( ( env.screen_height - SHOP_BAR_HEIGHT ) / STUFF_BAR_HEIGHT );

	// Init global for drawing the shop:
	global.do_updates();
	global.stop_window = true;

	// before we do anything else, put a cap on money
	for ( int32_t z = 0; z < env.num_game_players; ++z ) {
		if ( env.players[ z ]->money > 1000000000 ) {
			env.players[ z ]->money = 1000000000;
		}
		if ( env.players[ z ]->money < 0 ) {
			env.players[ z ]->money = 0;
		}
	}


	if ( env.is_game_loaded ) {
		// after the first shopping loop the game isn't fresh any more
		env.is_game_loaded = false;
	} else {
		// Money dividing within the non-neutral teams only happens if no game
		// was loaded. Game saving is done after that rounds money dividing.
		divide_team_money();
	}


	// Determine maximum boost value and score
	for ( int32_t z = 0; z < env.num_game_players; ++z ) {
		int32_t boostValue = env.players[ z ]->get_boost_value();
		if ( boostValue > max_boost ) {
			max_boost = boostValue;
		}
		if ( env.players[ z ]->score > max_score ) {
			max_score = env.players[ z ]->score;
		}
	}

	// If this is demo mode, raise the max boost level, as there
	// are no human players to define a maximum value
	if ( global.demo_mode ) {
		max_boost += static_cast< int32_t >( env.rounds - global.current_round );
	}
}

void Shop::perform_save_game() {
	if ( !performed_save_game && save_game() ) {
		performed_save_game = true;
	}
	if ( performed_save_game ) {
		info_text.assign( env.ingame->get_line( 17 ) ).append( "\"" ).append( env.game_name ).append( "\"" );
	} else {
		info_text.assign( env.ingame->get_line( 41 ) );
	}
	draw_text_in_box( &info_area, info_text.c_str(), true );
	need_draw = true;
}

void Shop::put_into_trolley( int32_t pl ) {
	int32_t cost, amt, inInv;
	selected_item = env.available_items[ item_index ];
	if ( selected_item >= WEAPONS ) {
		cost  = item[ selected_item - WEAPONS ].cost;
		amt   = item[ selected_item - WEAPONS ].amt;
		inInv = env.players[ pl ]->ni[ selected_item - WEAPONS ];
	} else {
		cost  = weapon[ selected_item ].cost;
		amt   = weapon[ selected_item ].amt;
		inInv = env.players[ pl ]->nm[ selected_item ];
	}

	if ( key[ KEY_LCONTROL ] || key[ KEY_RCONTROL ] ) {
		cost *= 10;
		amt  *= 10;
	}

	if ( ( money >= cost ) && ( ( inInv + trolley[ selected_item ] ) < ( MAX_ITEMS_IN_STOCK - amt ) ) ) {
		if ( trolley[ selected_item ] <= -amt ) {
			if ( env.sell_percent > 0.01 ) {
				money                    -= ROUND( cost * env.sell_percent );
				trolley[ selected_item ] += amt;
				need_draw                 = true;
			}
		} else {
			money                    -= cost;
			trolley[ selected_item ] += amt;
			need_draw                 = true;
			if ( inInv + trolley[ selected_item ] > MAX_ITEMS_IN_STOCK ) {
				trolley[ selected_item ] = MAX_ITEMS_IN_STOCK;
			}
		}
	}
	selected_item = -1;
}

void Shop::reset( int32_t pl ) {
	done              = false;
	hoverOver         = -1;
	hoverOver_old     = -1;
	item_index        = 1;
	item_scrolled_old = -1;
	item_scrolled_to  = 1;
	lb_pressed        = false;
	money             = env.players[ pl ]->money;

	need_draw         = false;
	selected_item     = -1;

	std::fill( trolley, trolley + THINGS, 0 );
	wheel_pos = mouse_z;
}

void Shop::take_out_of_trolley( int32_t pl ) {
	int32_t cost, amt, inInv;
	selected_item = env.available_items[ item_index ];
	if ( selected_item >= WEAPONS ) {
		cost  = item[ selected_item - WEAPONS ].cost;
		amt   = item[ selected_item - WEAPONS ].amt;
		inInv = env.players[ pl ]->ni[ selected_item - WEAPONS ];
	} else {
		cost  = weapon[ selected_item ].cost;
		amt   = weapon[ selected_item ].amt;
		inInv = env.players[ pl ]->nm[ selected_item ];
	}

	if ( key[ KEY_LCONTROL ] || key[ KEY_RCONTROL ] ) {
		cost *= 10;
		amt  *= 10;
	}

	if ( inInv + trolley[ selected_item ] >= amt ) {
		if ( trolley[ selected_item ] >= amt ) {
			money                    += cost;
			trolley[ selected_item ] -= amt;
			need_draw                 = true;
		} else {
			if ( env.sell_percent > 0.01 ) {
				money                    += ROUND( cost * env.sell_percent );
				trolley[ selected_item ] -= amt;
				need_draw                 = true;
			}
		}
	}
	selected_item = -1;
}

/// ==== The Shop (tm) ====
bool shop( CLevelCreator* lvl_creator ) {
	Shop new_shop( lvl_creator );
	new_shop();


	// If the close button was pressed, the creator thread must end ASAP so we can get out of here.
	if ( global.is_close_btn_pressed() ) {
		lvl_creator->die_now();
	}


	// The CLevelCreator, if not finished, can work alone, now:
	if ( !lvl_creator->is_finished() ) {
		lvl_creator->work_alone();
	}


	// Wait until the level creator is done
	while ( !lvl_creator->is_finished() ) {
		MSLEEP( 20 );
		if ( lvl_creator->has_progress() ) {
			// Hide custom mouse pointer
			SHOW_MOUSE( nullptr )

			lvl_creator->print_state();

			// Draw custom mouse cursor
			SHOW_MOUSE( global.canvas )

			global.do_updates();
		}
	}


	return !global.is_close_btn_pressed();
}

/*
 *  Calculate the potential damage for a given weapon.
 */
static int32_t calc_potential_dmg( int32_t weap_num ) {
	CWeapon const* weap = &weapon[ weap_num ];

	if ( ( weap->submunition >= 0 ) && ( weap->numSubmunitions > 0 ) ) {
		return weapon[ weap->submunition ].damage * weap->numSubmunitions;
	}

	return weap->damage;
}

// If configured to do so, this method divides team money to help out team mates
static void divide_team_money() {
	if ( !env.divide_money ) {
		return;
	}

	int32_t jediMoney = 0;
	int32_t jediCount = 0;
	int32_t sithMoney = 0;
	int32_t sithCount = 0;
	int32_t teamFee;

	for ( int32_t z = 0; z < env.num_game_players; ++z ) {
		// Sum up team money:
		if ( env.players[ z ]->team == TEAM_JEDI ) {
			teamFee = ROUND( env.players[ z ]->money / 4. );
			if ( teamFee > MAX_TEAM_AMOUNT ) {
				teamFee = MAX_TEAM_AMOUNT;
			}
			jediMoney += teamFee;
			jediCount++;
		} else if ( env.players[ z ]->team == TEAM_SITH ) {
			teamFee = ROUND( env.players[ z ]->money / 4. );
			if ( teamFee > MAX_TEAM_AMOUNT ) {
				teamFee = MAX_TEAM_AMOUNT;
			}
			sithMoney += teamFee;
			sithCount++;
		}
		// Note: The team Fee is not docked, yet, as it is not clear
		// whether there is more than one team member.
	}

	DEBUG_LOG_FIN( "Overview", "Jedi Count: %d - Sith Count: %d", jediCount, sithCount )

	// Now apply the team money (if any):
	if ( jediCount > 1 ) {
		DEBUG_LOG_FIN( "Overview", "The Jedi summed up a pool of %13d credits!", jediMoney )
		jediMoney = ROUND( jediMoney * .9 / jediCount );
		DEBUG_LOG_FIN( "Overview", "Every Jedi will receive %10d credits out of the pool!", jediMoney )
		for ( int32_t z = 0; z < env.num_game_players; ++z ) {
			if ( TEAM_JEDI == env.players[ z ]->team ) {
				teamFee = ROUND( env.players[ z ]->money / 4. );
				if ( teamFee > MAX_TEAM_AMOUNT ) {
					teamFee = MAX_TEAM_AMOUNT;
				}
				env.players[ z ]->money -= teamFee;
				env.players[ z ]->money += jediMoney;
			}
		}
	}

	if ( sithCount > 1 ) {
		DEBUG_LOG_FIN( "Overview", "The Sith summed up a pool of %13d credits!", sithMoney )
		sithMoney = ROUND( sithMoney * .9 / sithCount );
		DEBUG_LOG_FIN( "Overview", "Every Sith will receive %10d credits out of the pool!", sithMoney )
		for ( int32_t z = 0; z < env.num_game_players; ++z ) {
			if ( TEAM_SITH == env.players[ z ]->team ) {
				teamFee = ROUND( env.players[ z ]->money / 4. );
				if ( teamFee > MAX_TEAM_AMOUNT ) {
					teamFee = MAX_TEAM_AMOUNT;
				}
				env.players[ z ]->money -= teamFee;
				env.players[ z ]->money += sithMoney;
			}
		}
	}
}

/// @brief dedicated function for AI shopping.
void do_ai_shopping( CPlayer* player, int32_t max_boost, int32_t max_score ) {
	// Print player info and inventory
#ifdef ATANKS_DEBUG_FINANCE
	DEBUG_LOG_FIN( player->get_name(), "Starting to buy: (Defensiveness: %4.2lf)", player->defensive )


	DEBUG_LOG_FIN( player->get_name(), " --- Inventory --- ", 0 )
	DEBUG_LOG_FIN( player->get_name(), "-------------------", 0 )
	for ( int32_t i = 1; i < WEAPONS; ++i ) {
		if ( player->nm[ i ] ) {
			DEBUG_LOG_FIN(
				player->get_name(),
				"% 4d x %s",
				player->nm[ i ] / weapon[ i ].get_delay_div(),
				weapon[ i ].get_name()
			)
		}
	}
	DEBUG_LOG_FIN( player->get_name(), " - - - - - - - - - ", 0 )
	for ( int32_t i = 1; i < ITEMS; ++i ) {
		if ( player->ni[ i ] ) {
			DEBUG_LOG_FIN( player->get_name(), "% 4d x %s", player->ni[ i ], item[ i ].get_name() )
		}
	}
	DEBUG_LOG_FIN( player->get_name(), "-------------------", 0 )

	int32_t oldMoneyToSave = -1; // So the same message isn't repeated over and over again.
#endif                               // ATANKS_DEBUG_FINANCE

	player->update_preferences( max_boost, max_score );

	// money saving will be made possible when:
	// 1. It's not the first three rounds
	// 2. It's not the last 5 rounds
	// and, dynamically:
	// 3. We have at least 10 parachutes or no gravity
	// 4. We have at least 2 damage dealing (not small missile) weapon
	//    per AI level.

	// Check for a minimum of damage dealing weapons and parachutes,
	// then buy until 'moneyToSave' is reached.
	auto    ai_level     = static_cast< int32_t >( player->type );
	int32_t buy_count    = 0;
	int32_t last_buy_idx = 0; // Used to "remember" where the AI was in its cart.
	int32_t pressed      = -1;

	do {
		int32_t moneyToSave = 0; // How much money will the player save?

		// The AI does not save up money in the first three or last five rounds
		if ( ( global.current_round > 5 ) && ( ( env.rounds - global.current_round ) > 3 ) ) {
			moneyToSave = player->get_money_to_save( !buy_count );
#ifdef ATANKS_DEBUG_FINANCE
			if ( oldMoneyToSave != moneyToSave ) {
				DEBUG_LOG_FIN( player->get_name(), "Maximum Money to save: %d (I have %d)", moneyToSave, player->money )
				oldMoneyToSave = moneyToSave;
			}
		} else {
			DEBUG_LOG_FIN( player->get_name(), "No money to save this round!", 0 );
		}
#else
		}
#endif // ATANKS_DEBUG_FINANCE

		int32_t numPara     = player->ni[ ITEM_PARACHUTE ];
		int32_t numDmgWeaps = 0;

		for ( int32_t i = 1; i < WEAPONS; ++i ) {
			// start from 1, as 0 is the small missile
			if ( weapon[ i ].damage > 0 ) {
				numDmgWeaps += player->nm[ i ] / weapon[ i ].get_delay_div();
			}
		}

		// Try to choose something to buy if enough money is there or either
		// the number of parachutes or damage dealing weapons is too low.
		if ( ( player->money > moneyToSave ) || ( ( numPara < ai_level ) && ( env.landslide_type > SLIDE_NONE ) )
		     || ( numDmgWeaps < ( ai_level * 2 ) ) ) {
			pressed = player->choose_item_to_buy( max_boost, last_buy_idx );
		} else {
			pressed = -1; // Forced to end.
		}

		DEBUG_LOG_FIN(
			player->get_name(),
			"I have %s%s%s%d credits left%s",
			pressed > -1 ? "bought: " : "finished, with ",
			pressed > -1 ? pressed < WEAPONS ? weapon[ pressed ].get_name() : item[ pressed - WEAPONS ].get_name() : "",
			pressed < 0 ? " " : " (",
			player->money,
			pressed < 0 ? "" : ")"
		)
		buy_count++;
	} while ( ( pressed != -1 ) && ( buy_count < 1000 ) );

	DEBUG_LOG_FIN( player->get_name(), "============================================", 0 )
}

static void draw_shop( CPlayer* pl ) {
	global.make_update( 0, 0, env.screen_width, env.screen_height );
	global.lock_land();
	SHOW_MOUSE( nullptr )
	draw_simple_bg( false );

	if ( pl ) {
		draw_sprite( global.canvas, env.misc[ DONE_IMAGE ], env.half_width - 100, env.screen_height - 50 );
		draw_sprite(
			global.canvas,
			env.misc[ FAST_UP_ARROW_IMAGE ],
			env.screen_width - STUFF_BAR_WIDTH - 30,
			env.half_height - 50
		);
		draw_sprite( global.canvas, env.misc[ UP_ARROW_IMAGE ], env.screen_width - STUFF_BAR_WIDTH - 30, env.half_height - 25 );
		draw_sprite( global.canvas, env.misc[ DOWN_ARROW_IMAGE ], env.screen_width - STUFF_BAR_WIDTH - 30, env.half_height );
		draw_sprite(
			global.canvas,
			env.misc[ FAST_DOWN_ARROW_IMAGE ],
			env.screen_width - STUFF_BAR_WIDTH - 30,
			env.half_height + 25
		);
	}

	drawing_mode( DRAW_MODE_TRANS, nullptr, 0, 0 );
	global.current_drawing_mode = DRAW_MODE_TRANS;

	if ( pl ) {
		double  left  = env.half_width - 200; // short cut
		int32_t right = env.screen_width - 1; // another short cut.

		for ( int32_t z = 0; z < env.half_width - 200; z++ ) {
			set_trans_blender( 0, 0, 0, ROUNDu( static_cast< double >( z ) / left * 240 ) + 15 );
			vline( global.canvas, z, 0, SHOP_BAR_HEIGHT, pl->color );
			vline( global.canvas, right - z, 0, SHOP_BAR_HEIGHT, pl->color );
		} // End of drawing player colour blending
	}         // End of having a player

	solid_mode();
	global.current_drawing_mode = DRAW_MODE_SOLID;

	if ( pl ) {
		textout_ex( global.canvas, font, env.ingame->get_line( 14 ), 20, 420, WHITE, -1 );
		textout_ex( global.canvas, font, env.ingame->get_line( 15 ), 20, 450, WHITE, -1 );
		textout_ex( global.canvas, font, env.ingame->get_line( 16 ), 20, 465, WHITE, -1 );
	}

	global.unlock_land();
	fi = 1;
}

void draw_simple_bg( bool drawImage ) {
	if ( !env.draw_background ) {
		rectfill( global.canvas, 0, 0, env.screen_width - 1, env.screen_height - 1, BLACK );
	} else if ( drawImage && env.misc[ 17 ] ) {
		stretch_blit(
			env.misc[ 17 ],
			global.canvas,
			0,
			0,
			env.misc[ 17 ]->w,
			env.misc[ 17 ]->h,
			0,
			0,
			env.screen_width,
			env.screen_height
		);
	} else {
		rectfill( global.canvas, 0, 0, env.screen_width - 1, env.screen_height - 1, DARK_GREEN );
	}
}

/** @brief Executes a fast and simple transition from global.canvas to the screen.
 **/
void quick_change( bool clearerror ) {
	if ( errorMessage ) {
		textout_ex( global.canvas, font, errorMessage, errorX, errorY, makecol( 255, 0, 0 ), -1 );
		if ( clearerror ) {
			errorMessage = nullptr;
		}
	}

	blit( global.canvas, screen, 0, 0, 0, 0, env.screen_width, env.screen_height );
}
