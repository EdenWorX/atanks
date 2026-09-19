#ifndef ATANKS_BOX_H
#define ATANKS_BOX_H 1
//
// Created by sed on 28.07.23.
//


#include <cstdint>

/** @struct BOX
 * @brief Integer rectangle helper.
 **/
struct BOX {
	int32_t x = 0; ///< Left position.
	int32_t y = 0; ///< Top position.
	int32_t w = 0; ///< Width.
	int32_t h = 0; ///< Height.

	BOX()     = default;
	/// Assign the rectangle.
	BOX( int32_t x_, int32_t y_, int32_t w_, int32_t h_ );
	/// Set the rectangle.
	void set( int32_t x_, int32_t y_, int32_t w_, int32_t h_ );
};

// Make the BOX usage easier:
bool operator== ( const BOX& lhs, const BOX& rhs );
bool operator!= ( const BOX& lhs, const BOX& rhs );

#endif // ATANKS_BOX_H
