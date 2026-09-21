#pragma once
#ifndef ATANKS_CLOCK_H_INCLUDED
#  define ATANKS_CLOCK_H_INCLUDED

#  include "debug.h"

#  include <cstdint>

bool    check_time_changed(); // check to see if one second has passed
int32_t game_us_get();
void    game_us_reset();
int32_t menu_ms_get();
void    menu_ms_reset();

/// REMOVE_VS12_WORKAROUND
#  if defined( ATANKS_IS_WINDOWS ) && ( 0 == ATANKS_HAS_MSVC12_BUG )
void win_clock_deinit();
void win_clock_init();
#    define WIN_CLOCK_INIT   win_clock_init();
#    define WIN_CLOCK_REMOVE win_clock_deinit();
#  else
#    define WIN_CLOCK_INIT \
	    {}
#    define WIN_CLOCK_REMOVE \
	    {}
#  endif

#endif // ATANKS_CLOCK_H_INCLUDED
