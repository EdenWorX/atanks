#ifndef VIRTOBJ_DEFINE
#define VIRTOBJ_DEFINE 1

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

#include "box.h"
#include "main.h"
#include "text.h"

/// @enum ePhysType
/// @brief Determine which kind of physics should be used
enum ePhysType {
	PT_NORMAL = 0,  //!< No special processing, just a normal curve shot and impact.
	PT_FUNKY_FLOAT, //!< Funky bomb-lets ignore gravitation.
	PT_DIGGING,     //!< Burrowers and the like dig through dirt in a reverse curve.
	PT_ROLLING,     //!< Roll over the surface.
	PT_DIRTBOUNCE,  //!< Dirt debris bounces off of dirt and all walls.
	PT_SMOKE,       //!< Smoke reacts on nothing but repulsor shields.
	PT_NONE,        //!< Special values for age caused detonation triggering.
};


#ifndef HAS_PLAYER
class PLAYER;
#endif // HAS_PLAYER

/** @class VIRTUAL_OBJECT
 * @brief Root of the game object hierarchy.
 **/
class VIRTUAL_OBJECT {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */
	/// Construct an empty object.
	explicit VIRTUAL_OBJECT() = default;
	/// Destroy the object.
	virtual ~VIRTUAL_OBJECT() = default;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	/* --- non-inline methods --- */
	void         addUpdateArea( int32_t left_, int32_t top_, int32_t width_, int32_t height_ ); ///< Queue a dirty rectangle.
	/// Advance the object state.
	virtual void applyPhysics();
	/// Render the object.
	virtual void draw();
	/// Reset the object state.
	virtual void initialise();
	void         setUpdateArea( int32_t left_, int32_t top_, int32_t width_, int32_t height_ ); ///< Set the dirty rectangle.

	/* variable helpers to also allow double coordinates */
	/// Queue a dirty rectangle; coordinates are rounded.
	void addUpdateArea( double left_, double top_, int32_t width_, int32_t height_ ) {
		addUpdateArea( ROUND( left_ ), ROUND( top_ ), width_, height_ );
	}

	/// Set the dirty rectangle; coordinates are rounded.
	void setUpdateArea( double left_, double top_, int32_t width_, int32_t height_ ) {
		setUpdateArea( ROUND( left_ ), ROUND( top_ ), width_, height_ );
	}

	/// Redraw the object if required.
	void update();

	/* --- inline methods --- */
	/// Flag the object for redraw.
	void requireUpdate() { needsUpdate.store( true, ATOMIC_WRITE ); }

	/* --- pure virtual (abstract) methods --- */
	/// Return the object class.
	virtual eClass getClass() = 0;

	/* ------------------------------
	 * --- templated list getters ---
	 * ------------------------------
	 */

	/// @brief If not nullptr, set @a prev_ to the predecessor of this.
	template< typename obj_T > void getPrev( obj_T** prev_ ) {
		auto* prev_obj = static_cast< obj_T* >( prev );
		if ( prev_ ) {
			*prev_ = prev_obj;
		}
	}

	/// @brief If not nullptr, set @a next_ to the successor of this.
	template< typename obj_T > void getNext( obj_T** next_ ) {
		auto* next_obj = static_cast< obj_T* >( next );
		if ( next_ ) {
			*next_ = next_obj;
		}
	}

	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	bool            destroy = false;   ///< Flagged for deletion.
	VIRTUAL_OBJECT* next    = nullptr; ///< Successor in the class list.
	PLAYER*         player  = nullptr; ///< Owning player.
	VIRTUAL_OBJECT* prev    = nullptr; ///< Predecessor in the class list.
	double          x       = 0.;      ///< Horizontal position.
	double          y       = 0.;      ///< Vertical position.

protected:
	/* -------------------------
	 * --- Protected methods ---
	 * -------------------------
	 */

	/// Read the bitmap, if any.
	[[nodiscard]] BITMAP* getBitmap() const { return bitmap; }

	/// Test whether a bitmap is set.
	[[nodiscard]] bool    hasBitmap() const { return ( bitmap != nullptr ); }

	/// Replace the bitmap.
	void                  setBitmap( BITMAP* bitmap_ );


	/* -------------------------
	 * --- Protected members ---
	 * -------------------------
	 */

	int32_t   age       = 0;        ///< Age in frames.
	alignType align     = LEFT;     ///< Label alignment.
	int32_t   angle     = 0;        ///< Facing angle.
	BOX       dim_cur{};            ///< Current dirty rectangle.
	BOX       dim_old{};            ///< Previous dirty rectangle.
	int32_t   height    = 0;        ///< Bitmap height.
	int32_t   maxAge    = -1;       ///< Lifespan in frames (-1 is forever).
	ePhysType physType = PT_NORMAL; ///< Special physics processing.
	int32_t   width     = 0;        ///< Bitmap width.
	double    xv        = 0.;       ///< Horizontal velocity.
	double    yv        = 0.;       ///< Vertical velocity.

private:
	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	BITMAP* bitmap      = nullptr;
	abool_t needsUpdate = ATOMIC_VAR_INIT( false );
};

/// === Shorten the usage of virtual objects ===
typedef VIRTUAL_OBJECT vobj_t;

#endif // VIRTOBJ_DEFINE
