#ifndef MISSILE_DEFINE
#define MISSILE_DEFINE 1

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

// The ages are *seconds* and transformed to frames in the ctor.
#define MAX_JELLY_AGE   1
#define MAX_MISSILE_AGE 15
#define MAX_METEOR_AGE  5

#define SDI_DISTANCE    100
#define TRIGGER_HEIGHT  300

struct sSDI;

/** @enum EMissileType
 * @brief Determines what kind of weapon is shot
 **/
enum EMissileType {
	MT_WEAPON = 0, //!< Normal weapon, nothing special
	MT_ITEM,       //!< Not a weapon but an item
	MT_NATURAL,    //!< Fired by natural disaster, like meteors and dirt balls.
	MT_MIND_SHOT   //!< AI thinking.
};

/** @class CMissile
 * @brief Ballistic projectile.
 **/
class CMissile final : public CPhysicalObject {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Fire a missile.
	explicit CMissile(
		CPlayer*      player_,
		double       xpos,
		double       ypos,
		double       xvel,
		double       yvel,
		int32_t      weapon_type,
		EMissileType missile_type,
		int32_t      ai_level_,
		int32_t      delay_idx_
	);
	/// Destroy a missile.
	~CMissile() final;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void   applyPhysics() final;                                ///< Advance physics.
	void   draw() final;                                        ///< Render the missile.
	void   update_submun( EPhysType p_type, int32_t cnt_down ); ///< Release submunitions.

	/// Return the object class.
	EClass get_class() final { return CLASS_MISSILE; }

	/* Status Getters */
	[[nodiscard]] int32_t bounced() const;   ///< Bounce count.
	[[nodiscard]] int32_t direction() const; ///< Flight direction.


private:
	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void    apply_physics_funky();   // Handle funky projectiles
	void    apply_physics_normal();  // Handle standard physics projectiles
	void    apply_physics_other();   // Handle what is not normal, funky or rolling
	void    apply_physics_rolling(); // Handle rolling projectiles
	sSDI*   build_sdi_list( sSDI* sdi );
	void    check_cluster();                    // Check/Launch weapons with submunition
	bool    check_missile_hit( sSDI* sdi );     // Check whether the missile will hit a certain target
	bool    check_roller( double old_delta_x ); // Check whether a roller triggers
	void    check_sdi();                        // see if missile should be shot down
	void    check_tanks();                      // see whether any tank is hit
	int32_t height_above_ground();
	void    repulse_missile();
	void    trigger();
	void    trigger_test();


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t      ai_level     = 0; // Level of the AI shooting a mind shot
	int32_t      countdown    = -1;
	int32_t      funky_colour = BLACK;
	int32_t      grow_radius   = 0;
	bool         is_growing    = false;
	EMissileType missile_type  = MT_WEAPON;
	double       roll_carry    = 0.; // Carry for frame-rate independent roller steps
	CWeapon*      weap         = nullptr;
};

#endif // MISSILE_DEFINE
