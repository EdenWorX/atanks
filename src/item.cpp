//
// Created by sed on 28.07.23.
//

#include "item.h"

#include <cstdio>
#include <cstring>

// Safe ctor for CWeapon class
CItem::CItem() = default;

/// @brief Get the current item description
char const* CItem::get_desc() const {
	return desc;
}

/// @brief Get the current item name
char const* CItem::get_name() const {
	return name;
}

/// @brief Safely set a new item description
void CItem::set_desc( char const* desc_ ) {
	if ( strlen( desc_ ) > MAX_ITEM_DESC_LEN ) {
		fprintf( stderr,
		         "Item description for \"%s\" truncated! (%d/%lu characters)\n",
		         name,
		         MAX_ITEM_DESC_LEN,
		         strlen( desc_ ) );
	}
	strncpy( desc, desc_, MAX_ITEM_DESC_LEN );
}

/// @brief Safely set a new item name
void CItem::set_name( char const* name_ ) {
	if ( strlen( name_ ) > MAX_ITEM_NAME_LEN ) {
		fprintf( stderr, "Item name for \"%s\" truncated! (%d/%lu characters)\n", name_, MAX_ITEM_NAME_LEN, strlen( name_ )
		);
	}
	strncpy( name, name_, MAX_ITEM_NAME_LEN );
}
