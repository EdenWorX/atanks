#ifndef ATANKS_OPTIONITEMMENU_H_INCLUDED
#define ATANKS_OPTIONITEMMENU_H_INCLUDED 1

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

#include "optionitembase.h"

/** @file optionitemmenu.h
 * @brief declaration of the option entry class specialized on handling
 * CMenu instances
 **/


/** @class COptionItemMenu
 * @brief abstract one option menu entry to handle a CMenu instance
 *
 * This class is a special version of the TOptionItem template that can only
 * handle CMenu instances.
 *
 * The the only entry type supported is the ET_MENU.
 **/
class COptionItemMenu final : public COptionItemBase {
public:
	/* -------------------------------------------
	 * --- Public constructors and destructors ---
	 * -------------------------------------------
	 */

	/// Create a sub menu entry.
	explicit COptionItemMenu(
		CMenu*       menu_,
		char const* title_,
		int32_t     title_idx_,
		int32_t     color_,
		int32_t     top_,
		int32_t     left_,
		int32_t     width_,
		int32_t     height_,
		int32_t     padding_
	);
	/// Destroy a sub menu entry.
	~COptionItemMenu() final;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	/// Open the sub menu.
	int32_t activate( int32_t, int32_t, int32_t, int32_t ) final;
	/// Menus have no minimum.
	bool    canGoDown() final;
	/// Menus have no maximum.
	bool    canGoUp() final;
	/// Render the sub menu entry.
	void    display( bool show_full ) final;
	/// Menus are never exit buttons.
	bool    isExitButton() final;
	/// Retranslate the entry.
	void    set_language();

private:
	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	CMenu* menu = nullptr; //!< CMenu instance to handle
};


#endif // ATANKS_OPTIONITEMMENU_H_INCLUDED
