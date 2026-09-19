#ifndef BEAM_DEFINE
#define BEAM_DEFINE 1

/*
atanks - obliterate each other with oversize weapons
Copyright (C) 2003  Thomas Hudson

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "externs.h"
#include "main.h"
#include "physobj.h"
#include "weapon.h"

/** @enum eBeamType
 * @brief Determines what kind of beam is generated
 **/
enum eBeamType {
	BT_WEAPON = 0, //!< Normal weapon, nothing special
	BT_SDI,        //!< Not a weapon but an SDI laser
	BT_NATURAL,    //!< Fired by natural disaster, like lightning.
	BT_MIND_SHOT   //!< AI thinking.
};

/** @struct POINT_t
 * @brief 2D beam path point.
 **/
struct POINT_t {
	int32_t x          = 0; ///< Horizontal coordinate.
	int32_t y          = 0; ///< Vertical coordinate.

	explicit POINT_t() = default;
};

/** @class CBeam
 * @brief Laser weapon.
 **/
class CBeam final : public CPhysicalObject {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Fire an angled beam.
	explicit CBeam( CPlayer* player_, double x_, double y_, int32_t fireAngle, int32_t weaponType, eBeamType beam_type );
	/// Fire a point-to-point beam.
	CBeam( CPlayer* player_, double x_, double y_, double tx, double ty, int32_t weaponType, bool is_burnt_out );
	/// Destroy a beam.
	~CBeam() final;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void   applyPhysics() final;                  ///< Advance physics.
	void   draw() final;                          ///< Render the beam.
	void   getEndPoint( int32_t& x, int32_t& y ); ///< Fetch the beam end point for mind shots.
	void   moveStart( double x_, double y_ );     ///< Move the beam start for the satellite.

	/// Return the object class.
	eClass getClass() final { return CLASS_BEAM; }


private:
	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void createBeamPath();
	void makeLightningPath();
	void traceBeamPath();


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	eBeamType beamType  = BT_WEAPON;
	int32_t   color     = WHITE;
	double    damage    = 0.;
	int32_t   numPoints = 2; // Default for lasers
	POINT_t   points[ 12 ];  // Maximum for lightnings
	int32_t   radius    = 0;
	int32_t   seed      = 0;
	int32_t   tgtLeftX  = 0;
	int32_t   tgtRightX = 0;
	CWeapon*   weap      = nullptr;
};

#endif // BEAM_DEFINE
