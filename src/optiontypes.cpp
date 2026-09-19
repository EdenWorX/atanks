#include "optiontypes.h"

#if defined( ATANKS_DEBUG )

/** @brief get the name of an entry type name
 *
 * @param[in] etype The enum entry to get the name of.
 * @return A sattic C-string with the name or "UNIMPLEMENTED" if a new entry hasn't been added here, yet.
 */
char const* getEntryTypeName( EEntryType etype ) {
	switch ( etype ) {
		case ET_NONE:
			return "ET_NONE";
		case ET_ACTION:
			return "ET_ACTION";
		case ET_BUTTON:
			return "ET_BUTTON";
		case ET_COLOR:
			return "ET_COLOR";
		case ET_MENU:
			return "ET_MENU";
		case ET_OPTION:
			return "ET_OPTION";
		case ET_TEXT:
			return "ET_NONE";
		case ET_TOGGLE:
			return "ET_TOGGLE";
		case ET_VALUE:
			return "ET_VALUE";
		default:
			break;
	}
	return "UNIMPLEMENTED";
}

/** @brief get the name of menu class name
 *
 * @param[in] mclass The enum entry to get the name of.
 * @return A sattic C-string with the name or "UNIMPLEMENTED" if a new entry hasn't been added here, yet.
 */
char const* getMenuClassName( EMenuClass mclass ) {
	switch ( mclass ) {
		case MC_FINANCE:
			return "MC_FINANCE";
		case MC_GRAPHICS:
			return "MC_GRAPHICS";
		case MC_MAIN:
			return "MC_MAIN";
		case MC_NETWORK:
			return "MC_NETWORK";
		case MC_PHYSICS:
			return "MC_PHYSICS";
		case MC_PLAY:
			return "MC_PLAY";
		case MC_PLAYERS:
			return "MC_PLAYERS";
		case MC_SOUND:
			return "MC_SOUND";
		case MC_WEATHER:
			return "MC_WEATHER";
		case MC_MENUCLASS_COUNT:
			return "MC_MENUCLASS_COUNT";
		default:
			break;
	}
	return "UNIMPLEMENTED";
}

/** @brief get the name of a text class name
 *
 * @param[in] tclass The enum entry to get the name of.
 * @return A static C-string with the name or "UNIMPLEMENTED" if a new entry hasn't been  added here, yet.
 */
char const* getTextClassName( ETextClass tclass ) {
	switch ( tclass ) {
		case TC_COLOUR:
			return "TC_COLOUR";
		case TC_LANDSLIDE:
			return "TC_LANDSLIDE";
		case TC_LANDTYPE:
			return "TC_LANDTYPE";
		case TC_LANGUAGE:
			return "TC_LANGUAGE";
		case TC_LIGHTNING:
			return "TC_LIGHTNING";
		case TC_METEOR:
			return "TC_METEOR";
		case TC_MOUSE:
			return "TC_MOUSE";
		case TC_OFFON:
			return "TC_OFFON";
		case TC_OFFONRANDOM:
			return "TC_OFFONRANDOM";
		case TC_PLAYERPREF:
			return "TC_PLAYERPREF";
		case TC_PLAYERTEAM:
			return "TC_PLAYERTEAM";
		case TC_PLAYERTYPE:
			return "TC_PLAYERTYPE";
		case TC_SATELLITE:
			return "TC_SATELLITE";
		case TC_SKIPTYPE:
			return "TC_SKIPTYPE";
		case TC_SOUNDDRIVER:
			return "TC_SOUNDDRIVER";
		case TC_TANKTYPE:
			return "TC_TANKTYPE";
		case TC_TURNTYPE:
			return "TC_TURNTYPE";
		case TC_WALLTYPE:
			return "TC_WALLTYPE";
		case TC_TEXTCLASS_COUNT:
			return "TC_TEXTCLASS_COUNT";
		case TC_FREETEXT:
			return "TC_FREETEXT";
		case TC_NONE:
			return "TC_NONE";
		default:
			break;
	}
	return "UNIMPLEMENTED";
}

#endif // ATANKS_DEBUG
