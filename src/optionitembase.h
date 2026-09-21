#ifndef ATANKS_OPTIONITEMBASE_H_INCLUDED
#define ATANKS_OPTIONITEMBASE_H_INCLUDED 1

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

#include "environment.h"
#include "globaldata.h"
#include "optiontypes.h"

#include <cassert>
#include <string>


/** @file optionitembase.h
 * @brief declaration of the option item base class
 **/

extern int32_t select_text_len; ///< Needed for the item distribution.

// Forward CButton if it isn't known, yet:
#ifndef ATANKS_BUTTON_H_INCLUDED
// class CButton;
#  include "button.h"
#endif // ATANKS_BUTTON_H_INCLUDED

// Forward CMenu if it isn't known, yet:
#ifndef MENU_CLASS_DECLARES
class CMenu;
#endif // MENU_CLASS_DECLARES


/** @class COptionItemBase
 * @brief Common base class for all option items.
 *
 * This base class holds all common values of the option item represented and
 * is used to order option items as a doubly linked list.
 *
 * The class has several public methods to change the inner state, like moving
 * or resizing the display area, changing the title, switching the text content
 * array and so on.
 **/
class COptionItemBase {
public:
	/* ------------------------------
	 * --- Public ctors and dtors ---
	 * ------------------------------
	 */

	/// Create an option item.
	explicit COptionItemBase(
		EEntryType   type_,
		char const*  title_,
		int32_t      title_idx_,
		char const** text_,
		int32_t      color_,
		ETextClass   class_,
		char const*  format_,
		int32_t      top_,
		int32_t      left_,
		int32_t      width_,
		int32_t      height_,
		int32_t      padding_,
		int32_t      show_size_
	);

	/// Destroy an option item.
	virtual ~COptionItemBase();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void            clear_display( bool update_full );                         ///< Clear the display area.
	void            cursor_flip();                                             ///< Blink the text cursor.
	void            get_dimension( int32_t& tgt_width, int32_t& tgt_height );   ///< read the display size.
	COptionItemBase* get_next();                                                 ///< Next list entry.
	COptionItemBase* get_prev();                                                 ///< Previous list entry.
	uint32_t        get_text_class();                                            ///< read the text class.
	EEntryType      get_type();                                                 ///< read the entry type.
	void            insert_after( COptionItemBase* new_prev );                  ///< Insert an entry after this.
	void            insert_before( COptionItemBase* new_next );                 ///< Insert an entry before this.
	bool            is_click_in( int32_t x, int32_t y, int32_t& ret );         ///< Hit-test a click.
	void            move( int32_t new_left, int32_t new_top, bool do_update ); ///< Move the display area.
	bool            needs_text();                                              ///< Require text content.
	void            remove();                                                  ///< Unlink from the list.
	void            resize( int32_t new_width, int32_t new_height );           ///< Resize the display area.
	void            select();                                                  ///< Select the entry.
	void            set_padding( int32_t new_padding );                         ///< Set the title padding.
	void            set_title( char const* new_title );                         ///< Set the title.
	void            set_text_class( ETextClass new_class );                      ///< Set the text class.
	void            set_texts( char const** new_texts );                        ///< Set the text array.
	void            unselect();                                                ///< Deselect the entry.

	// Status Getters
	[[nodiscard]] int32_t  get_key_code() const;  ///< read the button key code.
	[[nodiscard]] uint32_t get_title_idx() const; ///< read the title index.
	[[nodiscard]] bool     is_selected() const; ///< Test the selection state.

	// virtuals to be implemented by the deriving template
	/// Handle activation.
	virtual int32_t activate( int32_t val, int32_t x, int32_t y, int32_t k ) = 0;
	/// Test downward navigation.
	virtual bool    canGoDown()                                              = 0;
	/// Test upward navigation.
	virtual bool    canGoUp()                                                = 0;
	/// Render the entry.
	virtual void    display( bool show_full )                                = 0;
	/// Test for exit buttons.
	virtual bool    isExitButton()                                           = 0;


protected:
	/* -------------------------
	 * --- Protected methods ---
	 * -------------------------
	 */
	/// Feed a keypress to a text target.
	void activate_text( char* target, int32_t raw_key );
	/// Flip a toggle target.
	void activate_toggle( bool* target );
	/// Draw the button.
	void display_button();
	/// Draw decorations.
	void display_deco( int32_t show_color = BLACK );
	/// Draw a sub menu entry.
	void display_menu( CMenu* target );
	/// Draw a text target.
	void display_text( char* target );
	/// Draw a constant text target.
	void display_text( char const* target );
	/// Draw an unsigned text target.
	void display_text( uint32_t* target );
	/// Draw a toggle target.
	void display_toggle( bool const* target );

	/// Forward to the const overload.
	void display_toggle( bool* target ) { return display_toggle( static_cast< bool const* >( target ) ); }

	// This one can be static
	/// Open a sub menu.
	static int32_t activate_menu( CMenu* target );

	/// @brief As OT_VALUE might be anything, it is templated on method scale.
	template< typename tgt_t > void display_value( tgt_t* target ) {
		if ( format ) {
			char txt_buf[ 256 ] = { 0x0 };
			snprintf( txt_buf, 255, format, *target );
			text_len = static_cast< int32_t >( strlen( txt_buf ) );
			this->display_text( txt_buf );
		} else if ( texts && texts[ entry_num ] ) {
			text_len = static_cast< int32_t >( strlen( texts[ entry_num ] ) );
			this->display_text( texts[ entry_num ] );
		}
	}

	// Note: The following templates only define a path for the dispatcher
	// when checking for which method to call. If calling path, and thus any
	// option configuration, are invalid, they will print an error message and
	// terminate. These templates are only instantiated by the compiler for
	// syntax and call path checking, and thrown away being unused later.
	// This looks like a waste, but makes the dispatching a lot less complex
	// and more secure.
#define EMERGENCY_OUT                                                                                                \
	fprintf( stderr, "%s:%d [%s] : Illegal target type, template called!\n", __FILE__, __LINE__, __FUNCTION__ ); \
	std::terminate(); ///< Abort on invalid dispatch configuration.

	/// Abort on invalid dispatch configuration.
	template< typename t_t > int32_t activate_menu( t_t* ) { EMERGENCY_OUT }

	/// Abort on invalid dispatch configuration.
	template< typename t_t > void    activate_text( t_t*, int ) { EMERGENCY_OUT }

	/// Abort on invalid dispatch configuration.
	template< typename t_t > void    activate_toggle( t_t* ) { EMERGENCY_OUT }

	/// Abort on invalid dispatch configuration.
	template< typename t_t > void    display_menu( t_t* ) { EMERGENCY_OUT }

	/// Abort on invalid dispatch configuration.
	template< typename t_t > void    display_text( t_t* ) { EMERGENCY_OUT }

	/// Abort on invalid dispatch configuration.
	template< typename t_t > void    display_toggle( t_t* ) { EMERGENCY_OUT }

#undef EMERGENCY_OUT


	/* -------------------------
	 * --- Protected members ---
	 * -------------------------
	 */

	CButton*         button    = nullptr; //!< The button used by ET_BUTTON.
	int32_t         color     = BLACK;   //!< The color to use for the text, mainly useful for OT_TOGGLE.
	bool            cursor_on = false;   //!< selected ET_TEXT elements feature a cursor.
	int32_t         curs_clk  = 0;       //!< Only react on every CURSOR_FLIP_TIME call.
	bool            decorated = false;   //!< Set to true by display_deco() and false by clear_display(true)
	bool            drawn     = false;   //!< Set to true by display methods, and false by clear_display().
	int32_t         entry_num  = 0;       //!< Store the currently displayed text index with OT_VALUE options.
	char const*     format    = nullptr; //!< Format string to use with OT_VALUE
	int32_t         height    = 0;       //!< Height of the display area.
	int32_t         key_code   = 0;       //!< Key Code returned when clicking an ET_BUTTON.
	int32_t         left      = 0;       //!< Left x position of the display area.
	COptionItemBase* next      = nullptr; //!< Next option entry in a doubly linked list.
	int32_t         padding   = 2;       //!< Padding of the title and possible buttons to the display area.
	COptionItemBase* prev      = nullptr; //!< Previous option entry in a doubly linked list.
	bool            read_only = true;    //!< Whether ET_TEXT reacts on clicks and keys or not.
	bool            selected  = false;   //!< Whether this entry is selected or not.
	bool            show_menu = true;    //!< If set to true, the sub menu indicator is shown.
	int32_t         show_size = 0;       //!< Size of the color box ET_COLOR displays the current color in
	int32_t         text_len   = 0;       //!< Store the current size of OT_TEXT content.
	ETextClass      text_class = TC_NONE; //!< Noted for language switch.
	bool            text_only  = false;   //!< If set to true, display_text() draws no box.
	char const**    texts     = nullptr; //!< Text array to use for OT_VALUE
	char const*     title     = nullptr; //!< Title to display, mandatory
	int32_t         title_idx  = 0;       //!< Noted for language switch. -1 means the title is fixed.
	int32_t         title_len  = 0;       //!< Length of the title in pixels
	int32_t         top       = 0;       //!< Top y position of the display area.
	EEntryType      type      = ET_NONE; //!< Type of the option, mandatory
	int32_t         width     = 0;       //!< Width of the display area.
};

#endif // ATANKS_OPTIONITEMBASE_H_INCLUDED
