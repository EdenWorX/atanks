#ifndef ATANKS_GFXDATA_H_INCLUDED
#define ATANKS_GFXDATA_H_INCLUDED 1


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

#include "bitmap.h"

#include <cstdint>

#define ALL_SKIES        16
#define ALL_LANDS        16

#define STUFF_BAR_WIDTH  400
#define STUFF_BAR_HEIGHT 35

#define EXPLODEFRAMES    18
#define DISPERSEFRAMES   10
#define EXPLOSIONFRAMES  ( EXPLODEFRAMES + DISPERSEFRAMES )

/// @brief Consolidate global gfx data in a struct to have RAII in effect.
struct sGfxData {
	/// Construct graphics data.
	explicit sGfxData();
	/// Destroy graphics data.
	~sGfxData();

	/// Release generated graphics.
	void    destroy();
	/// Generate graphics.
	void    first_init();

	BITMAP* sky_gradient_strips[ ALL_SKIES ]{ nullptr };  ///< Sky gradients.
	BITMAP* land_gradient_strips[ ALL_LANDS ]{ nullptr }; ///< Land gradients.
	BITMAP* stuff_bar_gradient_strip{ nullptr };          ///< Shop bar sGradient.
	BITMAP* topbar_gradient_strip{ nullptr };             ///< Top bar sGradient.
	BITMAP* explosion_gradient_strip{ nullptr };          ///< Explosion sGradient.
	BITMAP* stuff_bar[ 2 ]{};                             ///< Shop bars.
	BITMAP* stuff_icon_base{ nullptr };                   ///< Shop icon base.
	BITMAP* topbar{ nullptr };                            ///< Top bar.
	BITMAP* explosions[ EXPLOSIONFRAMES ]{ nullptr };     ///< Explosion frames.
	BITMAP* flameFront[ EXPLOSIONFRAMES ]{ nullptr };     ///< Flame front frames.

private:
	bool initDone = false;
};

// === Helper Functions ===
// ========================
BITMAP* create_gradient_strip( sGradient const* grad, int32_t len );
int32_t gradientColorPoint( sGradient const* grad, double len, double line );


#endif // ATANKS_GFXDATA_H_INCLUDED
