#ifndef ATANKS_SRC_OPTIONITEMMENU_H_INCLUDED
#define ATANKS_SRC_OPTIONITEMMENU_H_INCLUDED 1

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
 * Menu instances
 **/


/** @class OptionItemMenu
 * @brief abstract one option menu entry to handle a Menu instance
 *
 * This class is a special version of the OptionItem template that can only
 * handle Menu instances.
 *
 * The the only entry type supported is the ET_MENU.
 **/
class OptionItemMenu final : public OptionItemBase {
public:
	/* -------------------------------------------
	 * --- Public constructors and destructors ---
	 * -------------------------------------------
	 */

	explicit OptionItemMenu(
		Menu*       menu_,
		char const* title_,
		int32_t     titleIdx_,
		int32_t     color_,
		int32_t     top_,
		int32_t     left_,
		int32_t     width_,
		int32_t     height_,
		int32_t     padding_
	);
	~OptionItemMenu() final;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	int32_t activate( int32_t, int32_t, int32_t, int32_t ) final;
	bool    canGoDown() final;
	bool    canGoUp() final;
	void    display( bool show_full ) final;
	bool    isExitButton() final;
	void    setLanguage();

private:
	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	Menu* menu = nullptr; //!< Menu instance to handle
};


#endif // ATANKS_SRC_OPTIONITEMMENU_H_INCLUDED
