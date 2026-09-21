#include "optionitemmenu.h"

#include "clock.h"
#include "menu.h"

/** @brief Default constructor.
 *
 * The target is the CMenu instance to handle.
 *
 * Activation is a simple call to the Menus operator(), its return value
 * is then returned without further ado.
 *
 * @param[in,out] menu_ Pointer to the CMenu instance to handle.
 * @param[in] title_ The title of the option to display.
 * @param[in] title_idx_ Index value of the submitted title. -1 means @a title_ is fixed.
 * @param[in] color Regular display color of the title.
 * @param[in] top_ Top position of the display area.
 * @param[in] left_ Left position of the display area.
 * @param[in] width_ Width of the display area.
 * @param[in] height_ Height of the display area.
 * @param[in] padding_ Padding of the title and buttons to the display area.
 **/
COptionItemMenu::COptionItemMenu(
	CMenu*       menu_,
	char const* title_,
	int32_t     title_idx_,
	int32_t     color_,
	int32_t     top_,
	int32_t     left_,
	int32_t     width_,
	int32_t     height_,
	int32_t     padding_
)
	: COptionItemBase(
		ET_MENU,
		title_  ? title_
		: menu_ ? menu_->get_title()
			: nullptr,
		title_idx_,
		nullptr,
		color_,
		TC_NONE,
		nullptr,
		top_,
		left_,
		width_,
		height_,
		padding_,
		0
	)
	, menu( menu_ ) {
	// Both action or player must be set
	assert( menu_ && "A nullptr menu_ makes no sense..." );
	// As the title is displayed as text, text_only must be set:
	this->text_only = true;
}

/// @brief default dtor only setting nullptr values. No further action needed.
COptionItemMenu::~COptionItemMenu() {
	menu = nullptr;
}

/* ----------------------
 * --- Public methods ---
 * ----------------------
 */

/** @brief activate the sub menu
 *
 * This calls operator() on the menu.
 *
 * Note: The parameters are defined by COptionItemBase but unused
 * here.
 *
 * @return The return code of the sub menu.
 **/
int32_t COptionItemMenu::activate( int32_t, int32_t, int32_t, int32_t ) {
	// Remove parent menu timer
	WIN_CLOCK_REMOVE

	int32_t result = ( *menu )();

	// Changes are displayed at once:
	this->drawn = false;
	this->display( false );

	// Re-add parent menu timer
	WIN_CLOCK_INIT

	return result;
}

/// @brief returns always true
bool COptionItemMenu::canGoDown() {
	return true;
}

/// @brief returns always true
bool COptionItemMenu::canGoUp() {
	return true;
}

/** @brief display the sub menu title
 *
 * @param[in] show_full If set to true, title and buttons are redrawn.
 **/
void COptionItemMenu::display( bool show_full ) {
	this->display_menu( menu );

	// Show decorations if wanted:
	if ( show_full ) {
		this->display_deco();
	}
}

/// @brief return true, the menu must be able to return an exit code.
bool COptionItemMenu::isExitButton() {
	return true;
}

/// @brief simply calls set_language(false) on the target menu
void COptionItemMenu::set_language() {
	if ( menu ) {
		menu->set_language( false );
	}
}
