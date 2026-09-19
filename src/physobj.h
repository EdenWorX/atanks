#ifndef PHYSOBJ_DEFINE
#define PHYSOBJ_DEFINE 1

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

#include "globaltypes.h"
#include "virtobj.h"

// Switch sides (real angle!)
#define FLIP_ANGLE( angle_ ) ( 180 + ( 180 - ( angle_ ) ) )

// Get the direct angle without any checks.
// atan2() returns an angle with 180° up, 90° right and -90° left.
// But we need it from 90° right to 270° left counter-clockwise.
#define GET_ANGLE( x, y )                                      \
	( []( double a, double b ) -> int32_t {                \
		double result = RAD2DEG( std::atan2( a, b ) ); \
		if ( result < 0 ) result += 360.;              \
		return ROUND( result );                        \
	}( static_cast< double >( x ), static_cast< double >( y ) ) )


// Get the angle brought into the 90-270 degree range.
// To be usable more widely, this macro allows an additional argument m, which is the angle modifier
// (errors made by the AI and such things)
#define GET_SAFE_ANGLE( x, y, m )                                  \
	( []( double a, double b, double c ) -> int32_t {          \
		double result = RAD2DEG( std::atan2( a, b ) ) + c; \
		if ( result < 0. ) result += 360.;                 \
		if ( result < 90. )                                \
			result = 90.;                              \
		else if ( result > 270. )                          \
			result = 270.;                             \
		return ROUND( result );                            \
	}( static_cast< double >( x ), static_cast< double >( y ), static_cast< double >( m ) ) )

// Re-calculate angle_ into a value displayable on the top bar:
#define GET_DISP_ANGLE( angle_ ) ( 180 - ( (angle_)-90 ) )

/** @class CPhysicalObject
 * @brief Ballistic game object with gravity and drag.
 **/
class CPhysicalObject : public CVirtualObject {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */
	/// Create a physical object.
	explicit CPhysicalObject( bool is_weapon );

	// No explicit dtor needed

	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	/// Render the object.
	void inline draw() override { CVirtualObject::draw(); };

	/// Read the current velocity.
	void getVelocity( double &xv_, double &yv_ );

	/* Status Getters */
	/// Test whether this is weapon fire.
	[[nodiscard]] bool isWeapon() const;


	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	bool    allowDirtyWrap = true;  //!< Whether ceiling wrap is allowed into dirt bottom
	double  drag           = 0.;    ///< Air drag.
	bool    hitSomething   = false; ///< Collision occurred.
	int32_t weapType       = 0;     ///< Weapon type index.

protected:
	/* -------------------------
	 * --- Protected methods ---
	 * -------------------------
	 */

	/// Advance the physics simulation.
	void applyPhysics() override;
	/// Reset the physics state.
	void initialise() override;


	/* -------------------------
	 * --- Protected members ---
	 * -------------------------
	 */

	int32_t bounces      = 0;     //!< Bounces off walls, floor and ceiling
	bool    isWeaponFire = true;  ///< Fired as a weapon.
	bool    lacerated    = false; //!< Set to true if the velocity check fails.
	double  mass         = 0.;    ///< Mass.
	double  maxVel       = 0.;    //!< maximum Velocity
	double  mindDelay    = 0.;    //!< for mind shots to travel through dirt if delayed
	double  mindPassed   = 0.;    //!< Counts the amount of dirt a delayed shot already passed through
	bool    noimpact     = false; ///< Skip impact handling.
	int32_t spin         = 0;     ///< Spin.
};

/// global helper methods:
bool checkPixelsBetweenTwoPoints( double *startX, double *startY, double endX, double endY, double can_delay, double *has_delayed );
void getDirtBounceReact( double x, double y, double xv, double yv, double &rxv, double &ryv );

#endif // PHYSOBJ_DEFINE
