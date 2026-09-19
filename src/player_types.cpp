#include "player_types.h"

EPlayerType &operator+= ( EPlayerType &src, int32_t val ) {
	int32_t cur = static_cast< int32_t >( src ) + val;
	if ( cur > 0 ) {
		cur %= LAST_PLAYER_TYPE;
	}
	if ( cur < 0 ) {
		cur = LAST_PLAYER_TYPE - ( ( -1 * cur ) % LAST_PLAYER_TYPE );
	}
	src = static_cast< EPlayerType >( cur );
	return src;
}

EPlayerType &operator-= ( EPlayerType &src, int32_t val ) {
	return src += -1 * val;
}

EPlayerType &operator++ ( EPlayerType &src ) {
	return src += 1;
}

EPlayerType operator++ ( EPlayerType &src, int32_t ) { // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]
	EPlayerType old_val  = src;
	src                += 1;
	return old_val;
}

EPlayerPrefType &operator+= ( EPlayerPrefType &src, int32_t val ) {
	int32_t cur = static_cast< int32_t >( src ) + val;
	if ( cur > 0 ) {
		cur %= PREF_COUNT;
	}
	if ( cur < 0 ) {
		cur = PREF_COUNT - ( ( -1 * cur ) % PREF_COUNT );
	}
	src = static_cast< EPlayerPrefType >( cur );
	return src;
}

EPlayerPrefType &operator-= ( EPlayerPrefType &src, int32_t val ) {
	return src += -1 * val;
}

EPlayerPrefType &operator++ ( EPlayerPrefType &src ) {
	return src += 1;
}

EPlayerPrefType operator++ ( EPlayerPrefType &src, int32_t ) { // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]
	EPlayerPrefType old_val  = src;
	src                    += 1;
	return old_val;
}

EPlayerStages &operator+= ( EPlayerStages &src, int32_t val ) {
	int32_t cur = static_cast< int32_t >( src ) + val;
	if ( cur > 0 ) {
		cur %= PS_STAGE_COUNT;
	}
	if ( cur < 0 ) {
		cur = PS_STAGE_COUNT - ( ( -1 * cur ) % PS_STAGE_COUNT );
	}
	src = static_cast< EPlayerStages >( cur );
	return src;
}

EPlayerStages &operator-= ( EPlayerStages &src, int32_t val ) {
	return src += -1 * val;
}

EPlayerStages &operator++ ( EPlayerStages &src ) {
	return src += 1;
}

EPlayerStages operator++ ( EPlayerStages &src, int32_t ) { // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]
	EPlayerStages old_val  = src;
	src                   += 1;
	return old_val;
}

ETeamTypes &operator+= ( ETeamTypes &src, int32_t val ) {
	int32_t cur = static_cast< int32_t >( src ) + val;
	if ( cur > 0 ) {
		cur %= TEAM_COUNT;
	}
	if ( cur < 0 ) {
		cur = TEAM_COUNT - ( ( -1 * cur ) % TEAM_COUNT );
	}
	src = static_cast< ETeamTypes >( cur );
	return src;
}

ETeamTypes &operator-= ( ETeamTypes &src, int32_t val ) {
	return src += -1 * val;
}

ETeamTypes &operator++ ( ETeamTypes &src ) {
	return src += 1;
}

ETeamTypes operator++ ( ETeamTypes &src, int32_t ) { // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]
	ETeamTypes old_val  = src;
	src                += 1;
	return old_val;
}

ETankTypes &operator+= ( ETankTypes &src, int32_t val ) {
	int32_t cur = static_cast< int32_t >( src ) + val;
	if ( cur > 0 ) {
		cur %= TT_TANK_COUNT;
	}
	if ( cur < 0 ) {
		cur = TT_TANK_COUNT - ( ( -1 * cur ) % TT_TANK_COUNT );
	}
	src = static_cast< ETankTypes >( cur );
	return src;
}

ETankTypes &operator-= ( ETankTypes &src, int32_t val ) {
	return src += -1 * val;
}

ETankTypes &operator++ ( ETankTypes &src ) {
	return src += 1;
}

ETankTypes operator++ ( ETankTypes &src, int32_t ) { // NOLINT(cert-dcl21-cpp) [clang-tidy is wrong here.]
	ETankTypes old_val  = src;
	src                += 1;
	return old_val;
}
