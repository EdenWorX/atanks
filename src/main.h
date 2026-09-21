#ifndef ATANKS_MAIN_H_INCLUDED
#define ATANKS_MAIN_H_INCLUDED

/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2003  Thomas Hudson
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
 * */

// Generated build configuration. ATANKS_HAVE_CONFIG_H is only defined for
// CMake builds (see config.h.in); Visual Studio and legacy Makefile builds
// keep passing -DVERSION= and -DDATA_DIR= on the compiler command line.
#ifdef ATANKS_HAVE_CONFIG_H
#  include "config.h"
#endif

#ifndef VERSION
#  define VERSION "0.0.0"
#endif

#ifndef BUFFER_SIZE
#  define BUFFER_SIZE 256
#endif


/// Important: debug.h not only detects which OS/compiler this is,
/// it puts an important fix with allegro on windows in place.
/// Therefore it *must* stay before the block including winalleg.h!
#include "debug.h"


// The windows port does some crazy stuff with int*_t types.
#if defined( ATANKS_IS_WINDOWS )
#  include <cstdint>
#  ifndef ALLEGRO_HAVE_STDINT_H
#    define ALLEGRO_HAVE_STDINT_H 1
#  endif // ALLEGRO_HAVE_STDINT_H
#  if !defined( ATANKS_ATANKS_CPP )
#    define ALLEGRO_NO_MAGIC_MAIN
#  endif // Not called from atanks.cpp
#endif   // Windows build system

#include <allegro.h>

#if defined( ATANKS_IS_WINDOWS )
#  include <winalleg.h>
#endif // windows


// For visual studio some "workarounds" must be put in place
#if defined( ATANKS_IS_MSVC )
#  define PATH_MAX MAX_PATH
// Needed for M_PIl to show up
#  if !defined( _USE_MATH_DEFINES )
#    define _USE_MATH_DEFINES 1
#  endif
#  include <cmath>
#else
#  include <cmath>
#  include <unistd.h>
#endif // Windows versus Linux


// Be sure M_PI and M_PIl are set:
#if !defined( M_PI )
#  define M_PI 3.14159265358979323846f
#endif
#if !defined( M_PIl )
#  define M_PIl 3.14159265358979323846L
#endif


#include "globaltypes.h"
#include "wrap_dirent.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>


#define DONE_IMAGE            11
#define FAST_UP_ARROW_IMAGE   12
#define UP_ARROW_IMAGE        13
#define DOWN_ARROW_IMAGE      14
#define FAST_DOWN_ARROW_IMAGE 15


// Some more workarounds to compile using visual studio:
#if defined( ATANKS_IS_MSVC )
#  define snprintf           atanks_snprintf
#  define strncpy( d, s, c ) strncpy_s( d, c + 1, s, c )
#  define strncat( d, s, c ) strncat_s( d, c + 1, s, c )
#  define access             _access
#  define F_OK               02
#  define R_OK               04
#  define W_OK               02
#  define strcasecmp         _stricmp
#  define strdup             _strdup
#  define unlink             _unlink
#  define mkdir              _mkdir
#endif

// Note: See winclock.h why this is necessary
/// REMOVE_VS12_WORKAROUND
#if ATANKS_HAS_MSVC12_BUG
#  define USLEEP( microseconds_ ) Sleep( microseconds_ / 1000 )
#  define MSLEEP( milliseconds_ ) Sleep( milliseconds_ )
#else
#  define USLEEP( microseconds_ ) std::this_thread::sleep_for( std::chrono::microseconds( microseconds_ ) )
#  define MSLEEP( milliseconds_ ) std::this_thread::sleep_for( std::chrono::milliseconds( milliseconds_ ) )
#endif // VS12 workaround
#define LINUX_SLEEP MSLEEP( 10 )
#define LINUX_REST  MSLEEP( 40 )


using std::cerr;
using std::cout;
using std::endl;
using std::string;


// place to save config and save games
#if defined( ATANKS_IS_WINDOWS )
#  define HOME_DIR "AppData"
#elif defined( ATANKS_IS_LINUX )
#  define HOME_DIR "HOME"
#endif // Windows versus Linux

#ifndef DATA_DIR
#  define DATA_DIR "."
#endif

#define MAX_OVERSHOOT 10000


// The nex few are some math helpers that shorten things dramatically.
#define SIGN( x_arg )   ( ( x_arg ) < 0 ? -1 : 1 )
#define SIGNd( x_arg )  ( ( x_arg ) < 0. ? -1. : 1. )
#define ROUND( x_arg )  static_cast< int32_t >( std::lround( x_arg ) )
#define ROUNDu( x_arg ) static_cast< uint32_t >( std::lround( x_arg ) )

#define FABSDISTANCE2( x1, y1, x2, y2 )                                                     \
	std::sqrt(                                                                          \
		std::pow( static_cast< double >( x2 ) - static_cast< double >( x1 ), 2. )   \
		+ std::pow( static_cast< double >( y2 ) - static_cast< double >( y1 ), 2. ) \
	)
#define FABSDISTANCE3( x1, y1, z1, x2, y2, z2 )                                             \
	std::sqrt(                                                                          \
		std::pow( static_cast< double >( x2 ) - static_cast< double >( x1 ), 2. )   \
		+ std::pow( static_cast< double >( y2 ) - static_cast< double >( y1 ), 2. ) \
		+ std::pow( static_cast< double >( z2 ) - static_cast< double >( z1 ), 2. ) \
	)

#define ABSDISTANCE2( x1, y1, x2, y2 )         ROUNDu( FABSDISTANCE2( x1, y1, x2, y2 ) )
#define ABSDISTANCE3( x1, y1, z1, x2, y2, z2 ) ROUNDu( FABSDISTANCE3( x1, y1, z1, x2, y2, z2 ) )

#define DEG2RAD( degree_ )                     static_cast< double >( degree_ * M_PIl / 180. )
#define RAD2DEG( radian_ )                     static_cast< double >( radian_ * 180. / M_PIl )


// Helpers to turn strings into integer/float without the need to use
// sscanf(), which is considered unsecure these days.
#define SAFE_STOI( target_, source_ )                                                           \
	{                                                                                       \
		try {                                                                           \
			target_ = std::stoi( source_ );                                         \
		} catch ( std::invalid_argument const& ex ) {                                   \
			cerr << "ERROR parsing " << #source_ << ": " << ex.what() << endl;      \
		} catch ( std::out_of_range const& ex ) {                                       \
			cerr << "ERROR " << #source_ << " out of range: " << ex.what() << endl; \
		}                                                                               \
	}                                                                                       \
	do {                                                                                    \
	} while ( 0 )

#define SAFE_STOD( target_, source_ )                                                           \
	{                                                                                       \
		try {                                                                           \
			target_ = std::stod( source_ );                                         \
		} catch ( std::invalid_argument const& ex ) {                                   \
			cerr << "ERROR parsing " << #source_ << ": " << ex.what() << endl;      \
		} catch ( std::out_of_range const& ex ) {                                       \
			cerr << "ERROR " << #source_ << " out of range: " << ex.what() << endl; \
		}                                                                               \
	}                                                                                       \
	do {                                                                                    \
	} while ( 0 )

#define SAFE_STOUL( target_, source_ )                                                          \
	{                                                                                       \
		try {                                                                           \
			target_ = std::stoul( source_ );                                        \
		} catch ( std::invalid_argument const& ex ) {                                   \
			cerr << "ERROR parsing " << #source_ << ": " << ex.what() << endl;      \
		} catch ( std::out_of_range const& ex ) {                                       \
			cerr << "ERROR " << #source_ << " out of range: " << ex.what() << endl; \
		}                                                                               \
	}                                                                                       \
	do {                                                                                    \
	} while ( 0 )

/** @brief show or hide the custom mouse cursor
 *
 * This macro can either hide the custom mouse cursor when @a where is set
 * to nullptr, or draw it on @a where, which then must be a pointer to a
 * BITMAP.
 *
 * This macro should be used to hide the custom mouse cursor before doing any
 * drawing and to place the mouse cursor on @a where once all other drawing is
 * done.
 *
 * If the OS mouse cursor is used, this macro does nothing.
 *
 * @param[in] where BITMAP pointer to draw the custom cursor on or nullptr to
 * hide the custom mouse cursor.
 **/
#define SHOW_MOUSE( where )                                                                                 \
	{                                                                                                   \
		if ( !env.os_mouse ) {                                                                       \
			if ( ( where ) != nullptr )                                                         \
				unscare_mouse();                                                            \
			else                                                                                \
				scare_mouse();                                                              \
			show_mouse( where );                                                                \
			/* Make the neccessary updates */                                                   \
			if ( ( where ) != nullptr ) {                                                       \
				global.make_update( mouse_x, mouse_y, env.misc[ 0 ]->w, env.misc[ 0 ]->h ); \
				global.make_update( lx, ly, env.misc[ 0 ]->w, env.misc[ 0 ]->h );           \
				lx = mouse_x;                                                               \
				ly = mouse_y;                                                               \
			}                                                                                   \
		}                                                                                           \
	}

#define MAXPLAYERS        10
#define MAX_POWER         2000
#define MIN_POWER         100
#define MAX_ROUNDS        10000

#define MENUHEIGHT        40
#define BOXED_TOP         41 // This is the highest non-border pixel in boxed mode
#define BALLISTICS        53
#define BEAMWEAPONS       3
#define WEAPONS           ( BALLISTICS + BEAMWEAPONS )
#define ITEMS             24
#define THINGS            ( WEAPONS + ITEMS )
#define NATURALS          6
#define DIRT_FRAGMENT     ( -1 )
#define MAX_ITEM_DESC_LEN 511
#define MAX_ITEM_NAME_LEN 127

#define MENUBUTTONS       7
#define INGAMEBUTTONS     4
#define SPREAD            10
#define NAME_LEN          24
#if 0 /// REMOVEME: Nowhere used
#  define ADDRESS_LENGTH 16
#endif // 0

#define WAIT_AT_END_OF_ROUND 1 // second (enough with the new live score board)

#define MAX_ITEMS_IN_STOCK   999999
#if 0 /// REMOVEME: Nowhere used
#  define MAX_MONEY_IN_WALLET 1000000000
#endif // 0

// to make the theft bomb base steal easier to change, here
// is a useful define. Maybe, one day, we'll add it to the options?
#define THEFT_AMOUNT 5000

// Use these instead of the (most strict) defaults,
// But only where timing by memory fences do not matter.
#define ATOMIC_READ  std::memory_order_acquire
#define ATOMIC_WRITE std::memory_order_release

/// Color stop of a sGradient strip.
struct sGradient {
	RGB   color; ///< Stop color.
	float point; ///< Stop position (0.0-1.0); -1 terminates the strip.
};

// signals
#define SIG_QUIT_GAME          ( -1 )
#define SIG_OK                 0
#define GLOBAL_COMMAND_QUIT    ( -1 )
#define GLOBAL_COMMAND_MENU    0
#define GLOBAL_COMMAND_OPTIONS 1
#define GLOBAL_COMMAND_PLAYERS 2
#define GLOBAL_COMMAND_CREDITS 3
#define GLOBAL_COMMAND_HELP    4
#define GLOBAL_COMMAND_PLAY    5
#define GLOBAL_COMMAND_DEMO    6
#define GLOBAL_COMMAND_NETWORK 7

/** @enum EClass
 * @brief class definitions of everything from virtual objects up
 *
 * The ordering here determines the order of the drawing.
 **/
enum EClass {
	CLASS_MISSILE = 0,
	CLASS_BEAM,
	CLASS_TANK,
	CLASS_TELEPORT,
	CLASS_DECOR_DIRT,
	CLASS_DECOR_SMOKE,
	CLASS_EXPLOSION,
	CLASS_FLOATTEXT,
	CLASS_COUNT
};


#ifndef HAS_TANK
class CTank; // forwarding if not known
#endif      // HAS_TANK

/// === Global functions used in several compilation units ====
double interpolate( double x1, double x2, double i );
double noise( int x );
double noise_2d( int x, int y );
double perlin_1d_point( double amplitude, double scale, double xo, double lambda, int octaves );
double perlin_2d_point( double amplitude, double scale, double xo, double yo, double lambda, int octaves );
void   quick_change( bool clearerror );

/// === Helpful wrappers and overrides ===
[[maybe_unused]] static inline void circle( BITMAP* bmp, double x, double y, int radius, int color ) {
	circle( bmp, ROUND( x ), ROUND( y ), radius, color );
}

[[maybe_unused]] static inline void circlefill( BITMAP* bmp, double x, double y, double radius, int color ) {
	return circlefill( bmp, ROUND( x ), ROUND( y ), ROUND( radius ), color );
}

[[maybe_unused]] static inline void circlefill( BITMAP* bmp, double x, double y, int32_t radius, int color ) {
	return circlefill( bmp, ROUND( x ), ROUND( y ), radius, color );
}

[[maybe_unused]] static inline void draw_sprite( BITMAP* bmp, BITMAP* sprite, double x, double y ) {
	draw_sprite( bmp, sprite, ROUND( x ), ROUND( y ) );
}

[[maybe_unused]] static inline void ellipsefill( BITMAP* bmp, double x, double y, int32_t rx, int32_t ry, int color ) {
	ellipsefill( bmp, ROUND( x ), ROUND( y ), rx, ry, color );
}

[[maybe_unused]] static inline void ellipsefill( BITMAP* bmp, int32_t x, int32_t y, double rx, double ry, int color ) {
	ellipsefill( bmp, x, y, ROUND( rx ), ROUND( ry ), color );
}

[[maybe_unused]] static inline int getpixel( BITMAP* bmp, double x, double y ) {
	return getpixel( bmp, ROUND( x ), ROUND( y ) );
}

[[maybe_unused]] static inline int getpixel( BITMAP* bmp, int32_t x, double y ) {
	return getpixel( bmp, x, ROUND( y ) );
}

[[maybe_unused]] static inline int getpixel( BITMAP* bmp, double x, int32_t y ) {
	return getpixel( bmp, ROUND( x ), y );
}

[[maybe_unused]] static inline int makecol( double r, double g, double b ) {
	return makecol( ROUND( r ), ROUND( g ), ROUND( b ) );
}

[[maybe_unused]] static inline void rectfill( BITMAP* bmp, double x1, double y_1, double x2, double y2, int color ) {
	rectfill( bmp, ROUND( x1 ), ROUND( y_1 ), ROUND( x2 ), ROUND( y2 ), color );
}

[[maybe_unused]] static inline void
	rotate_scaled_sprite( BITMAP* bmp, BITMAP* sprite, double x, double y, fixed angle, fixed scale ) {
	rotate_scaled_sprite( bmp, sprite, ROUND( x ), ROUND( y ), angle, scale );
}

[[maybe_unused]] static inline void rotate_sprite( BITMAP* bmp, BITMAP* sprite, double x, double y, fixed angle ) {
	rotate_sprite( bmp, sprite, ROUND( x ), ROUND( y ), angle );
}

[[maybe_unused]] static inline void
	triangle( BITMAP* bmp, double x1, double y_1, double x2, double y2, double x3, double y3, int color ) {
	triangle( bmp, ROUND( x1 ), ROUND( y_1 ), ROUND( x2 ), ROUND( y2 ), ROUND( x3 ), ROUND( y3 ), color );
}

#include "externs.h"

#endif // ATANKS_MAIN_H_INCLUDED
