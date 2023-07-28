#ifndef ATANKS_WEAPON_H
#define ATANKS_WEAPON_H 1
//
// Created by sed on 28.07.23.
//

#include "main.h"

class WEAPON {
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	explicit WEAPON();


	/* -----------------------------------
	 * --- Public methods              ---
	 * -----------------------------------
	 */

	/* Getters */
	[[nodiscard]] int32_t     getDelayDiv() const;
	[[nodiscard]] char const* getDesc() const;
	[[nodiscard]] char const* getName() const;

	/* Setters */
	void setDesc( char const* desc_ );
	void setName( char const* name_ );


	/* -----------------------------------
	 * --- Public members              ---
	 * -----------------------------------
	 */
	int32_t cost            = 0; //!< $ :))
	int32_t amt             = 0; //!< number of weapons in one buying package
	double  mass            = 0.;
	double  drag            = 0.;
	int32_t radius          = 0; //!< of the explosion
	int32_t sound           = 0;
	int32_t etime           = 0;
	int32_t damage          = 0; //!< damage power
	int32_t picpoint        = 0; //!< which picture do we show in flight?
	int32_t spread          = 0; //!< number of weapons in the shot
	int32_t delay           = 0; //!< volleys etc.
	int32_t noimpact        = 0;
	int32_t techLevel       = 0;
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
