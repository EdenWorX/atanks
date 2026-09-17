//
// Created by sed on 01.08.23.
//

#include "zbuffer.h"

/** @brief explicit ctor
 *
 *  Construct a ZBuffer object capable of storing "popup" values for a w by h grid.  All cells in the ZBuffer start out lowered.
 *
 *  @param[in] w width of the buffer
 *  @param[in] h height of the buffer
 */
ZBuffer::ZBuffer( int32_t w, int32_t h ) {
	int32_t width = w;
	while ( width ) {
		width >>= 1;
		++shiftamt;
	}
	z.resize( ( h << shiftamt ) | w );
}

/** @brief set a cell in the buffer to be raised
 *
 * Causes a cell in the ZBuffer to become raised.  Follows the same conditions on x and y as the test function does.
 *
 *  @param[in] x x coordinate of the location to raise
 *  @param[in] y y coordinate of the location to raise
 */
void ZBuffer::set( int32_t x, int32_t y ) {
	try {
		z.at( ( y << shiftamt ) | x ) = true;
	} catch ( ... ) {
		/* nothing can be done here... */
	}
}

/** @brief test whether a cell is raised
 *
 *  Returns true if the cell at location (x,y) is raised.
 *  Behaviour is undefined if x does not fall in the range [0,w) or if y does not fall in the range [0,h);
 *  w and h being the parameters to the ctor.
 *
 *  @param[in] x x coordinate of the location to test
 *  @param[in] y y coordinate of the location to test
 */
bool ZBuffer::test( int32_t x, int32_t y ) const {
	try {
		return z.at( ( y << shiftamt ) | x );
	} catch ( ... ) {
		return false;
	}
}
