//
// Created by sed on 28.07.23.
//

#include "weapon.h"

// Safe ctor for CWeapon class
CWeapon::CWeapon() {
	memset( desc, 0, sizeof( char ) * MAX_ITEM_DESC_LEN + 1 );
	memset( name, 0, sizeof( char ) * MAX_ITEM_NAME_LEN + 1 );
}

/// @brief Get the delay or 1 if delay is zero.
int32_t CWeapon::getDelayDiv() const {
	return delay > 0 ? delay : 1;
}

/// @brief Get the current weapon description
char const* CWeapon::getDesc() const {
	return desc;
}

/// @brief Get the current weapon name
char const* CWeapon::getName() const {
	return name;
}

/// @brief Safely set a new weapon description
void CWeapon::setDesc( char const* desc_ ) {
	if ( strlen( desc_ ) > MAX_ITEM_DESC_LEN ) {
		fprintf( stderr,
		         "Weapon description for \"%s\" truncated! (%d/%lu characters)\n",
		         name,
		         MAX_ITEM_DESC_LEN,
		         strlen( desc_ ) );
	}
	strncpy( desc, desc_, MAX_ITEM_DESC_LEN );
}

/// @brief Safely set a new weapon name
void CWeapon::setName( char const* name_ ) {
	if ( strlen( name_ ) > MAX_ITEM_NAME_LEN ) {
		fprintf( stderr, "Weapon name for \"%s\" truncated! (%d/%lu characters)\n", name_, MAX_ITEM_NAME_LEN, strlen( name_ )
		);
	}
	strncpy( name, name_, MAX_ITEM_NAME_LEN );
}
