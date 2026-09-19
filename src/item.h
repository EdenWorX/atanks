#ifndef ATANKS_ITEM_H
#define ATANKS_ITEM_H 1
//
// Created by sed on 28.07.23.
//

#include "main.h"

#define MAX_ITEM_VALUES 10

#define ITEM_NO_SHIELD  ( -1 )

enum itemType {
	ITEM_TELEPORT            = 0, // 56 (weap_idx - WEAPONS)
	ITEM_SWAPPER             = 1, // 57
	ITEM_MASS_TELEPORT       = 2, // 58
	ITEM_FAN                 = 3, // 59
	ITEM_VENGEANCE           = 4, // 60
	ITEM_DYING_WRATH         = 5, // 61
	ITEM_FATAL_FURY          = 6, // 62
	ITEM_LGT_SHIELD          = 7,
	ITEM_MED_SHIELD          = 8,
	ITEM_HVY_SHIELD          = 9,
	ITEM_LGT_REPULSOR_SHIELD = 10,
	ITEM_MED_REPULSOR_SHIELD = 11,
	ITEM_HVY_REPULSOR_SHIELD = 12,
	ITEM_ARMOUR              = 13,
	ITEM_PLASTEEL            = 14,
	ITEM_INTENSITY_AMP       = 15,
	ITEM_VIOLENT_FORCE       = 16,
	ITEM_SLICKP              = 17,
	ITEM_DIMPLEP             = 18,
	ITEM_PARACHUTE           = 19,
	ITEM_REPAIRKIT           = 20,
	ITEM_FUEL                = 21, // 77
	ITEM_ROCKET              = 22, // 78
	ITEM_SDI                 = 23  // 79 (Last item)
};

enum selfDestructVals { SELFD_TYPE = 0, SELFD_NUMBER };

/** @class ITEM
 * @brief Shop item stats record.
 **/
class ITEM {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Construct default item stats.
	explicit ITEM();


	/* -----------------------------------
	 * --- Public methods              ---
	 * -----------------------------------
	 */

	/* Getters */
	/// Localized description.
	[[nodiscard]] char const* getDesc() const;
	/// Localized name.
	[[nodiscard]] char const* getName() const;

	/* Setters */
	/// Set the description.
	void setDesc( char const* desc_ );
	/// Set the name.
	void setName( char const* name_ );


	/* -----------------------------------
	 * --- Public members              ---
	 * -----------------------------------
	 */

	int32_t cost       = 0; ///< Price.
	int32_t amt        = 0; ///< Package amount.
	int32_t selectable = 0; ///< Selectable in battle.
	int32_t techLevel  = 0; ///< Shop tech level.
	int32_t sound      = 0; ///< Sound index.
	/// Effect values.
	double  vals[ MAX_ITEM_VALUES ]{ 0x0 };


private:
	/* -----------------------------------
	 * --- Private members             ---
	 * -----------------------------------
	 */

	char desc[ MAX_ITEM_DESC_LEN + 1 ]{ 0x0 };
	char name[ MAX_ITEM_NAME_LEN + 1 ]{ 0x0 };
};

#define HAS_ITEM 1

#endif // ATANKS_ITEM_H
