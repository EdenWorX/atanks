#ifndef ATANKS_BOX_H
#define ATANKS_BOX_H 1
//
// Created by sed on 28.07.23.
//


#include <cstdint>

/** @struct sBox
 * @brief Integer rectangle helper.
 **/
struct sBox {
	int32_t x = 0; ///< Left position.
	int32_t y = 0; ///< Top position.
	int32_t w = 0; ///< Width.
	int32_t h = 0; ///< Height.

	sBox()     = default;
	/// Assign the rectangle.
	sBox( int32_t x_, int32_t y_, int32_t w_, int32_t h_ );
	/// Set the rectangle.
	void set( int32_t x_, int32_t y_, int32_t w_, int32_t h_ );
};

// Make the sBox usage easier:
bool operator== ( const sBox& lhs, const sBox& rhs );
bool operator!= ( const sBox& lhs, const sBox& rhs );

#endif // ATANKS_BOX_H
