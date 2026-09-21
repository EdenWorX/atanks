#ifndef ATANKS_BUTTON_H_INCLUDED
#define ATANKS_BUTTON_H_INCLUDED 1

#include "box.h"
#include "main.h"

/** @class CButton
 * @brief Clickable menu button.
 **/
class CButton {
public:
	/* --------------------
	 * --- constructors ---
	 * --------------------
	 */

	/// Minimum ctor without text.
	explicit CButton( int32_t left_, int32_t top_, BITMAP* bmp_, BITMAP* hover_, BITMAP* depressed_ );

	/// Ctor for using a bitmap.
	CButton( char const* text_, bool text_only_, int32_t left_, int32_t top_, BITMAP* bmp_, BITMAP* hover_, BITMAP* depressed_
	);

	/// Ctor for drawing a manual box.
	CButton( char const* text_, bool text_only_, int32_t left_, int32_t top_, int32_t width_, int32_t height_ );


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	/// Render the button.
	void draw();
	/// read the button geometry.
	void get_location( int32_t& x, int32_t& y, int32_t& w, int32_t& h ) const;
	/// Test mouse hover.
	bool is_mouse_over() const;
	/// Test button press.
	bool is_pressed() const;
	/// Replace the button text.
	void set_text( char const* text_ );

private:
	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	BITMAP*     bmp       = nullptr;
	BITMAP*     depressed = nullptr;
	BITMAP*     hover     = nullptr;
	sBox         location;               //!< is {0, 0, 0, 0} by default
	char const* text      = nullptr;
	bool        text_only = false;      //!< If set to true, only the title is displayed.
	int32_t     x1, y1, x2, y2, x3, y3; //!< Shortcuts, as those stay fixed.
};

#endif // ATANKS_BUTTON_H_INCLUDED
