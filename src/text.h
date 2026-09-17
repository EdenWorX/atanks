#ifndef ATANKS_SRC_TEXT_H_INCLUDED
#define ATANKS_SRC_TEXT_H_INCLUDED 1

/* This file contains functions for reading text from files and
 * storing it in the game. The entire text file will be kept in memory.
 * Each text object will have the ability to return a line,
 * track its position in the text or return a random line.
 * This should allow for better multi-language handling, faster
 * access to tank speech and remove the need to rely on the lineseq
 * class.
 * -- Jesse
 */

#include "box.h"

#include <cstdint>

#define MAX_LINE_LENGTH   512
#define MAX_LINES_IN_FILE 1024

/// @brief alignment of texts
enum alignType { CENTRE = 0, LEFT, RIGHT };

class TEXTBLOCK {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	TEXTBLOCK() = default;
	explicit TEXTBLOCK( char const* filename );
	~TEXTBLOCK();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	bool Load_File( char const* filename ); // load lines from a file
	void Render_Lines( int32_t scrollOffset, int32_t spacing, int32_t top,
	                   int32_t bottom ); // Render to global.canvas

	/* Text Getters */
	[[nodiscard]] char const* Get_Line( int32_t index ) const; // return a specific line
	[[nodiscard]] char const* Get_Random_Line() const;         // give us a random line
	[[nodiscard]] int32_t     Lines() const;                   // Return number of total lines


private:
	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void destroy();


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t total_lines   = 0;
	char**  complete_text = nullptr;
};

// Functions we can use anywhere

// This function returns a string with
// comma characters between every three digits.
// You *MUST* *NOT* free the returned string.
char const* Add_Comma( int32_t number );

void        draw_text_in_box( BOX* region, char const* text, bool with_box );

// hack the newline off a string
void Trim_Newline( char* line );

#endif // ATANKS_SRC_TEXT_H_INCLUDED
