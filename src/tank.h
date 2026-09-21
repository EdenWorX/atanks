#ifndef TANK_DEFINE
#define TANK_DEFINE 1

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


#include "floattext.h"
#include "physobj.h"
#include "weapon.h"

#define DIR_RIGHT      1
#define DIR_LEFT       ( -1 )

#define VIOLENT_CHANCE 6


class CPlayer;
class CExplosion;

/** @class CTank
 * @brief Player tank avatar.
 **/
class CTank final : public CPhysicalObject {

public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Construct a tank.
	explicit CTank();
	/// Destroy a tank.
	~CTank() final;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void    activate();                                                                                      ///< Show turn labels.
	void    activate_current_selection();                                                                      ///< Fire selected weapon.
	void    add_damage( CPlayer* damage_from, double damage_ );                                                 ///< Record inbound damage.
	void    apply_damage();                                                                                   ///< Apply recorded damage.
	void    applyPhysics() final;                                                                            ///< Advance physics.
	void    check_weapon();                                                                                  ///< Validate weapon index.
	void    deactivate();                                                                                    ///< Hide turn labels.
	void    draw() final;                                                                                    ///< Render the tank.
	void    explode( bool allow_vengeance );                                                                 ///< Destroy the tank.
	int32_t get_bottom();                                                                                     ///< Lowest tank pixel.
	void    get_guntop( int32_t angle_, double& top_x, double& top_y );                                       ///< Muzzle position.
	int32_t how_buried( int32_t* left, int32_t* right );                                                      ///< Measure dirt burial.
	bool    is_flying();                                                                                      ///< Test airborne state.
	bool    is_in_box( int32_t x1, int32_t y1, int32_t x2, int32_t y2 );                                       ///< Test box overlap.
	bool    is_in_box( double x1, double y1, double x2, double y2 );                                           ///< Rounded box overlap.
	bool    is_in_ellipse( double ex, double ey, double rx, double ry, double& in_rate_x, double& in_rate_y ); ///< Test ellipse overlap.
	bool    move_tank( int32_t direction );                                                                  ///< Drive the tank.
	void    new_round( int32_t pos_x, int32_t pos_y );                                                       ///< Spawn for a new round.
	void    reactivate_shield();                                                                             ///< Reload the shield.
	void    repair();                                                                                        ///< Field-repair the tank.
	bool    repulse( double xpos, double ypos, double* xa, double* ya, EPhysType phys_type );                ///< Repulsor pushback.
	void    reset_flash_damage();                                                                              ///< Flush flash damage.
	bool    shoot_clearance( int32_t target_angle, double minimum_clearance, bool& crashed );                   ///< Check shot clearance.
	void    sim_activate_current_selection();                                                                   ///< Fire simultaneously.

	/// Return the object class.
	EClass  get_class() final { return CLASS_TANK; }

	/* Status Getters */
	[[nodiscard]] double  get_diameter() const;          ///< Tank diameter.
	[[nodiscard]] int32_t get_max_life() const;           ///< Maximum life.
	[[nodiscard]] bool    has_repulsor_activated() const; ///< Repulsor shield active.


	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	int32_t   a                 = 90;      ///< Aim angle.
	int32_t   cw                = SML_MIS; ///< Current weapon index.
	int32_t   fire_another_shot = 0;       ///< Pending extra shots.
	CFloatText health_text;                  ///< Health label.
	int32_t   l = 100;                     ///< Life.
	CFloatText name_text;                    ///< Name label.
	int32_t   p  = MAX_POWER / 2;          ///< Shot power.
	int32_t   sh = 0;                      ///< Shield strength.
	CFloatText shield_text;                  ///< Shield label.
	int32_t   sht = 0;                     ///< Shield type.

private:
	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void set_bitmap();
	void set_text_positions( bool renew_colour );
	bool tank_on_tank(); // is this tank on top of another?


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	CPlayer*   credit_to = nullptr;
	double    damage   = 0.;
	CSpinLock damage_lock;
	int32_t   delay_fall       = ROUND( env.landslide_delay * 100 * env.frame_count_mod ); //!< time the tank will hover
	int32_t   flash_damage      = 0;
	bool      is_teleported     = false; //!< Set to true if a teleport occurs to award falling damage.
	int32_t   max_life          = 100;   //!< amount awarded at beginning of round
	bool      new_damager       = false;
	int32_t   para             = 0;
	int32_t   repair_rate      = 0;
	int32_t   repulsion        = 0;
	int32_t   shld_col_inner   = BLACK;
	int32_t   shld_col_outer   = BLACK;
	double    shld_delta       = 360.; //!< divided by FPS in ctor
	double    shld_phase       = 0.;   //!< Neutral
	int32_t   shld_rad_x       = 0;    //!< Determined by the used bitmap
	int32_t   shld_rad_y       = 0;    //!< Determined by the used bitmap
	int32_t   shld_thickness   = 0;
	double    tank_dia         = 1.; //!< Tank diameter, determined by the used bitmap
	int32_t   tank_off_x       = 0;  //!< Determined by the used bitmap
	int32_t   tank_off_y       = 0;  //!< Determined by the used bitmap
	int32_t   tank_sag         = 0;  //!< Determined by the used bitmap
	int32_t   turr_off_x       = 0;  //!< Determined by the used bitmap
	int32_t   turr_off_y       = 0;  //!< Determined by the used bitmap
	int32_t   use_tankbitmap   = -1;
	int32_t   use_turretbitmap = -1;
};

#define HAS_TANK 1

#endif // TANK_DEFINE
