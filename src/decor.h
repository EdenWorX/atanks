#ifndef DECOR_DEFINE
#define DECOR_DEFINE 1

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


#include "debris_pool.h"
#include "physobj.h"

enum EDecorTypes { DECOR_SMOKE = 0, DECOR_DIRT };

/** @class CDecor
 * @brief Dirt and smoke debris.
 **/
class CDecor final : public CPhysicalObject {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Create smoke or dirt without a bitmap.
	explicit CDecor( double x_, double y_, double xv_, double yv_, int32_t max_radius, int32_t type_, int32_t delay_ );

	/// Create debris with bitmaps.
	CDecor( double       x_,
	       double       y_,
	       double       xv_,
	       double       yv_,
	       int32_t      max_radius,
	       int32_t      type_,
	       int32_t      delay_,
	       sDebrisItem* deb_item,
	       sDebrisItem* met_item );


	/// Destroy debris.
	~CDecor() final;


	/* -----------------------------------
	 * --- Public methods              ---
	 * -----------------------------------
	 */

	void   applyPhysics() final;          ///< Advance physics.
	void   draw() final;                  ///< Render the debris.
	void   force_aging( int32_t frames ); ///< Catch up after FPS drops.

	/// Return the object class.
	EClass get_class() final { return ( DECOR_SMOKE == type ? CLASS_DECOR_SMOKE : CLASS_DECOR_DIRT ); }


private:
	typedef sDebrisItem item_t;


	/* -----------------------------------
	 * --- Private methods             ---
	 * -----------------------------------
	 */

	bool is_on_floor();
	void repulse_decor();
	void update_dirt();


	/* -----------------------------------
	 * --- Private members             ---
	 * -----------------------------------
	 */

	int32_t color        = BLACK;
	double  cur_wind      = 0.;      //!< shortcut to help physics calculations.
	int32_t delay        = -1;      //!< How long until debris must be on its way.
	int32_t diameter     = 10;      //!< Pre-calculated shortcut for debris items.
	item_t* dirt         = nullptr; //!< The debris item to throw around if not smoke.
	int32_t got_pixels    = 0;       //!< Helper for phased debris creation.
	int32_t grab_x       = 0;       //!< Helper for phased debris creation.
	int32_t grab_y       = 0;       //!< Helper for phased debris creation.
	int32_t grab_per_call  = 0;       //!< Helper for phased debris creation.
	double  max_grav_accel = 1.;      //!< Pre-calculated physics helper.
	double  max_wind      = 8;       //!< env.wind_strength cast to double.
	double  max_wind_accel = 1.;      //!< Pre-calculated physics helper.
	item_t* meteor       = nullptr; //!< Metor data if not enough dirt was found, but a meteor stroke.
	int32_t radius       = 5;
	bool    ready        = false;   //!< Whether a debris item is finished or not.
	int32_t type         = DECOR_SMOKE;
};

#endif // DECOR_DEFINE
