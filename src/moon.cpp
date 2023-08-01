//
// Created by sed on 01.08.23.
//

#include "moon.h"

#include "levelcreator.h"
#include "main.h"
#include "random.h"
#include "zbuffer.h"


/** @brief struct moon
 *
 * A simple data structure to store the parameters of a moon for easy passing.
 *
 **/
struct moon {
	BITMAP* bitmap;
	int32_t col1;
	int32_t col2;
	double  lambda;
	int32_t octaves;
	int32_t radius;
	double  smoothness;
	int32_t x;
	double  xoffset;
	int32_t y;
	double  yoffset;

	// Simple ctor:
	explicit moon( int32_t scrnw, int32_t scrnh )
		: col1( makecol( get_rand() % 255, get_rand() % 255, get_rand() % 255 ) )
		, col2( makecol( get_rand() % 255, get_rand() % 255, get_rand() % 255 ) )
		, lambda( ( ( get_rand() % 60 ) + 30 ) / 100. )
		, octaves( ( get_rand() % 4 ) + 6 )
		, radius( ROUND( central_rand( scrnw / 8. ) + .5 ) )
		, smoothness( ( get_rand() % 20 ) + 3 )
		, x( get_rand() % scrnw )
		, xoffset( get_rand() )
		, y( get_rand() % scrnh )
		, yoffset( get_rand() ) {
		bitmap = create_bitmap( radius * 2, radius * 2 );
	}

	// Simple dtor to get rid of the temp bitmap
	~moon() {
		if ( bitmap ) {
			destroy_bitmap( bitmap );
		}
	}
};

/*****************************************************************************
Static function prototypes that need either moon or ZBuffer
*****************************************************************************/
static void paint_moonpix( int32_t x, int32_t y, moon const& mn, double xval, double yval, double blend );

/** @brief clamp an int into a range
 * Clamps an integer value @arg v into a range from 0 to @arg u.
 *
 * @param[in] v value to clamp
 * @param[in] u upper clamp value
 * @return The clamped value.
 **/
static inline int32_t clamped_int( int32_t v, int32_t u ) {
	return ( v < 0 ? 0 : ( v > u ? u : v ) );
}

/** @brief coverage of a pixel
 *
 * Compute the percent coverage of a pixel by a sphere given the pixel's distance from the centre and the sphere's radius.
 * @param[in] distance distance to the center
 * @param[in] radius radius of the sphere
 * @return coverage in the range of [0.;1.]
 **/
static inline double coverage( double distance, double radius ) {
	if ( distance > radius ) {
		return 1 - ( distance - radius );
	}
	return 1.;
}

/** @brief draw a single moon
 *
 * Renders a single moon onto a bitmap.  Assumes transparent drawing is enabled.
 * Obeys the given bounding box, which may be smaller than the moon itself.
 * Uses the darkside parameter to decide which side of the moon should be dark.
 * Obeys and updates the z-buffer.
 *
 * @todo The current implementation of this function is begging for some simplifications.
 *
 * @params[in] The levelCreator in action, so we can ask whether it has been called to break off
 * @param[out] sky The bitmap to draw on
 * @param mn[in] The moon instance to draw onto the @arg sky
 * @param[in] x0 One corner x coordinate; The lower of x0 and x1 is the start, the other is the end.
 * @param[in] y0 One corner y coordinate; The lower of y0 and y1 is the start, the other is the end.
 * @param[in] x1 One corner x coordinate; The lower of x0 and x1 is the start, the other is the end.
 * @param[in] y1 One corner y coordinate; The lower of y0 and y1 is the start, the other is the end.
 * @param[in] darkside if true paint the left, otherwise the right side of the moon.
 * @param[in,out] zbuffer buffer to record which pixels are "taken".
 **/
static void draw_amoon(
	LevelCreator* lcr,
	BITMAP*       sky,
	moon const&   mn,
	int32_t       x0,
	int32_t       y0,
	int32_t       x1,
	int32_t       y1,
	bool          darkside,
	ZBuffer&      zbuffer
) {
	int32_t startX = std::min( x0, x1 );
	int32_t endX   = std::max( x0, x1 );
	int32_t startY = std::min( y0, y1 );
	int32_t endY   = std::max( y0, y1 );

	clear_to_color( mn.bitmap, BLACK );
	blit( sky, mn.bitmap, startX, startY, 0, 0, mn.radius * 2, mn.radius * 2 );

	for ( int32_t y = startY; ( y < endY ) && lcr->can_work(); ++y ) {
		bool hityet = false;

		for ( int32_t x = startX; ( x < endX ) && lcr->can_work(); ++x ) {
			/* Occupied? */
			if ( zbuffer.test( x, y ) ) {
				continue;
			}

			/* Find distance from this moon */
			int32_t xdist = mn.x - x;
			int32_t ydist = mn.y - y;

			/* Compute some other nice circle values */
			double const radius    = mn.radius;
			double       xval      = static_cast< double >( xdist ) / radius;
			double       yval      = static_cast< double >( ydist ) / radius;
			double       distance2 = ( xdist * xdist ) + ( ydist * ydist );
			double       distance  = std::sqrt( distance2 );

			/* A bound check -> are we in the circle? */
			if ( distance > ( radius + 1 ) ) {
				if ( hityet ) { // If we've already been inside at this y...
					break;  // then skip ahead to the next y
				}
				continue; // Otherwise stay at this y, and skip to the next x
			}

			double xval = xdist / radius;
			double yval = ydist / radius;

			/* Edges use lighter blending */
			double distance = std::sqrt( distance2 );
			double edgeval  = coverage( distance, radius );

			/* Now, should we paint this side of the moon? */
			if ( xval && ( ( xval < 0 ) == darkside ) ) {
				lcr->yield();
				paint_moonpix( x - startX, y - startY, mn, fabs( xval ), yval, edgeval );
			}

			/* Mark this pixel as occupied */
			zbuffer.set( x, y );
			hityet = true;
		}
	}

	// Put the moon on the sky bitmap:
	global.lockLand();
	drawing_mode( DRAW_MODE_TRANS, nullptr, 0, 0 );
	blit( mn.bitmap, sky, 0, 0, startX, startY, mn.radius * 2, mn.radius * 2 );
	drawing_mode( global.current_drawing_mode, nullptr, 0, 0 );
	global.unlockLand();
}

/** @brief paint a moon pixel into a moons bitmap
 *
 * Paint a pixel onto the screen for a particular part of a moon.
 *
 * @param mn[out] The moon instance into whichs bitmap to paint
 * @param[in] x x coordanate of the pixel
 * @param[in] y y coordinate of the pixel
 * @param[in] xval x coordinate percentage along the moon, must fall within [0;1]
 * @param[in] yval y coordinate percentage along the moon, must fall within [0;1]
 * @param[in] blend Controls how much "paint" is used. Must be in the range [0,1].
 **/
static void paint_moonpix( int32_t x, int32_t y, Moon const& mn, double xval, double yval, double blend ) {
	auto const   thetax = RAD2DEG( asin( xval ) );
	auto const   thetay = RAD2DEG( acos( yval ) );
	double const offset =
		( perlin2DPoint( 1., mn.smoothness, mn.xoffset + mn.x + thetax, mn.yoffset + mn.y + thetay, mn.lambda, mn.octaves )
	          + 1. )
		/ 2.;
	double const percVal =
		( perlin2DPoint(
			  1.0,
			  mn.smoothness,
			  mn.xoffset + mn.x * 1000 + thetax,
			  mn.yoffset + mn.y * 1000 + thetay,
			  mn.lambda,
			  mn.octaves
		  )
	          + 1 )
		/ 2;

	set_add_blender( 0, 0, 0, ROUND( blend * xval * percVal * offset * 255 ) );
	drawing_mode( DRAW_MODE_TRANS, nullptr, 0, 0 );
	putpixel( mn.bitmap, x, y, mn.col1 );
	set_add_blender( 0, 0, 0, ROUND( blend * xval * ( 1. - percVal ) * offset * 255 ) );
	putpixel( mn.bitmap, x, y, mn.col2 );
	drawing_mode( global.current_drawing_mode, nullptr, 0, 0 );
}

/** @brief draw_moons on a sky
 * Renders a set of moons over a given bitmap.
 * The bitmap to draw of and the appropriate dimensions must be given.
 *
 * @params[in] The levelCreator in action, so we can ask whether it has been called to break off
 * @param[out] sky The bitmap to draw on
 * @param[in] width The width of the area to draw moons in
 * @param[in] height The height of the area to draw moons in
 **/
void draw_moons( LevelCreator* lcr, BITMAP* sky, int32_t width, int32_t height ) {
	bool const darkside = get_rand() > ( RAND_MAX / 2 + 1 );
	ZBuffer    zbuffer( width, height );

	for ( auto numMoons = ROUND( central_rand( 14.0 ) ); numMoons; --numMoons ) {
		/* Make up a moon */
		moon const mn( width, height );

		/* Where is it? */
		int32_t x0 = clamped_int( mn.x - mn.radius, width );
		int32_t y0 = clamped_int( mn.y - mn.radius, height );
		int32_t x1 = clamped_int( mn.x + mn.radius, width );
		int32_t y1 = clamped_int( mn.y + mn.radius, height );

		/* Draw it */
		draw_amoon( lcr, sky, mn, x0, y0, x1, y1, darkside, zbuffer );
	}
}
