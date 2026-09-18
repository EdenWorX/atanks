#ifndef FLOATTEXT_DEFINE
#define FLOATTEXT_DEFINE

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


#include "environment.h"
#include "main.h"
#include "virtobj.h"

/// @enum eTextSway
/// @brief Type of text swaying
enum eTextSway {
	TS_NO_SWAY    = 0,  //!< Static text that is moving normally
	TS_VERTICAL   = 15, //!< Vertical "bouncing" text like tank health.
	TS_HORIZONTAL = 22  //!< Horizontal swaying text, if turned on, used for damage and money.
};

/** @class FLOATTEXT
 * @brief Floating label text.
 **/
class FLOATTEXT final : public VIRTUAL_OBJECT {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Create a floating text.
	explicit FLOATTEXT(
		char const* text_,
		double      xpos,
		double      ypos,
		double      xv_,
		double      yv_,
		int32_t     color_,
		alignType   alignment,
		eTextSway   sway_type,
		int32_t     max_age,
		bool        is_fixed_
	);
	/// Destroy a floating text.
	~FLOATTEXT() final;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void   applyPhysics() final;                       ///< Advance physics.
	void   draw() final;                               ///< Render the text.
	void   newRound();                                 ///< Expire round-scoped texts.
	void   set_color( int32_t color_ );                ///< Set the text color.
	void   set_pos( int32_t xpos, int32_t ypos );      ///< Set the text position.
	void   set_sway_type( eTextSway sway_type );       ///< Set the sway type.
	void   set_text( char const* text_ );              ///< Set the text content.

	/// Return the object class.
	eClass getClass() final { return CLASS_FLOATTEXT; }

	/* Little inline helper */
	/// Set the text position; coordinates are rounded.
	inline void set_pos( double xpos, double ypos ) { set_pos( ROUND( xpos ), ROUND( ypos ) ); }

private:
	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void    check_pos( bool is_new );
	int32_t overlaps_by( const FLOATTEXT* other );
	void    push_down( int32_t ydiff, bool is_new );
	void    reset_sway();
	void    set_speed( double xv_, double yv_ );


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t   color     = SILVER; //!< Foreground colour
	int32_t   halfColor = GREY;   //!< Shadow colour
	bool      is_fixed  = false;  //!< Whether new texts can push this out of the way
	bool      is_pushed = false;
	double    pos_x     = 0.;
	double    pos_y     = 0.;
	eTextSway sway      = TS_NO_SWAY;
	char*     text      = nullptr;
};

// This function returns a shade colour, which
// is either brighter or darker depending on
// the given colour and options.
int32_t GetShadeColor( int32_t colour, bool do_lighten, int32_t bg_colour );

#endif
