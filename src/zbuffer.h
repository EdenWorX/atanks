#ifndef ATANKS_ZBUFFER_H
#define ATANKS_ZBUFFER_H 1
//
// Created by sed on 01.08.23.
//


#include <cstdint>
#include <vector>

/** @brief ZBuffer
 * Acts a a simple, 1bpp zbuffer.  For each pixel location, the ZBuffer can remember if something is "popping up" at that
 * location.
 */
class ZBuffer {
public:
	// No copies:
	ZBuffer()                             = delete;
	ZBuffer& operator= ( ZBuffer const& ) = delete;

	/// Construct a z-buffer.
	explicit ZBuffer( int32_t w, int32_t h );

	/// Mark a pixel as popped up.
	void set( int32_t x, int32_t y );
	/// Test whether a pixel popped up.
	bool test( int32_t x, int32_t y ) const;

private:
	std::vector< bool > z;
	int32_t             shiftamt = 0;
};


#endif // ATANKS_ZBUFFER_H
