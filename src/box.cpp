//
// Created by sed on 28.07.23.
//

#include "box.h"

bool operator== ( const sBox& lhs, const sBox& rhs ) {
	return ( &lhs == &rhs ) || ( ( lhs.x == rhs.x ) && ( lhs.y == rhs.y ) && ( lhs.w == rhs.w ) && ( lhs.h == rhs.h ) );
}

bool operator!= ( const sBox& lhs, const sBox& rhs ) {
	return !( lhs == rhs );
}

sBox::sBox( int32_t x_, int32_t y_, int32_t w_, int32_t h_ ) : x( x_ ), y( y_ ), w( w_ ), h( h_ ) {}

void sBox::set( int32_t x_, int32_t y_, int32_t w_, int32_t h_ ) {
	x = x_;
	y = y_;
	w = w_;
	h = h_;
}
