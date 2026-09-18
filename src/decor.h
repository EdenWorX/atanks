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

enum decorTypes { DECOR_SMOKE = 0, DECOR_DIRT };

/** @class DECOR
 * @brief Dirt and smoke debris.
 **/
class DECOR final : public PHYSICAL_OBJECT {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Create smoke or dirt without a bitmap.
	explicit DECOR( double x_, double y_, double xv_, double yv_, int32_t maxRadius, int32_t type_, int32_t delay_ );

	/// Create debris with bitmaps.
	DECOR( double       x_,
	       double       y_,
	       double       xv_,
	       double       yv_,
	       int32_t      maxRadius,
	       int32_t      type_,
	       int32_t      delay_,
	       sDebrisItem* deb_item,
	       sDebrisItem* met_item );


	/// Destroy debris.
	~DECOR() final;


	/* -----------------------------------
	 * --- Public methods              ---
	 * -----------------------------------
	 */

	void   applyPhysics() final;                    ///< Advance physics.
	void   draw() final;                            ///< Render the debris.
	void   force_aging( int32_t frames );           ///< Catch up after FPS drops.

	/// Return the object class.
	eClass getClass() final { return ( DECOR_SMOKE == type ? CLASS_DECOR_SMOKE : CLASS_DECOR_DIRT ); }


private:
	typedef sDebrisItem item_t;


	/* -----------------------------------
	 * --- Private methods             ---
	 * -----------------------------------
	 */

	bool isOnFloor();
	void repulseDecor();
	void updateDirt();


	/* -----------------------------------
	 * --- Private members             ---
	 * -----------------------------------
	 */

	int32_t color        = BLACK;
	double  curWind      = 0.;      //!< shortcut to help physics calculations.
	int32_t delay        = -1;      //!< How long until debris must be on its way.
	int32_t diameter     = 10;      //!< Pre-calculated shortcut for debris items.
	item_t* dirt         = nullptr; //!< The debris item to throw around if not smoke.
	int32_t gotPixels    = 0;       //!< Helper for phased debris creation.
	int32_t grab_x       = 0;       //!< Helper for phased debris creation.
	int32_t grab_y       = 0;       //!< Helper for phased debris creation.
	int32_t grabPerCall  = 0;       //!< Helper for phased debris creation.
	double  maxGravAccel = 1.;      //!< Pre-calculated physics helper.
	double  maxWind      = 8;       //!< env.windstrength cast to double.
	double  maxWindAccel = 1.;      //!< Pre-calculated physics helper.
	item_t* meteor       = nullptr; //!< Metor data if not enough dirt was found, but a meteor stroke.
	int32_t radius       = 5;
	bool    ready        = false;   //!< Whether a debris item is finished or not.
	int32_t type         = DECOR_SMOKE;
};

#endif // DECOR_DEFINE
