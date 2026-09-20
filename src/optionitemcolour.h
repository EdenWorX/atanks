#ifndef ATANKS_OPTIONITEMCOLOUR_H_INCLUDED
#define ATANKS_OPTIONITEMCOLOUR_H_INCLUDED 1

/*
 * atanks - obliterate each other with oversize weapons
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
 *
 */

#  include "optionitembase.h"

/** @file optionitemcolour.h
 * @brief declaration of the option entry class specialized on handling
 * int32_t instances representing colours.
 **/


/** @class COptionItemColour
 * @brief abstract one option menu entry to handle an int32_t instance
 *
 * This class is a special version of the TOptionItem template that can only
 * handle int32_t instances representing colours.
 *
 * The the only entry type supported is the ET_COLOR.
 **/
class COptionItemColour final : public COptionItemBase {
public:
	/* -------------------------------------------
	 * --- Public constructors and destructors ---
	 * -------------------------------------------
	 */

	/// Create a color entry.
	explicit COptionItemColour(
		int32_t*    color_,
		char const* title_,
		int32_t     title_idx_,
		int32_t     top_,
		int32_t     left_,
		int32_t     width_,
		int32_t     height_,
		int32_t     padding_,
		int32_t     show_size_
	);
	/// Destroy a color entry.
	~COptionItemColour() final;

	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	/// Handle activation with click position.
	int32_t activate( int32_t, int32_t, int32_t, int32_t ) final;
	/// Colors have no minimum.
	bool    canGoDown() final;
	/// Colors have no maximum.
	bool    canGoUp() final;
	/// Render the color box.
	void    display( bool show_full ) final;
	/// Colors are never exit buttons.
	bool    isExitButton() final;


private:
	/* ----------------------------------------------
	 * --- Private methods and external functions ---
	 * ----------------------------------------------
	 */


	void display_cross();


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t  act_x      = 0;
	int32_t  act_y      = 0;
	BITMAP*  tgt_bitmap = nullptr; //!< Pre-drawn Rainbow box
	int32_t* tgt_color  = nullptr; //!< Colour instance to handle
};


#endif // ATANKS_OPTIONITEMCOLOUR_H_INCLUDED
