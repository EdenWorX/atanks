#ifndef TELEPORT_DEFINE
#define TELEPORT_DEFINE 1

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

/** @class CTeleport
 * @brief Teleport transit effect.
 **/
class CTeleport final : public CVirtualObject {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */
	/// Create the source end.
	explicit CTeleport(
		CVirtualObject* target_obj,
		int32_t         destination_x,
		int32_t         destination_y,
		int32_t         obj_radius,
		int32_t         duration,
		int32_t         type
	);

	/// Delegate with a rounded radius.
	CTeleport(
		CVirtualObject* target_obj,
		int32_t         destination_x,
		int32_t         destination_y,
		double          obj_radius,
		int32_t         duration,
		int32_t         type
	)
		: CTeleport( target_obj, destination_x, destination_y, ROUND( obj_radius ), duration, type ) {}

	/// Delegate with rounded coordinates.
	CTeleport( CVirtualObject* target_obj, double destination_x, double destination_y, double obj_radius, int32_t duration, int32_t type )
		: CTeleport( target_obj, ROUND( destination_x ), ROUND( destination_y ), ROUND( obj_radius ), duration, type ) {}

	/// Destroy a teleport.
	~CTeleport() final;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void   applyPhysics() final; ///< Advance physics.
	void   draw() final;         ///< Render the teleport.

	/// Return the object class.
	EClass get_class() final { return CLASS_TELEPORT; }


private:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	// Target constructor
	CTeleport( CTeleport* remote_end, int32_t dest_x, int32_t dest_y );

	// Non-copyable: shallow copies would corrupt the remote-end destroy protocol in the destructor.
	CTeleport( CTeleport const& )            = delete;
	CTeleport& operator=( CTeleport const& ) = delete;


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t         clock      = 0;
	CVirtualObject* object     = nullptr;
	int32_t         radius     = 0;
	CTeleport*       remote     = nullptr;
	int32_t         start_clock = 0;
};

#endif // TELEPORT_DEFINE
