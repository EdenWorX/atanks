#include "main.h"


// Safe ctor for WEAPON class
ITEM::ITEM() {
	memset( vals, 0, sizeof( double ) * MAX_ITEMVALS );
	memset( desc, 0, sizeof( char ) * MAX_ITEM_DESC_LEN + 1 );
	memset( name, 0, sizeof( char ) * MAX_ITEM_NAME_LEN + 1 );
}

/// @brief Get the current item description
char const* ITEM::getDesc() const {
	return desc;
}

/// @brief Get the current item name
char const* ITEM::getName() const {
	return name;
}

/// @brief Safely set a new item description
void ITEM::setDesc( char const* desc_ ) {
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
void ITEM::setName( char const* name_ ) {
	if ( strlen( name_ ) > MAX_ITEM_NAME_LEN ) {
		fprintf( stderr, "Item name for \"%s\" truncated! (%d/%lu characters)\n", name_, MAX_ITEM_NAME_LEN, strlen( name_ )
		);
	}
	strncpy( name, name_, MAX_ITEM_NAME_LEN );
}
