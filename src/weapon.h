#ifndef ATANKS_WEAPON_H
#define ATANKS_WEAPON_H 1
//
// Created by sed on 28.07.23.
//

#include "main.h"

enum weaponType {
	SML_MIS           = 0,
	MED_MIS           = 1,
	LRG_MIS           = 2,
	SML_NUKE          = 3,
	NUKE              = 4,
	DTH_HEAD          = 5,
	SML_SPREAD        = 6,
	MED_SPREAD        = 7,
	LRG_SPREAD        = 8,
	SUP_SPREAD        = 9,
	DTH_SPREAD        = 10,
	ARMAGEDDON        = 11,
	CHAIN_MISSILE     = 12,
	CHAIN_GUN         = 13,
	JACK_HAMMER       = 14,
	SHAPED_CHARGE     = 15,
	WIDE_BOY          = 16,
	CUTTER            = 17,
	SML_ROLLER        = 18,
	LRG_ROLLER        = 19,
	DTH_ROLLER        = 20,
	SMALL_MIRV        = 21,
	ARMOUR_PIERCING   = 22,
	CLUSTER           = 23,
	SUP_CLUSTER       = 24,
	FUNKY_BOMB        = 25,
	FUNKY_DEATH       = 26,
	FUNKY_BOMBLET     = 27,
	FUNKY_DEATHLET    = 28,
	BOMBLET           = 29,
	SUP_BOMBLET       = 30,
	BURROWER          = 31,
	PENETRATOR        = 32,
	SML_NAPALM        = 33,
	MED_NAPALM        = 34,
	LRG_NAPALM        = 35,
	NAPALM_JELLY      = 36,
	DRILLER           = 37,
	TREMOR            = 38,
	SHOCKWAVE         = 39,
	TECTONIC          = 40,
	RIOT_BOMB         = 41,
	HVY_RIOT_BOMB     = 42,
	RIOT_CHARGE       = 43,
	RIOT_BLAST        = 44,
	DIRT_BALL         = 45,
	LRG_DIRT_BALL     = 46,
	SUP_DIRT_BALL     = 47,
	SMALL_DIRT_SPREAD = 48,
	CLUSTER_MIRV      = 49,
	PERCENT_BOMB      = 50,
	REDUCER           = 51,
	THEFT_BOMB        = 52, // Last ballistic (BALLISTICS == 53)
	SML_LAZER         = 53,
	MED_LAZER         = 54,
	LRG_LAZER         = 55, // Last weapon (WEAPONS == 56)
	SML_METEOR        = 56,
	MED_METEOR        = 57,
	LRG_METEOR        = 58,
	SML_LIGHTNING     = 59,
	MED_LIGHTNING     = 60,
	LRG_LIGHTNING     = 61 // Last natural
};

#define LAST_EXPLOSIVE DRILLER

/** @class WEAPON
 * @brief Weapon stats record.
 **/
class WEAPON {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	/// Construct default weapon stats.
	explicit WEAPON();


	/* -----------------------------------
	 * --- Public methods              ---
	 * -----------------------------------
	 */

	/* Getters */
	/// Volley delay divisor.
	[[nodiscard]] int32_t     getDelayDiv() const;
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
	int32_t cost            = 0;  //!< $ :))
	int32_t amt             = 0;  //!< number of weapons in one buying package
	double  mass            = 0.; ///< Mass.
	double  drag            = 0.; ///< Air drag.
	int32_t radius          = 0;  //!< of the explosion
	int32_t sound           = 0;  ///< Sound index.
	int32_t etime           = 0;  ///< Explosion frame time.
	int32_t damage          = 0;  //!< damage power
	int32_t picpoint        = 0;  //!< which picture do we show in flight?
	int32_t spread          = 0;  //!< number of weapons in the shot
	int32_t delay           = 0;  //!< volleys etc.
	int32_t noimpact        = 0;  ///< No impact detonation.
	int32_t techLevel       = 0;  ///< Shop tech level.
	int32_t warhead         = 0;  //!< Is it a warhead?
	int32_t numSubmunitions = 0;  //!< Number of submunitions
	int32_t submunition     = 0;  //!< The next stage
	double  impartVelocity  = 0.; //!< Impart velocity 0.0-1.0 to subs
	int32_t divergence      = 0;  //!< Total angle for submunition spread
	double  spreadVariation = 0.; //!< Uniform or random distribution
	                              //!< 0-1.0 (0=uniform, 1.0=random)
	                              //!< divergence at centre of range
	double launchSpeed    = 0.;   //!< Speed given to submunitions
	double speedVariation = 0.;   //!< Uniform or random speed
	                              //!< 0-1.0 (0=uniform, 1.0=random)
	                              //!< launchSpeed at centre of range
	int32_t countdown      = 0;   //!< Set the countdown to this
	double  countVariation = 0.;  //!< Uniform or random countdown
	                              //!< 0-1.0 (0=uniform, 1.0=random)
	                              //!< countdown at centre of range


private:
	/* -----------------------------------
	 * --- Private members             ---
	 * -----------------------------------
	 */

	char desc[ MAX_ITEM_DESC_LEN + 1 ] = { 0x0 };
	char name[ MAX_ITEM_NAME_LEN + 1 ] = { 0x0 };
};

#define HAS_WEAPON 1

#endif // ATANKS_WEAPON_H
