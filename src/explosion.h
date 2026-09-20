#ifndef EXPLOSION_DEFINE
#define EXPLOSION_DEFINE

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


#include "main.h"
#include "physobj.h"
#include "weapon.h"

/** @class CExplosion
 * @brief Detonation effect.
 **/
class CExplosion final : public CPhysicalObject {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Detonate a standard explosion.
	explicit CExplosion( CPlayer* player_, double x_, double y_, double xv_, double yv_, int32_t type, bool is_weapon );
	/// Detonate a beam explosion with custom damage.
	CExplosion( CPlayer* player_, double x_, double y_, double xv_, double yv_, int32_t type, double damage_, bool is_weapon );
	/// Destroy an explosion.
	~CExplosion() final;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void   applyPhysics() final; ///< Advance physics.
	void   draw() final;         ///< Render the explosion.
	void   explode();            ///< Detonate the explosion.

	/// Return the object class.
	EClass get_class() final { return CLASS_EXPLOSION; }


private:
	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void do_clear();
	void do_throw();
	void draw_fracture( int32_t x, int32_t y, int32_t frac_angle, int32_t width, int32_t segment_length, int32_t max_recurse );


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	bool    apply_damage = true;
	int32_t cur_frame     = 1;
	int32_t damage       = 0;
	int32_t etime        = 0;
	int32_t ex_clock      = 0;
	bool    has_cleared   = false;
	int32_t has_debris    = 0;
	bool    has_slid      = false;
	bool    has_thrown    = false;
	double  impact_xv    = 0.;
	double  impact_yv    = 0.;
	int32_t max_debris    = 0;
	int32_t max_frame     = 0;
	bool    peaked       = false;
	int32_t radius       = 10;
};

// Global helpers:
void   draw_Napalm_Blob( CVirtualObject* blob, double x, double y, int32_t radius, int32_t frame );
double get_hit_damage( CTank* tank, EWeaponType type, double hit_x, double hit_y );

#endif
