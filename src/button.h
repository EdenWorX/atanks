#ifndef ATANKS_SRC_BUTTON_H_INCLUDED
#define ATANKS_SRC_BUTTON_H_INCLUDED 1

#include "box.h"
#include "main.h"

/** @class BUTTON
 * @brief Clickable menu button.
 **/
class BUTTON {
public:
	/* --------------------
	 * --- constructors ---
	 * --------------------
	 */

	/// Minimum ctor without text.
	explicit BUTTON( int32_t left_, int32_t top_, BITMAP* bmp_, BITMAP* hover_, BITMAP* depressed_ );

	/// Ctor for using a bitmap.
	BUTTON( char const* text_, bool text_only_, int32_t left_, int32_t top_, BITMAP* bmp_, BITMAP* hover_, BITMAP* depressed_
	);

	/// Ctor for drawing a manual box.
	BUTTON( char const* text_, bool text_only_, int32_t left_, int32_t top_, int32_t width_, int32_t height_ );


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	/// Render the button.
	void draw();
	/// Read the button geometry.
	void getLocation( int32_t& x, int32_t& y, int32_t& w, int32_t& h ) const;
	/// Test mouse hover.
	bool isMouseOver() const;
	/// Test button press.
	bool isPressed() const;
	/// Replace the button text.
	void setText( char const* text_ );

private:
	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	BITMAP*     bmp       = nullptr;
	BITMAP*     depressed = nullptr;
	BITMAP*     hover     = nullptr;
	BOX         location;               //!< is {0, 0, 0, 0} by default
	char const* text      = nullptr;
	bool        text_only = false;      //!< If set to true, only the title is displayed.
	int32_t     x1, y1, x2, y2, x3, y3; //!< Shortcuts, as those stay fixed.
};

#endif // ATANKS_SRC_BUTTON_H_INCLUDED
